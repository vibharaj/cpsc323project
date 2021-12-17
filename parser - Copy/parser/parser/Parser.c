#pragma warning(disable : 4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

// different tokens
typedef enum {
    EOI_token, Mul_token, Div_token, Mod_token, Add_token, Sub_token, Neg_token, Not_token, LessThan_token, LessThanEQ_token, 
    GreaterThan_token, GreaterThanEQ_token, EqualTo_token, NotEqualTo_token, AssignEQ_token, And_token, Or_token, If_token, 
    Else_token, While_token, Print_token, PutC_token, OpenParenthese_token, CloseParenthese_token, OpenBracket_token, CloseBracket_token, 
    Semicolon_token, Comma_token, Idenifier_token, Int_token, String_token, Type_int_token
} TokenType;

// `
typedef enum {
    nd_Ident, nd_String, nd_Integer, nd_Sequence, nd_If, nd_Prtc, nd_Prts, nd_Prti, nd_While,
    nd_Assign, nd_Negate, nd_Not, nd_Mul, nd_Div, nd_Mod, nd_Add, nd_Sub, nd_Lss, nd_Leq,
    nd_Gtr, nd_Geq, nd_Eql, nd_Neq, nd_And, nd_Or, nd_Decl
} NodeType;

// definition of a token
typedef struct {
    TokenType token;
    int lnError, colError;
    char* text;
} token_s;


// definition of a tree
typedef struct Tree {
    NodeType node_type;
    struct Tree* left;
    struct Tree* right;
    char* value;
} Tree;


struct {
    char* text, * enum_text;
    TokenType   token;
    bool        right_associative, is_binary, is_unary;
    int         precedence;
    NodeType    node_type;
} atrr[] = {
    {"EOI",             "End_of_input"   , EOI_token,               false, false, false, -1, -1        },
    {"*",               "Op_multiply"    , Mul_token,               false, true,  false, 13, nd_Mul    },
    {"/",               "Op_divide"      , Div_token,               false, true,  false, 13, nd_Div    },
    {"%",               "Op_mod"         , Mod_token,               false, true,  false, 13, nd_Mod    },
    {"+",               "Op_add"         , Add_token,               false, true,  false, 12, nd_Add    },
    {"-",               "Op_subtract"    , Sub_token,               false, true,  false, 12, nd_Sub    },
    {"-",               "Op_negate"      , Neg_token,               false, false, true,  14, nd_Negate },
    {"!",               "Op_not"         , Not_token,               false, false, true,  14, nd_Not    },
    {"<",               "Op_less"        , LessThan_token,          false, true,  false, 10, nd_Lss    },
    {"<=",              "Op_lessequal"   , LessThanEQ_token,        false, true,  false, 10, nd_Leq    },
    {">",               "Op_greater"     , GreaterThan_token,       false, true,  false, 10, nd_Gtr    },
    {">=",              "Op_greaterequal", GreaterThanEQ_token,     false, true,  false, 10, nd_Geq    },
    {"==",              "Op_equal"       , EqualTo_token,           false, true,  false,  9, nd_Eql    },
    {"!=",              "Op_notequal"    , NotEqualTo_token,        false, true,  false,  9, nd_Neq    },
    {"=",               "Op_assign"      , AssignEQ_token,          false, false, false, -1, nd_Assign },
    {"&&",              "Op_and"         , And_token,               false, true,  false,  5, nd_And    },
    {"||",              "Op_or"          , Or_token,                false, true,  false,  4, nd_Or     },
    {"if",              "Keyword_if"     , If_token,                false, false, false, -1, nd_If     },
    {"else",            "Keyword_else"   , Else_token,              false, false, false, -1, -1        },
    {"while",           "Keyword_while"  , While_token,             false, false, false, -1, nd_While  },
    {"print",           "Keyword_print"  , Print_token,             false, false, false, -1, -1        },
    {"putc",            "Keyword_putc"   , PutC_token,              false, false, false, -1, -1        },
    {"int",             "Keyword_int"    , Type_int_token,          false, false, false, -1, nd_Decl   },
    {"(",               "LeftParen"      , OpenParenthese_token,    false, false, false, -1, -1        },
    {")",               "RightParen"     , CloseParenthese_token,   false, false, false, -1, -1        },
    {"{",               "LeftBrace"      , OpenBracket_token,       false, false, false, -1, -1        },
    {"}",               "RightBrace"     , CloseBracket_token,      false, false, false, -1, -1        },
    {";",               "Semicolon"      , Semicolon_token,         false, false, false, -1, -1        },
    {",",               "Comma"          , Comma_token,             false, false, false, -1, -1        },
    {"Ident",           "Identifier"     , Idenifier_token,         false, false, false, -1, nd_Ident  },
    {"Integer literal", "Integer"        , Int_token, false,        false, false,        -1, nd_Integer},
    {"String literal",  "String"         , String_token,            false, false, false, -1, nd_String }
};

char* Display_nodes[] = { "Identifier", "String", "Integer", "Sequence", "If", "Prtc",
    "Prts", "Prti", "While", "Assign", "Negate", "Not", "Multiply", "Divide", "Mod",
    "Add", "Subtract", "Less", "LessEqual", "Greater", "GreaterEqual", "Equal",
    "NotEqual", "And", "Or", "int" };

static token_s token;
static FILE* source_fp, * dest_fp;

Tree* parentheseExpr();

void error(int lineError, int colError, const char* format, ...) {
    va_list ap;
    char buffer[1000];

    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);
    printf("(%d, %d) error: %s\n", lineError, colError, buffer);
    exit(1);
}

char* read_line(int* len) {
    static char* text = NULL;
    static int textmax = 0;

    for (*len = 0; ; (*len)++) {
        int ch = fgetc(source_fp);
        if (ch == EOF || ch == '\n') {
            if (*len == 0)
                return NULL;
            break;
        }
        if (*len + 1 >= textmax) {
            textmax = (textmax == 0 ? 128 : textmax * 2);
            text = realloc(text, textmax);
        }
        text[*len] = ch;
    }
    text[*len] = '\0';
    return text;
}

char* rightTrim(char* text, int* len) {
    for (; *len > 0 && isspace(text[*len - 1]); --(*len))
        ;

    text[*len] = '\0';
    return text;
}

TokenType getEnum(const char* name) {
    for (size_t i = 0; i < NELEMS(atrr); i++) {
        if (strcmp(atrr[i].enum_text, name) == 0)
            return atrr[i].token;
    }
    error(0, 0, "Unknown token %s\n", name);
    return 0;
}

token_s gettoken() {
    int len;
    token_s token;
    char* currText = read_line(&len);
    currText = rightTrim(currText, &len);

    // [ ]*{lineno}[ ]+{colno}[ ]+token[ ]+optional

    // get line and column
    //atoi() is used to convert string value to int value
    token.lnError = atoi(strtok(currText, " "));
    token.colError = atoi(strtok(NULL, " "));

    // get the token name
    char* name = strtok(NULL, " ");
    token.token = getEnum(name);

    // if there is extra data, get it
    char* p = name + strlen(name);
    if (p != &currText[len]) {
        for (++p; isspace(*p); ++p)
            ;
        token.text = strdup(p);
    }
    return token;
}

Tree* makeNode(NodeType node_type, Tree* left, Tree* right) {
    Tree* t = calloc(sizeof(Tree), 1);
    t->node_type = node_type;
    t->left = left;
    t->right = right;
    return t;
}

Tree* makeLeaf(NodeType node_type, char* value) {
    Tree* t = calloc(sizeof(Tree), 1);
    t->node_type = node_type;
    t->value = strdup(value);
    return t;
}

void expect(const char msg[], TokenType s) {
    if (token.token == s) {
        token = gettoken();
        return;
    }
    error(token.lnError, token.colError, "%s: Expecting '%s', found '%s'\n", msg, atrr[s].text, atrr[token.token].text);
}

Tree* expression(int p) {
    Tree* x = NULL, * node;
    TokenType op;

    switch (token.token) {
    case OpenParenthese_token:
        x = parentheseExpr();
        break;
    case Sub_token: case Add_token:
        op = token.token;
        token = gettoken();
        node = expression(atrr[Neg_token].precedence);
        x = (op == Sub_token) ? makeNode(nd_Negate, node, NULL) : node;
        break;
    case Not_token:
        token = gettoken();
        x = makeNode(nd_Not, expression(atrr[Not_token].precedence), NULL);
        break;
    case Idenifier_token:
        x = makeLeaf(nd_Ident, token.text);
        token = gettoken();
        break;
    case Int_token:
        x = makeLeaf(nd_Integer, token.text);
        token = gettoken();
        break;
    case String_token:
        x = makeLeaf(nd_String, token.text);
        token = gettoken();
        break;
    case Type_int_token:
        token = gettoken();
        x = makeLeaf(nd_Ident, token.text);
        token = gettoken();
        break;

    default:
        error(token.lnError, token.colError, "Expecting a primary, found: %s\n", atrr[token.token].text);
    }

    while (atrr[token.token].is_binary && atrr[token.token].precedence >= p) {
        TokenType op = token.token;

        token = gettoken();

        int q = atrr[op].precedence;
        if (!atrr[op].right_associative)
            q++;

        node = expression(q);
        x = makeNode(atrr[op].node_type, x, node);
    }
    return x;
}

Tree* parentheseExpr() {
    expect("parentheseExpr", OpenParenthese_token);
    Tree* t = expression(0);
    expect("parentheseExpr", CloseParenthese_token);
    return t;
}

Tree* statement() {
    Tree* t = NULL, * v, * e, * s, * s2;

    switch (token.token) {
    case If_token:
        token = gettoken();
        e = parentheseExpr();
        s = statement();
        s2 = NULL;
        if (token.token == Else_token) {
            token = gettoken();
            s2 = statement();
        }
        t = makeNode(nd_If, e, makeNode(nd_If, s, s2));
        break;

    case PutC_token:
        token = gettoken();
        e = parentheseExpr();
        t = makeNode(nd_Prtc, e, NULL);
        expect("Putc", Semicolon_token);
        break;

    case Print_token:
        token = gettoken();
        for (expect("Print", OpenParenthese_token); ; expect("Print", Comma_token)) {
            if (token.token == String_token) {
                e = makeNode(nd_Prts, makeLeaf(nd_String, token.text), NULL);
                token = gettoken();
            }
            else
                e = makeNode(nd_Prti, expression(0), NULL);

            t = makeNode(nd_Sequence, t, e);

            if (token.token != Comma_token)
                break;
        }
        expect("Print", CloseParenthese_token);
        expect("Print", Semicolon_token);
        break;

    case Type_int_token:
        //token = gettoken();
        e = expression(0);
        t = makeNode(nd_Decl, e, NULL);
        expect("int", Semicolon_token);
        break;

    case Semicolon_token:
        token = gettoken();
        break;

    case Idenifier_token:
        v = makeLeaf(nd_Ident, token.text);
        token = gettoken();
        expect("assign", AssignEQ_token);
        e = expression(0);
        t = makeNode(nd_Assign, v, e);
        expect("assign", Semicolon_token);
        break;

    case While_token:
        token = gettoken();
        e = parentheseExpr();
        s = statement();
        t = makeNode(nd_While, e, s);
        break;

    case OpenBracket_token:
        for (expect("Lbrace", OpenBracket_token); token.token != CloseBracket_token && token.token != EOI_token;)
            t = makeNode(nd_Sequence, t, statement());
        expect("Lbrace", CloseBracket_token);
        break;

    case EOI_token:
        break;
        
    default: error(token.lnError, token.colError, "expecting start of statement, found '%s'\n", atrr[token.token].text);
    }
    return t;
}

Tree* parse() {
    Tree* t = NULL;

    token = gettoken();
    do {
        t = makeNode(nd_Sequence, t, statement());
    } while (t != NULL && token.token != EOI_token);
    return t;
}

void prt_ast(Tree* t) {
    if (t == NULL)
        printf(";\n");
    else {
        printf("%-14s ", Display_nodes[t->node_type]);
        if (t->node_type == nd_Ident || t->node_type == nd_Integer || t->node_type == nd_String) {
            printf("%s\n", t->value);
        }
        else {
            printf("\n");
            prt_ast(t->left);
            prt_ast(t->right);
        }
    }
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
    prt_ast(parse());
}