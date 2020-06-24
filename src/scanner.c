/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     scanner.c
    Autor:      Kateřina Mušková (xmusko00)
    Úpravy:     
*********************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "error.h"
#include "scanner.h"
#include "dynamic_string.h"
#include "stack.h"

#define L_A "Lexical Analyzer"

/// ########### Globals  ############

string *lex_str;
e_ind *ind;
e_char *c;

int state;
token g_token_ptr;

/// ########### Help string and char functions  ############

// lexeme of all key words
enum lexeme key_words_lex[] =
        {T_DEF, T_IF, T_ELSE, T_WHILE, T_RETURN, T_PASS, T_NONE};

// lexeme of all build-in functions
enum lexeme bi_fnc_lex[] =
        {T_INPUT_S, T_INPUT_I, T_INPUT_F, T_PRINT, T_LEN, T_PASS, T_SUBSTR, T_ORD, T_CHR};

const char *key_words[] = {
        [T_DEF] = "def",
        [T_IF] = "if",
        [T_ELSE] = "else",
        [T_WHILE] = "while",
        [T_RETURN] = "return",
        [T_PASS] = "pass",
        [T_NONE] = "None"
};

const char *build_in_fnc[] = {
        [T_INPUT_S] = "inputs",
        [T_INPUT_I] = "inputi",
        [T_INPUT_F] = "inputf",
        [T_PRINT] = "print",
        [T_LEN] = "len",
        [T_PASS] = "pass",
        [T_SUBSTR] = "substr",
        [T_ORD] = "ord",
        [T_CHR] = "chr",
};

int c_is_letter(char chr) {
    if ((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z'))
        return 1;
    return 0;
}

int c_is_digit(char chr) {
    return (chr >= '0' && chr <= '9') ? 1 : 0;
}

int c_is_H_digit(char chr) {
    if (c_is_digit(chr) || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F'))
        return 1;
    return 0;
}

int c_is_O_digit(char chr) {
    return (chr >= '0' && chr <= '7') ? 1 : 0;
}

int c_is_B_digit(char chr) {
    return (chr == '0' || chr == '1') ? 1 : 0;
}

int c_is_ID(char chr) {
    if (c_is_digit(chr) || c_is_letter(chr) || chr == '_')
        return 1;
    return 0;
}

int c_is_tab_space(char chr) {
    return (chr == ' ' || chr == '\t') ? 1 : 0;
}

// !!! WITHOUT BACKSLASH and APOSTROPHE !!!
int c_is_A31(char chr) {
    return (chr >= 31 && chr != '\'' && chr != '\\') ? 1 : 0;
}

int c_is_esc_char(char chr) {
    if (chr == '"' || chr == '\'' || chr == '\\' || chr == 'n' || chr == 't')
        return 1;
    return 0;
}

// there is limited number of chars, that can follow after ID, str, dbl and int
// i call them friendly
int c_is_next_friendly(char chr) {
    if (chr == ' ' || chr == '\t' || chr == '\n' ||       // white chars
        chr == '#' ||                                 // line comment
        (chr >= 40 && chr <= 45) ||                     // ( ) * + , -
        chr == '/' || chr == ':' ||                     // / :
        (chr >= 60 && chr <= 62)) {                     // < = >
        return 1;
    }
    return 0;
}

void conv_to_ifjcode_esc(string *str) {
    char *temp = str_as_chars(str);
    str_clear(str, 1);
    for (int i = 0; temp[i] != '\0'; i++) {
        if ((temp[i] >= 10 && temp[i] <= 32) || temp[i] == 35 || temp[i] == 92) {
            string *hex;
            if (temp[i] >= 10) hex = str_init_fmt("\\0%d", temp[i]);
            else hex = str_init_fmt("\\00%d", temp[i]);
            str_cat(str, hex);
            str_destroy(hex);
        } else {
            str_cat_char(str, temp[i]);
        }
    }
    free(temp);
}

int conv_esc(string *s) {

    char chr;
    str_get_last(s, &chr);
    str_del_chars(s, 2);

    if (chr == 'n') str_cat_char(s, '\n');
    else if (chr == 't') str_cat_char(s, '\t');
    else if (chr == '\'')str_cat_char(s, '\'');
    else if (chr == '"') str_cat_char(s, '\"');
    else if(chr == '\\') str_cat_char(s, '\\');
    else return -1;

    return 0;
}

int conv_esc_hex(string *s) {
    char chr[2];
    str_get_last(s, &chr[1]);
    str_del_char(s);
    str_get_last(s, &chr[0]);
    str_del_chars(s, 3);

    char *p_end = NULL;
    int new_char;
    new_char = (int) strtol(chr, &p_end, 16);
    if (p_end == s->str || *p_end != 0) {
        return -1;
    }
    str_cat_char(s, (char) new_char);

    return 0;

}

/// ########### Functions managing final token  ############

// this function is called by any lexical error
// sets err_flag and returns token T_LEX_UNKNOWN
token mng_LA_err(char *msg) {
    debug("STATE: %d, char: \"%c\"\n", state, (char) c->c);
    error(L_A, ERR_LEX_AN, "Unknown lex. %s", msg);
    g_token_ptr->lexeme = T_LEX_UNKNOWN;
    return g_token_ptr;
}

// got to next state, add current char to stack if needed
void mng_os(int new_state, int add_c_to_lex_str) {
    if (add_c_to_lex_str) str_cat_char(lex_str, (char) c->c);
    state = new_state;
}

// final state, function returns token
// enables current char for next round if needed
token mng_end_fs(enum lexeme t_state, int enable_char) {
    g_token_ptr->lexeme = t_state;
    c->enable = enable_char;
    return g_token_ptr;
}

// function returns ID, key word or build-in function token
token mng_ID() {
    if (c_is_next_friendly((char) c->c) || c->c == EOF) c->enable = 1;
    else return mng_LA_err("");

    // key word
    for (unsigned long i = 0; i < (sizeof(key_words_lex) / (sizeof(enum lexeme))); i++) {
        enum lexeme t = key_words_lex[i];
        if (str_cmp_chars(lex_str, key_words[t]) == 1) {
            g_token_ptr->lexeme = t;
            return g_token_ptr;
        }
    }
    // build-in fnc
    for (unsigned long i = 0; i < (sizeof(bi_fnc_lex) / (sizeof(enum lexeme))); i++) {
        enum lexeme t = bi_fnc_lex[i];
        if (str_cmp_chars(lex_str, build_in_fnc[t]) == 1) {
            g_token_ptr->lexeme = t;
            return g_token_ptr;
        }
    }
    // ID
    g_token_ptr->value.id_key = str_init();

    if (g_token_ptr->value.id_key == NULL) {
        error(L_A, ERR_INTERN, "Can not init string.");
        free_token(g_token_ptr);
        return NULL;
    }

    g_token_ptr->lexeme = T_ID;
    str_cpy(g_token_ptr->value.id_key, lex_str);
    return g_token_ptr;
}

// function converts current string in stack to double
// and returns double token with value
token mng_double() {
    if (c_is_next_friendly((char) c->c) || c->c == EOF) c->enable = 1;
    else return mng_LA_err("");

    double d;
    if (str_to_dbl(lex_str, &d) == -1) {
        error(L_A, ERR_INTERN, "Can not convert str to double");
        free_token(g_token_ptr);
        return NULL;
    }
    g_token_ptr->lexeme = T_DOUBLE;
    g_token_ptr->value.floating_point = d;
    return g_token_ptr;
}

// function converts current string in stack to int
// and returns int token with value
token mng_int(int base) {
    if (c_is_next_friendly((char) c->c) || c->c == EOF) c->enable = 1;
    else return mng_LA_err("");

    int i;
    if (str_to_int(lex_str, &i, base) == -1) return mng_LA_err("Can not convert str to int.");

    g_token_ptr->lexeme = T_INT;
    g_token_ptr->value.integer = i;
    return g_token_ptr;
}

// function returns string token
token mng_str() {
    if (c_is_next_friendly((char) c->c) || c->c == EOF) c->enable = 1;
    else return mng_LA_err("");

    g_token_ptr->value.string_struct = str_init();

    if (g_token_ptr->value.string_struct == NULL) {
        error(L_A, ERR_INTERN, "Can not init string.");
        free_token(g_token_ptr);
        return NULL;
    }

    g_token_ptr->lexeme = T_STRING;
    conv_to_ifjcode_esc(lex_str);
    str_cpy(g_token_ptr->value.string_struct, lex_str);
    c->enable = 1;
    return g_token_ptr;
}

// [0, 2, 3, 6, 10, 26]
// function returns indent, dedent or nothing if the value of indent did not change
token mng_ind() {
    if (ind->ind > indent_stack->head->value) {           // 28
        // new higher indent, return indent
        // next time start normally with this input char
        // next time T_EOL
        c->enable = 1;
        ind->enable = 0;

        if (ds_push(indent_stack, ind->ind) != 0) {
            free_token(g_token_ptr);
            return NULL;
        }

        ind->ind = 0;
        g_token_ptr->lexeme = T_INDENT;
        return g_token_ptr;

    } else if (ind->ind < indent_stack->head->value) {
        ds_pop(indent_stack);

        if (ind->ind > indent_stack->head->value) {         // 15
            // error, current indent between two indents in stack
            return mng_LA_err("Bad indent level.");

        } else if (ind->ind < indent_stack->head->value) {  // 2
            // multiple dedents, return dedent, next time go to T_INDENT after start
            // current_indent does not change !!!
            // save the current char in c->c
            ungetc(c->c, stdin);
            c->c = '\n';
            c->enable = 1;
            g_token_ptr->lexeme = T_DEDENT;
            return g_token_ptr;

        } else if (ind->ind == indent_stack->head->value) { // 10
            // one dedent, start with char in c->c
            // next time T_EOL
            c->enable = 1;
            ind->ind = 0;
            ind->enable = 0;
            g_token_ptr->lexeme = T_DEDENT;
            return g_token_ptr;
        }
    }
    error(L_A, ERR_INTERN, "");
    free_token(g_token_ptr);
    return NULL;
}

token mng_eof(){
    static int first_time = 1;

    if(first_time){
        g_token_ptr->lexeme = T_EOF;
        first_time = 0;
        return g_token_ptr;
    } else {
        free(g_token_ptr);
        return NULL;
    }
}

/*
****************************************
                Scanner
****************************************
*/

// Allocation of all global and dynamic variables, setting their init values
// by error function returns -1 and scanner_destroy() must be called
// otherwise returns 0
int scanner_init() {
    int success;

    // string, input from stdin
    lex_str = str_init();
    if (lex_str == NULL) {
        error(L_A, ERR_INTERN, "Can not init string.");
        return -1;
    }

    // stack of indents (0, 3, 10, 15, ...)
    indent_stack = malloc(sizeof(d_stack));
    if (indent_stack == NULL) {
        error(L_A, ERR_INTERN, "Can not alloc stack.");
        return -1;
    }
    success = ds_init(indent_stack);
    if (success == -1) {
        error(L_A, ERR_INTERN, "Can not init stack.");
        return -1;
    }
    success = ds_push(indent_stack, 0);
    if (success == -1) {
        error(L_A, ERR_INTERN, "Can not push stack.");
        return -1;
    }

    // current char
    c = malloc(sizeof(e_char));
    if (c == NULL) {
        error(L_A, ERR_INTERN, "Can not malloc e_char.");
        return -1;
    } else {
        // get rid off EOL at begin of file
        do {
            c->c = getchar();
        } while ((char) c->c == '\n');
        ungetc(c->c, stdin);
        c->c = '\n';
        c->enable = 1;
    }

    // current indent
    ind = malloc(sizeof(e_ind));
    if (c == NULL) {
        error(L_A, ERR_INTERN, "Can not malloc e_ind.");
        return -1;
    } else {
        ind->ind = 0;
        ind->enable = 1;
    }
    return 0;
}

// function frees all allocated memory
// must be called always before end of program
int scanner_destroy() {
    if (lex_str == NULL)
        warning(L_A, "Can not destroy lex_str. Pointer to NULL.");
    else
        str_destroy(lex_str);


    if (indent_stack == NULL)
        warning(L_A, "Can not destroy INDENT. Pointer to NULL.");
    else {
        ds_destroy(indent_stack);
        free(indent_stack);
    }

    if (c == NULL)
        warning(L_A, "Can not free c. Pointer to NULL");
    else
        free(c);

    if (ind == NULL)
        warning(L_A, "Can not destroy IND. Pointer to NULL.");
    else
        free(ind);
    return 0;
}

// function returns next token
// if any intern error appears, returns NULL
// if any lexical error appears, returns token T_ERR_UNKNOWN
// in any case of error, err_flag is set, error msg is printed
// and scanner_destroy() MUST be called to free allocated memory
token get_token() {
    token token_ptr = malloc(sizeof(struct token));
    if (token_ptr == NULL) {
        error(L_A, ERR_INTERN, "Can not allocate token.");
        return NULL;
    }
    g_token_ptr = token_ptr;

    // take last char or new one
    if (!c->enable)
        c->c = getchar();

    str_clear(lex_str, 0);
    state = OS_START;

    while (1) {
        //debug("STATE: %d, char: %d", state, c->c);
        switch (state) {
            case OS_START:
                str_clear(lex_str, 0);
                // eof
                if (c->c == EOF) return mng_eof();
                    // first diagram
                else if (c->c == '=') mng_os(T_ASSIGNMENT, 0);
                else if (c->c == '!') mng_os(OS_NOT, 0);
                else if (c->c == '>') mng_os(T_GT, 0);
                else if (c->c == '<') mng_os(T_LT, 0);
                else if (c->c == '+') return mng_end_fs(T_PLUS, 0);
                else if (c->c == '-') return mng_end_fs(T_MINUS, 0);
                else if (c->c == '*') return mng_end_fs(T_MUL, 0);
                else if (c->c == '/') mng_os(T_DIVISION, 0);
                else if (c->c == ':') return mng_end_fs(T_COLON, 0);
                else if (c->c == ')') return mng_end_fs(T_R_BRACKET, 0);
                else if (c->c == '(') return mng_end_fs(T_L_BRACKET, 0);
                else if (c->c == ',') return mng_end_fs(T_COMMA, 0);
                else if (c_is_letter((char) c->c) || c->c == '_') mng_os(T_ID, 1);
                    // second diagram
                else if (c->c == '0') mng_os(FS_ZERO, 1);
                else if (c->c >= '1' && c->c <= '9') mng_os(T_INT, 1);
                else if (c->c >= '.') mng_os(OS_DOT, 1);
                    // third diagram
                else if (c->c == '\'') mng_os(OS_APOSTROPHE, 0);
                    // fourth diagram
                else if (c->c == '"') mng_os(OS_D_APO_1, 0);
                else if (c->c == '#') mng_os(OS_LINE_COMM, 0);
                else if (c_is_tab_space((char) c->c));// START again
                else if (c->c == '\n') mng_os(T_EOL, 0);

                else return mng_LA_err("");
                break;

//********************************     FIRST DIAGRAM   *****************************
            case T_ASSIGNMENT:
                if (c->c == '=') return mng_end_fs(T_EQUAL, 0);
                else return mng_end_fs(T_ASSIGNMENT, 1);

            case OS_NOT:
                if (c->c == '=') return mng_end_fs(T_N_EQUAL, 0);
                else return mng_LA_err("Did you forget '=' ?");

            case T_GT:
                if (c->c == '=') return mng_end_fs(T_GE, 0);
                else return mng_end_fs(T_GT, 1);

            case T_LT:
                if (c->c == '=') return mng_end_fs(T_LE, 0);
                else return mng_end_fs(T_LT, 1);

            case T_DIVISION:
                if (c->c == '/') return mng_end_fs(T_DIVISION, 0);
                else return mng_end_fs(T_F_DIVISION, 1);

            case T_ID:
                if (c_is_ID((char) c->c) && c->c != EOF) str_cat_char(lex_str, (char) c->c);
                else return mng_ID();
                break;
//********************************     SECOND DIAGRAM  *****************************
//--------------------------------     double          -----------------------------
            case OS_DOT:
                if (c->c == EOF) mng_LA_err("After dot must follow digit.");
                if (c_is_digit((char) c->c)) mng_os(T_DOUBLE, 1);
                else return mng_LA_err("After dot must follow digit.");
                break;

            case T_DOUBLE:
                if (c_is_digit((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else if (c->c == '_') mng_os(OS_DOUBLE_UNDERSCORE, 0);
                else if (c->c == 'e' || c->c == 'E') mng_os(OS_DOUBLE_E, 1);
                else return mng_double();
                break;

            case OS_DOUBLE_UNDERSCORE:
                if (c_is_digit((char) c->c)) mng_os(T_DOUBLE, 1);
                else return mng_LA_err("After underscore in double must folow another digit.");
                break;

            case FS_DOUBLE_DOT:
                if (c_is_digit((char) c->c)) mng_os(T_DOUBLE, 1);
                else return mng_double();
                break;

            case OS_DOUBLE_E:
                if (c_is_digit((char) c->c)) mng_os(FS_DOUBLE_E_F, 1);
                else if (c->c == '+' || c->c == '-') mng_os(OS_DOUBLE_E_SIGN, 1);
                else return mng_LA_err("After e in double must follow digit.");
                break;

            case OS_DOUBLE_E_SIGN:
                if (c_is_digit((char) c->c)) mng_os(FS_DOUBLE_E_F, 1);
                else return mng_LA_err("After + in double must follow digit.");
                break;

            case FS_DOUBLE_E_F:
                if (c_is_digit((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else if (c->c == '_') mng_os(OS_DOUBLE_E_UNDERSCORE, 0);
                else return mng_double();
                break;

            case OS_DOUBLE_E_UNDERSCORE:
                if (c_is_digit((char) c->c)) mng_os(FS_DOUBLE_E_F, 1);
                else return mng_LA_err("After _ in double must follow digit.");
                break;
//--------------------------------     int             -----------------------------
            case T_INT:
                if (c_is_digit((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else if (c->c == '_') mng_os(OS_INT_UNDERSCORE, 0);
                else if (c->c == '.') mng_os(FS_DOUBLE_DOT, 1);
                else return mng_int(10);
                break;

            case OS_INT_UNDERSCORE:
                if (c_is_digit((char) c->c)) mng_os(T_INT, 0);
                else return mng_LA_err("After _ in int must follow digit.");
                break;

            case FS_ZERO:
                if (c->c == '.') mng_os(FS_DOUBLE_DOT, 1);
                else if (c->c == 'b' || c->c == 'B') mng_os(OS_BIN_PREF, 0);
                else if (c->c == 'o' || c->c == 'O') mng_os(OS_OKT_PREF, 0);
                else if (c->c == 'x' || c->c == 'X') mng_os(OS_HEX_PREF, 0);
                else return mng_int(10);
                break;
//--------------------------------     bin, okt, hex   -----------------------------
            case OS_BIN_PREF:
                if (c_is_B_digit((char) c->c)) mng_os(FS_BIN, 1);
                else if (c->c == '_') mng_os(OS_BIN_UNDERSCORE, 0);
                else return mng_LA_err("After prefix in int must follow digit.");
                break;

            case FS_BIN:
                if (c_is_B_digit((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else if (c->c == '_') mng_os(OS_BIN_UNDERSCORE, 0);
                else return mng_int(2);
                break;

            case OS_BIN_UNDERSCORE:
                if (c_is_B_digit((char) c->c)) mng_os(FS_BIN, 1);
                else return mng_LA_err("After _ in int must follow digit.");
                break;

            case OS_OKT_PREF:
                if (c_is_O_digit((char) c->c)) mng_os(FS_OKT, 1);
                else if (c->c == '_') mng_os(OS_OKT_UNDERSCORE, 0);
                else return mng_LA_err("After prefix in int must follow digit.");
                break;

            case FS_OKT:
                if (c_is_O_digit((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else if (c->c == '_') mng_os(OS_OKT_UNDERSCORE, 0);
                else return mng_int(8);
                break;

            case OS_OKT_UNDERSCORE:
                if (c_is_O_digit((char) c->c)) mng_os(FS_OKT, 1);
                else return mng_LA_err("After _ in int must follow digit.");
                break;

            case FS_HEX:
                if (c_is_O_digit((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else if (c->c == '_') mng_os(OS_HEX_UNDERSCORE, 0);
                else return mng_int(16);
                break;

            case OS_HEX_PREF:
                if (c_is_H_digit((char) c->c)) mng_os(FS_HEX, 1);
                else if (c->c == '_') mng_os(OS_HEX_UNDERSCORE, 0);
                else return mng_LA_err("After prefix in int must follow digit.");
                break;

            case OS_HEX_UNDERSCORE:
                if (c_is_H_digit((char) c->c)) mng_os(FS_HEX, 1);
                else return mng_LA_err("After _ in int must follow digit.");
                break;
//********************************     THIRD DIAGRAM   *****************************
//--------------------------------     string          -----------------------------
            case OS_APOSTROPHE:
                if (c->c == '\'') mng_os(T_STRING, 0);
                else if (c->c == '\\') mng_os(OS_SLASH, 1);
                else if (c_is_A31((char) c->c)) str_cat_char(lex_str, (char) c->c);
                else return mng_LA_err("Did you forget to end string with ' ?");
                break;

            case OS_SLASH:
                if (c->c == 'x') mng_os(OS_SLASH_X, 1);
                else if (c_is_esc_char((char) c->c)) mng_os(OS_SLASH_A, 1);
                else if (c_is_A31((char) c->c)) mng_os(OS_APOSTROPHE, 1);
                else return mng_LA_err("");
                break;

            case OS_SLASH_A:
                conv_esc(lex_str);
                if (c->c == '\'') mng_os(T_STRING, 0);
                else if (c->c == '\\') mng_os(OS_SLASH, 1);
                else if (c_is_A31((char) c->c)) mng_os(OS_APOSTROPHE, 1);
                else return mng_LA_err("");
                break;

            case OS_SLASH_X:
                if (c_is_H_digit((char) c->c)) mng_os(OS_SLASH_XA, 1);
                else return mng_LA_err("");
                break;

            case OS_SLASH_XA:
                if (c_is_H_digit((char) c->c)) mng_os(OS_SLASH_XAA, 1);
                else return mng_LA_err("");
                break;

            case OS_SLASH_XAA:
                conv_esc_hex(lex_str);
                if (c->c == '\'') mng_os(T_STRING, 0);
                else if (c->c == '\\') mng_os(OS_SLASH, 1);
                else if (c_is_A31((char) c->c)) mng_os(OS_APOSTROPHE, 1);
                else return mng_LA_err("");
                break;

            case T_STRING:
                return mng_str();

//********************************     FOURTH DIAGRAM  *****************************
//--------------------------------     block comment   -----------------------------
            case OS_D_APO_1:
                if (c->c == '"') mng_os(OS_D_APO_2, 0);
                else return mng_LA_err("Did you forget \" ?");
                break;

            case OS_D_APO_2:
                if (c->c == '"') mng_os(OS_DOC_STR, 0);
                else return mng_LA_err("Did you forget \" ?");
                break;

            case OS_DOC_STR:
                if (c->c == '"') mng_os(OS_DOC_STR_1, 1);
                else if (c->c == '\\') mng_os(OS_DOC_STR_SLASH, 1);
                else if (c->c == EOF) return mng_LA_err("You did not end docstring.");
                else str_cat_char(lex_str, (char) c->c);
                break;

            case OS_DOC_STR_SLASH:
                if (c->c == EOF) return mng_LA_err("You did not end docstring.");
                else if (c->c == '\"') mng_os(OS_DOC_STR_D_APO, 1);
                else mng_os(OS_DOC_STR, 1);
                break;

            case OS_DOC_STR_D_APO:
                conv_esc(lex_str);
                if (c->c == EOF) return mng_LA_err("You did not end docstring.");
                else if (c->c == '\"') mng_os(OS_DOC_STR_1, 1);
                else if (c->c == '\\') mng_os(OS_DOC_STR_SLASH, 1);
                else mng_os(OS_DOC_STR, 1);
                break;

            case OS_DOC_STR_1:
                if (c->c == EOF) return mng_LA_err("You did not end docstring.");
                else if (c->c == '"') mng_os(OS_DOC_STR_2, 1);
                else if (c->c == '\\') mng_os(OS_DOC_STR_SLASH, 1);
                else mng_os(OS_DOC_STR, 1);
                break;

            case OS_DOC_STR_2:
                if (c->c == EOF) return mng_LA_err("You did not end docstring.");
                else if (c->c == '\\') mng_os(OS_DOC_STR_SLASH, 1);
                else if (c->c == '"') {
                    str_del_chars(lex_str, 2);
                    mng_os(T_STRING, 0);
                } else mng_os(OS_DOC_STR, 1);
                break;

//--------------------------------     line comment    -----------------------------
            case OS_LINE_COMM:
                if (c->c == EOF) return mng_eof();
                else if (c->c == '\n') mng_os(T_EOL, 0);
                break;
//--------------------------------     INDENT, EOL, EOF ----------------------------
            case T_EOL:
                if (c->c == EOF) return mng_eof();
                else if (ind->enable) goto indent;
                else if (c->c == '\n');
                else {
                    ind->enable = 1;
                    ungetc(c->c, stdin);
                    c->c = '\n';
                    return mng_end_fs(T_EOL, 1);
                }
                break;

            case OS_IND_TAB_SPACE:
                if (c->c == EOF) return mng_eof();
                else if (c->c == '\n') mng_os(T_INDENT, 0);
                else if (c->c == '#') mng_os(OS_LINE_COMM, 0);
                else if (!c_is_tab_space((char) c->c))
                    return mng_LA_err("Space in indent can not be substituted by tab.");
                break;

            case T_INDENT:
            indent:
                if (c->c == EOF) return mng_eof();
                else if (c->c == '#') {
                    mng_os(OS_LINE_COMM, 0);
                    ind->ind = 0;
                } else if (c->c == '\n') ind->ind = 0;
                else if (c->c == ' ') ind->ind++;
                else if (c->c == '\t') {
                    ind->ind = 0;
                    mng_os(OS_IND_TAB_SPACE, 0);
                } else {  // some kind of indent

                    if (ind->ind == indent_stack->head->value) {
                        // the same indent => none indent, start again
                        ind->enable = 0; // OEL next time
                        ind->ind = 0;
                        ungetc(c->c, stdin);
                        state = OS_START;
                    } else return mng_ind();
                }
                break;
//---------------------------------------------------------------------
            default:
                debug("KONEEC. Tak presne sem se to dostat nemelo.");
                error(L_A, ERR_LEX_AN, "Unknown lexem.");
                free_token(g_token_ptr);
                return NULL;
        }
        c->c = getchar();
    }
}
