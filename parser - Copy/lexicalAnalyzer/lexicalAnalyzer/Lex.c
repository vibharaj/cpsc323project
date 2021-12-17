#pragma warning(disable : 4996)
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

#define da_dim(name, type)  type *name = NULL;          \
                            int _qy_ ## name ## _p = 0;  \
                            int _qy_ ## name ## _max = 0
#define da_rewind(name)     _qy_ ## name ## _p = 0
#define da_redim(name)      do {if (_qy_ ## name ## _p >= _qy_ ## name ## _max) \
                                name = realloc(name, (_qy_ ## name ## _max += 32) * sizeof(name[0]));} while (0)
#define da_append(name, x)  do {da_redim(name); name[_qy_ ## name ## _p++] = x;} while (0)
#define da_len(name)        _qy_ ## name ## _p

// the different tokens
typedef enum {
    EOI_token, Mult_token, Div_token, Mod_token, Add_token, Sub_token, Neg_token, Not_token, LessThan_token, LessThanEQ_token,
    GreaterThan_token, GreaterThanEQ_token, EqualTo_token, NotEqualTo_token, AssignEQ_token, And_token, Or_token, If_token, 
    Else_token, While_token, Print_token, PutC_token, OpenParenthese_token, CloseParenthese_token, OpenBracket_token, CloseBracket_token, 
    Semicolon_token, Comma_token, Identifier_token, Int_token, String_token, Type_int_token
} TokenType;

// definition of a token
typedef struct {
    TokenType token;
    int lnError, colError;

    union {
        int n;
        char* text;
    };
} token_s;

static FILE* source_fp, * dest_fp;
static int line = 1, col = 0, ch = ' ';
da_dim(text, char);

token_s gettoken(void);

static void error(int lineError, int colError, const char* format, ...) {
    char buffer[1000];
    va_list ap;

    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);

    printf("(%d,%d) error: %s\n", lineError, colError, buffer);
    exit(1);
}

// get next char from input
static int next_ch(void) {
    ch = getc(source_fp);
    ++col;

    // if new line, increment line and reset col = 0
    if (ch == '\n') {
        ++line;
        col = 0;
    }

    return ch;
}


static token_s char_lit(int n, int lineError, int colError) {
    if (ch == '\'')
        error(lineError, colError, "gettoken: empty character constant");

    if (ch == '\\') {
        next_ch();
        if (ch == 'n')
            n = 10;
        else if (ch == '\\')
            n = '\\';
        else error(lineError, colError, "gettoken: unknown escape sequence \\%c", ch);
    }

    if (next_ch() != '\'')
        error(lineError, colError, "multi-character constant");
    next_ch();

    return (token_s) { Int_token, lineError, colError, { n } };
}


// check if / (slash) is for mathematical divide symbol or for a comment
// ex: "100/10" vs "// comment"
static token_s div_comment(int lineError, int colError) {
    if (ch != '*')
        return (token_s) { Div_token, lineError, colError, { 0 } };

    // get next char and check if comment
    next_ch();
    for (;;) {
        if (ch == '*') {
            if (next_ch() == '/') {
                next_ch();
                return gettoken();
            }
        }
        else if (ch == EOF)
            error(lineError, colError, "EOF in comment");
        else
            next_ch();
    }
}


// check if string literal
static token_s stringLiteral(int start, int lineError, int colError) { 
    da_rewind(text);

    while (next_ch() != start) {
        if (ch == '\n') error(lineError, colError, "EOL in string");
        if (ch == EOF)  error(lineError, colError, "EOF in string");
        da_append(text, (char)ch);
    }
    da_append(text, '\0');

    next_ch();
    return (token_s) { String_token, lineError, colError, { .text = text } };
}


static int keywordCompare(const void* p1, const void* p2) {
    return strcmp(*(char**)p1, *(char**)p2);
}


// check if keywords 
static TokenType getIdentifierType(const char* identifier) {
    static struct {
        const char* s;
        TokenType sym;
    } keywords[] = {
        {"else",  Else_token},
        {"if",    If_token},
        {"int", Type_int_token},
        {"print", Print_token},
        {"putc",  PutC_token},
        {"while", While_token}
    }, * kwp;

    return (kwp = bsearch(&identifier, keywords, NELEMS(keywords), sizeof(keywords[0]), keywordCompare)) == NULL ? Identifier_token : kwp->sym;
}


// check if identifier or int
static token_s identifier_int(int lineError, int colError) {
    int n, is_number = true;

    da_rewind(text);
    while (isalnum(ch) || ch == '_') {
        da_append(text, (char)ch);
        if (!isdigit(ch))
            is_number = false;
        next_ch();
    }
    if (da_len(text) == 0)
        error(lineError, colError, "gettoken: unrecognized character (%d) '%c'\n", ch, ch);

    da_append(text, '\0');
    if (isdigit(text[0])) {
        if (!is_number)
            error(lineError, colError, "invalid number: %s\n", text);

        n = strtol(text, NULL, 0);
        if (n == LONG_MAX && errno == ERANGE)
            error(lineError, colError, "Number exceeds maximum value");

        return (token_s) { Int_token, lineError, colError, { n } };
    }

    return (token_s) { getIdentifierType(text), lineError, colError, { .text = text } };
}


// check following char to see if its ">=, <=, etc"
static token_s follow(int expect, TokenType ifyes, TokenType ifno, int lineError, int colError) {
    if (ch == expect) {
        next_ch();
        return (token_s) { ifyes, lineError, colError, { 0 } };
    }
    if (ifno == EOI_token)
        error(lineError, colError, "follow: unrecognized character '%c' (%d)\n", ch, ch);

    return (token_s) { ifno, lineError, colError, { 0 } };
}


// return token 
token_s gettoken(void) {
    while (isspace(ch))
        next_ch();

    int lineError = line;
    int colError = col;

    switch (ch) {
    case '{':  next_ch(); return (token_s) { OpenBracket_token,     lineError, colError, { 0 } };
    case '}':  next_ch(); return (token_s) { CloseBracket_token,    lineError, colError, { 0 } };
    case '(':  next_ch(); return (token_s) { OpenParenthese_token,  lineError, colError, { 0 } };
    case ')':  next_ch(); return (token_s) { CloseParenthese_token, lineError, colError, { 0 } };
    case '+':  next_ch(); return (token_s) { Add_token,             lineError, colError, { 0 } };
    case '-':  next_ch(); return (token_s) { Sub_token,             lineError, colError, { 0 } };
    case '*':  next_ch(); return (token_s) { Mult_token,            lineError, colError, { 0 } };
    case '%':  next_ch(); return (token_s) { Mod_token,             lineError, colError, { 0 } };
    case ';':  next_ch(); return (token_s) { Semicolon_token,       lineError, colError, { 0 } };
    case ',':  next_ch(); return (token_s) { Comma_token,           lineError, colError, { 0 } };
    case '/':  next_ch(); return div_comment(lineError, colError);
    case '\'': next_ch(); return char_lit(ch, lineError, colError);
    case '<':  next_ch(); return follow('=', LessThanEQ_token,    LessThan_token,    lineError, colError);
    case '>':  next_ch(); return follow('=', GreaterThanEQ_token, GreaterThan_token, lineError, colError);
    case '=':  next_ch(); return follow('=', EqualTo_token,       AssignEQ_token,    lineError, colError);
    case '!':  next_ch(); return follow('=', NotEqualTo_token,    Not_token,         lineError, colError);
    case '&':  next_ch(); return follow('&', And_token,           EOI_token,         lineError, colError);
    case '|':  next_ch(); return follow('|', Or_token,            EOI_token,         lineError, colError);
    case '"': return stringLiteral(ch, lineError, colError);
    default:   return identifier_int(lineError, colError);
    case EOF:  return (token_s) { EOI_token, lineError, colError, { 0 } };
    }
}


// tokenize input
void run(void) {
    token_s token;
    do {
        token = gettoken();
        fprintf(dest_fp, "%5d  %5d %.15s",
            token.lnError, token.colError,
            &"End_of_input    Op_multiply     Op_divide       Op_mod          Op_add          "
            "Op_subtract     Op_negate       Op_not          Op_less         Op_lessequal    "
            "Op_greater      Op_greaterequal Op_equal        Op_notequal     Op_assign       "
            "Op_and          Op_or           Keyword_if      Keyword_else    Keyword_while   "
            "Keyword_print   Keyword_putc    LeftParen       RightParen      LeftBrace       "
            "RightBrace      Semicolon       Comma           Identifier      Integer         "
            "String          Keyword_int"
            [token.token * 16]);
        if (token.token == Int_token)     fprintf(dest_fp, "  %4d", token.n);
        else if (token.token == Identifier_token)  fprintf(dest_fp, " %s", token.text);
        else if (token.token == String_token) fprintf(dest_fp, " \"%s\"", token.text);
        fprintf(dest_fp, "\n");
    } while (token.token != EOI_token);
    
    if (dest_fp != stdout)
        fclose(dest_fp);
}

void init_io(FILE** fp, FILE* std, const char mode[], const char fn[]) {
    if (fn[0] == '\0')
        *fp = std;
    else if ((*fp = fopen(fn, mode)) == NULL)
        error(0, 0, "Can't open %s\n", fn);
}

int main(int argc, char* argv[]) {
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    run();
    return 0;
}