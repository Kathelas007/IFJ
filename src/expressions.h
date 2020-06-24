/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     expressions.h
    Autor:      David Holas      (xholas11)
    Úpravy:     
*********************************************
*/


#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "token.h"
#include "symtable.h"

/// Generates IFJcode19 for expression.
///
/// It starts with 'first', eventually 'second' token if not NULL.
/// NULL 'global_tab', 'local_tab' or 'tmp_tab' causes ERR_INTERN
/// 'repeat' will point (if not NULL) to first token after expression (T_COLON, T_EOL, T_EOF).
/// 'result_type' will point (if not NULL) to data_type of final expression.
/// Returns 0 on success, otherwise -1 and sets ERR_FLAG.
int eval_expression(symtab *global_tab, symtab *local_tab, symtab *tmp_tab, token first, token second, token *repeat, data_type *result_type);

#endif // EXPRESSIONS_H
