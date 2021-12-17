#pragma warning(disable : 4996)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>

typedef unsigned char uchar;

// classifiers 
typedef enum {
    nd_Ident, nd_String, nd_Integer, nd_Sequence, nd_If, nd_Prtc, nd_Prts, nd_Prti, nd_While,
    nd_Assign, nd_Negate, nd_Not, nd_Mul, nd_Div, nd_Mod, nd_Add, nd_Sub, nd_Lss, nd_Leq,
    nd_Gtr, nd_Geq, nd_Eql, nd_Neq, nd_And, nd_Or, nd_Decl, nd_null
} NodeType;

// operators
typedef enum {
    FETCH, STORE, PUSH, ADD, SUB, MUL, DIV, MOD, LT, GT, LE, GE, EQ, NE, AND,
    OR, NEG, NOT, JMP, JZ, PRTC, PRTS, PRTI, HALT
} Code_t;

typedef uchar code;

// tree structure 
typedef struct Tree {
    NodeType node_type;
    struct Tree* left;
    struct Tree* right;
    char* value;
} Tree;

// definition of symbol table
typedef struct sym {
    char symbol[255];
    NodeType sym_type;
} SymbolRecord;

#define da_dim(name, type)  type *name = NULL;          \
                            int _qy_ ## name ## _p = 0;  \
                            int _qy_ ## name ## _max = 0

#define da_redim(name)      do {if (_qy_ ## name ## _p >= _qy_ ## name ## _max) \
                                name = realloc(name, (_qy_ ## name ## _max += 32) * sizeof(name[0]));} while (0)

#define da_rewind(name)     _qy_ ## name ## _p = 0

#define da_append(name, x)  do {da_redim(name); name[_qy_ ## name ## _p++] = x;} while (0)
#define da_len(name)        _qy_ ## name ## _p
#define da_add(name)        do {da_redim(name); _qy_ ## name ## _p++;} while (0)
#define MAX_SYMBOL_TABLE_SIZE 1024
SymbolRecord symTable[MAX_SYMBOL_TABLE_SIZE];

FILE* source_fp, * dest_fp;
static int here;
da_dim(object, code);
da_dim(globals, const char*);
da_dim(string_pool, const char*);

// NodeType, enum
struct {
    char* enum_text;
    NodeType   node_type;
    Code_t     opcode;
} atr[] = {
    {"Identifier"  , nd_Ident,    -1 },
    {"String"      , nd_String,   -1 },
    {"Integer"     , nd_Integer,  -1 },
    {"Sequence"    , nd_Sequence, -1 },
    {"If"          , nd_If,       -1 },
    {"Prtc"        , nd_Prtc,     -1 },
    {"Prts"        , nd_Prts,     -1 },
    {"Prti"        , nd_Prti,     -1 },
    {"While"       , nd_While,    -1 },
    {"Assign"      , nd_Assign,   -1 },
    {"Negate"      , nd_Negate,   NEG},
    {"Not"         , nd_Not,      NOT},
    {"Multiply"    , nd_Mul,      MUL},
    {"Divide"      , nd_Div,      DIV},
    {"Mod"         , nd_Mod,      MOD},
    {"Add"         , nd_Add,      ADD},
    {"Subtract"    , nd_Sub,      SUB},
    {"Less"        , nd_Lss,      LT },
    {"LessEqual"   , nd_Leq,      LE },
    {"Greater"     , nd_Gtr,      GT },
    {"GreaterEqual", nd_Geq,      GE },
    {"Equal"       , nd_Eql,      EQ },
    {"NotEqual"    , nd_Neq,      NE },
    {"And"         , nd_And,      AND},
    {"Or"          , nd_Or,       OR },
    {"int"         , nd_Decl,     -1 }
};

// error function
void error(const char* f, ...) {
    va_list ap;
    char buf[1000];

    va_start(ap, f);
    vsprintf(buf, f, ap);
    va_end(ap);
    printf("error: %s\n", buf);
    exit(1);
}

// type to operator
Code_t type_to_op(NodeType Ntype) {
    return atr[Ntype].opcode;
}

// node generator function
Tree* make_node(NodeType node_type, Tree* l, Tree* r) {
    Tree* t = calloc(sizeof(Tree), 1);
    t->node_type = node_type;

    // assign l and r to left and right
    t->left = l;
    t->right = r;
    return t;
}

// leaf generator function
Tree* make_leaf(NodeType node_type, char* val) {
    Tree* t = calloc(sizeof(Tree), 1);
    t->node_type = node_type;
    t->value = strdup(val);
    return t;
}

int hole() {
    int t = here;
    return t;
}

void fix(int src, int dst) {
    *(int32_t*)(object + src) = dst - src;
}

int fetch_var_offset(const char* id) {
    for (int i = 0; i < da_len(globals); ++i) {
        // comparing id and globals
        if (strcmp(id, globals[i]) == 0)
            return i;
    }
    da_add(globals);
    int n = da_len(globals) - 1;
    globals[n] = strdup(id);
    return n;
}

// looking up symbol in symbol table
int symbol_lookup(const char* id, NodeType t) {
    int i;
    for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; ++i) {
        if (strcmp(symTable[i].symbol, "") == 0) {
            strcpy(symTable[i].symbol, id);
            symTable[i].sym_type = t;
            break;
        }
        else {
            if (strcmp(symTable[i].symbol, id) == 0) {
                if (symTable[i].sym_type == t) {
                    break;
                }
                else {
                    return -1;
                }
            }
        }
        
    }
    return i;
   
}

// match type from symbol table
int MatchTypeFromSymbolTable(const char* id, NodeType t) {
    int i;
    int sym_found = -1;

    for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; ++i) {
        if (strcmp(symTable[i].symbol, "") == 0) {
            break;
        }
        else {
            if (strcmp(symTable[i].symbol, id) == 0) {
                if (symTable[i].sym_type == t) {
                    sym_found = i;
                }
                break;
            }
        }

    }
    return sym_found;

}

// add type to symbol table
int AddToSymbolTable(const char* id, NodeType t) {
    int i;

    for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; ++i) {
        if (strcmp(symTable[i].symbol, "") == 0) {
            strcpy(symTable[i].symbol, id);
            symTable[i].sym_type = t;
            break;
        }
    }

    return(i);

}

// checks if id is in symbol table
int IsInSymbolTable(const char* id) {
    int i;
    int sym_found = -1;

    for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; ++i) {
        if (strcmp(symTable[i].symbol, "") != 0) {
            if (strcmp(symTable[i].symbol, id) == 0) {
                sym_found = i;
                break;
            }
        }
        else {
            break;
        }
    }
    return sym_found;

}

// getting string offset
int fetch_string_offset(const char* s) {
    for (int i = 0; i < da_len(string_pool); ++i) {
        if (strcmp(s, string_pool[i]) == 0)
            return i;
    }
    da_add(string_pool);
    // n is the length of string offset
    int n = da_len(string_pool) - 1;
    string_pool[n] = strdup(s);
    return n;
}

// traverse tree
void tree_walk (Tree* x) {
    int p1, p2, n;
    char* pstr = NULL;

    if (x == NULL) return;
    switch (x->node_type) {
    // declaration
    case nd_Decl:
        n = AddToSymbolTable(x->left->value, nd_Integer);
        break;
    // identifier
    case nd_Ident:
        n = IsInSymbolTable(x->value);
        if (n < 0)
        {
            error("Variable %s is not defined.\n", x->value);
        }
        break;
    // integer
    case nd_Integer:
        break;
    // string
    case nd_String:
        n = fetch_string_offset(x->value);
        break;
    // assignment
    case nd_Assign:
        n = IsInSymbolTable(x->left->value);
        if (n < 0)
        {
            error("Variable %s is not defined.\n", x->left->value);
        }
        n = MatchTypeFromSymbolTable(x->left->value, x->right->node_type);
        if (n < 0) {
            switch (x->right->node_type)
            {
                case nd_String:
                    pstr = "string";
                    break;
                case nd_Integer:
                    pstr = "int";
                    break;
                default:
                    ;
            }
            error("Type Mismatch. LHS variable %s is not of type %s\n", x->left->value, pstr);
        }
        tree_walk(x->right);
        break;
    // if
    case nd_If:
        tree_walk(x->left);
        p1 = hole();
        tree_walk(x->right->left);
        if (x->right->right != NULL) {
            p2 = hole();
        }
        fix(p1, here);
        if (x->right->right != NULL) {
            tree_walk(x->right->right);
            fix(p2, here);
        }
        break;
    // while
    case nd_While:
        p1 = here;
        tree_walk(x->left);
        p2 = hole();
        tree_walk(x->right);
        fix(hole(), p1);
        fix(p2, here);
        break;
    // sequence
    case nd_Sequence:
        tree_walk(x->left);
        tree_walk(x->right);
        break;
    case nd_Prtc:
        tree_walk(x->left);
        break;
    case nd_Prti:
        tree_walk(x->left);
        break;
    case nd_Prts:
        tree_walk(x->left);
        break;
    // other cases
    case nd_Lss: case nd_Gtr: case nd_Leq: case nd_Geq: case nd_Eql: case nd_Neq:
    case nd_And: case nd_Or: case nd_Sub: case nd_Add: case nd_Div: case nd_Mul:
    case nd_Mod:
        tree_walk(x->left);
        tree_walk(x->right);
        break;
    // negation
    case nd_Negate: case nd_Not:
        tree_walk(x->left);
        break;
    default:
        error("error in semantic analyzer - found %d, expecting operator\n", x->node_type);
    }
}

// if unable to open file
void init_io(FILE** fp, FILE* std, const char mode[], const char fn[]) {
    if (fn[0] == '\0')
        *fp = std;
    else if ((*fp = fopen(fn, mode)) == NULL)
        error(0, 0, "Can't open %s\n", fn);
}

void init_symbol_table() {
    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; ++i) {
        strcpy(symTable[i].symbol, "");
        symTable[i].sym_type = nd_null;
    }
}

// getting enum val
NodeType get_enum_value(const char name[]) {
    for (size_t i = 0; i < sizeof(atr) / sizeof(atr[0]); i++) {
        if (strcmp(atr[i].enum_text, name) == 0) {
            return atr[i].node_type;
        }
    }
    error("Unknown token %s\n", name);
    return -1;
}

// read line
char* read_line(int* l) {
    static char* text = NULL;
    static int text_max = 0;

    for (*l = 0; ; (*l)++) {
        int ch = fgetc(source_fp);
        if (ch == EOF || ch == '\n') {
            if (*l == 0)
                return NULL;
            break;
        }
        if (*l + 1 >= text_max) {
            text_max = (text_max == 0 ? 128 : text_max * 2);
            text = realloc(text, text_max);
        }
        text[*l] = ch;
    }
    text[*l] = '\0';
    return text;
}

//removes trailing spaces
char* rtrim(char* text, int* l) {
    for (; *l > 0 && isspace(text[*l - 1]); --(*l))
        ;

    text[*l] = '\0';
    return text;
}

// load abstract syntax tree
Tree* load_ast() {
    int l;
    char* yytext = read_line(&l);
    yytext = rtrim(yytext, &l);

    // getting first token
    char* tok = strtok(yytext, " ");

    if (tok[0] == ';') {
        return NULL;
    }
    NodeType node_type = get_enum_value(tok);

    // get extra data
    char* p = tok + strlen(tok);
    if (p != &yytext[l]) {
        for (++p; isspace(*p); ++p)
            ;
        return make_leaf(node_type, p);
    }

    Tree* left = load_ast();
    Tree* right = load_ast();
    return make_node(node_type, left, right);
}

// main function
int main(int argc, char* argv[]) {
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");

    init_symbol_table();

    Tree* ast = load_ast();
    tree_walk(ast);

    return 0;
}