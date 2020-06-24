/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     token.h
    Autor:      David Holas      (xholas11)
    Úpravy:     Kateřina Mušková (xmusko00)
*********************************************
*/

#ifndef TOKEN_H
#define TOKEN_H

#include "symtable.h"

// final states
enum lexeme {
    // Control flow
    T_DEF,
    T_IF,
    T_ELSE,
    T_WHILE,
    T_RETURN,
    T_INDENT,
    T_DEDENT,
    T_PASS,

    // Build-in fnc
    T_INPUT_S,
    T_INPUT_I,
    T_INPUT_F,
    T_PRINT,
    T_LEN,
    T_SUBSTR,
    T_ORD,
    T_CHR,

    // ID
    T_ID,       // Do not reorder section begin

    //Expression
    T_NONE,
    T_INT,
    T_STRING,
    T_DOUBLE,

    // Arithmetic operations
    T_PLUS,
    T_MINUS,
    T_MUL,
    T_DIVISION,    //      division for both int and float arguments
    T_F_DIVISION,  //FLOOR division for both int and float arguments

    // Relational Operators
    T_GE,
    T_GT,
    T_LE,
    T_LT,
    T_EQUAL,
    T_N_EQUAL,

    // Brackets
    T_L_BRACKET,
    T_R_BRACKET,    // Do not reorder section end

    // Assignment
    T_ASSIGNMENT,
    // Others
    T_COLON,
    T_COMMA,
    T_EOF,
    T_EOL,
    // Error
    T_LEX_UNKNOWN // toto prosim at zustane jako posledni
};

typedef struct token {
    enum lexeme lexeme;
    union value {
        string* string_struct;
        string* id_key;
        int integer;
        double floating_point;
    } value;
} *token;

#define free_token(t) do{ \
    if (t->lexeme == T_ID) { \
        str_destroy(t->value.id_key); \
    } else if (t->lexeme == T_STRING) { \
        str_destroy(t->value.string_struct); \
    } \
    free(t); \
} while(0)


#endif
