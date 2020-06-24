/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     expr_stack.h
    Autor:      David Holas      (xholas11)
    Úpravy:     
*********************************************
*/

#ifndef _EXPR_STACK_H
#define _EXPR_STACK_H

#include "token.h"

#define is_NULL(wrapper) \
	(wrapper.token == NULL && wrapper.expr == NULL)

struct expression;

typedef struct wrapper {
	token token;
	struct expression *expr;
} wrapper;

typedef struct d_expr {
    wrapper value;
    struct d_expr* pre_elem_ptr;
} *d_expr_ptr;

typedef struct d_expr_stack {
    d_expr_ptr head;
} expr_stack;

wrapper wrap_token(token);

wrapper wrap_expr(struct expression *);

wrapper wrap_NULL();

void free_wrapper();

void es_init(expr_stack *);

void es_destroy(expr_stack *);

int es_empty(expr_stack *);

int es_copy(expr_stack *, wrapper *);

int es_copy_at(expr_stack *, int, wrapper *);

int es_push(expr_stack *, wrapper);

int es_push_at(expr_stack *, int, wrapper);

int es_pop(expr_stack *);

void es_print(expr_stack *);

#endif // _EXPR_STACK_H
