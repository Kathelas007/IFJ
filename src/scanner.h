/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     scanner.h
    Autor:      Kateřina Mušková (xmusko00)
    Úpravy:     
*********************************************
*/

#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"
#include "stack.h"

d_stack *indent_stack;

// first part is enum lexem in token.h
enum sec_part_state {
    // Final states
    FS_ZERO = T_LEX_UNKNOWN + 1,
    FS_BIN,
    FS_OKT,
    FS_HEX,

    FS_DOUBLE_DOT,
    FS_DOUBLE_E_F,

    // Non final states
    OS_START,
    OS_NOT,
    OS_INT_UNDERSCORE,
    OS_DOT,
    OS_DOUBLE_UNDERSCORE,

    OS_DOUBLE_E,
    OS_DOUBLE_E_SIGN,
    OS_DOUBLE_E_UNDERSCORE,

    OS_BIN_PREF,
    OS_OKT_PREF,
    OS_HEX_PREF,

    OS_BIN_UNDERSCORE,
    OS_OKT_UNDERSCORE,
    OS_HEX_UNDERSCORE,

    OS_APOSTROPHE,
    OS_SLASH,
    OS_SLASH_A,
    OS_SLASH_X,
    OS_SLASH_XA,
    OS_SLASH_XAA,

    OS_D_APO_1,
    OS_D_APO_2,

    OS_DOC_STR,
    OS_DOC_STR_1,
    OS_DOC_STR_2,
    OS_DOC_STR_SLASH,
    OS_DOC_STR_D_APO,

    OS_LINE_COMM,

    OS_IND_TAB_SPACE,
};


// after every reading a char is the char stored here
// enable - if should be by next calling used old char, or read new one
typedef struct {
    int c;
    int enable;
} e_char;

typedef struct {
    int ind;
    int enable;
}e_ind;

int scanner_init();

int scanner_destroy();

token get_token();


#endif //_SCANNER_H

