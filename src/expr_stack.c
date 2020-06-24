/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     expr_stack.c
    Autor:      David Holas      (xholas11)
    Úpravy:     
*********************************************
*/

#include <stdlib.h>
#include "error.h"
#include "expr_stack.h"

#define E_S "Expressions Stack"

wrapper wrap_token(token token) {
    wrapper out;
    out.token = token;
    out.expr = NULL;
    return out;
}

wrapper wrap_expr(struct expression *expr) {
    wrapper out;
    out.token = NULL;
    out.expr = expr;
    return out;
}

wrapper wrap_NULL() {
    wrapper out;
    out.token = NULL;
    out.expr = NULL;
    return out;
}

void free_wrapper(wrapper wrapper) {
    if(wrapper.token != NULL) {
        free_token(wrapper.token);
        wrapper.token = NULL;
    }
    if(wrapper.expr != NULL) {
        free(wrapper.expr);
        wrapper.expr = NULL;
    }
}

void es_init(expr_stack *ts) {
    ts->head = NULL;
}

void es_destroy(expr_stack *ts) {
    while (es_empty(ts) == 0){
        es_pop(ts);
    }
}

// 0 - false
// 1 - true
int es_empty(expr_stack *ts) {
    if (ts->head == NULL) {
        return 1;
    }
    return 0;
}

int es_copy(expr_stack *ts, wrapper *value) {
    if(ts->head == NULL) {
        warning(E_S, "Stack empty");
        return 0;
    }

    if(value != NULL) {
        *value = ts->head->value;
    }
    return 1;
}

// Index 0 is top
int es_copy_at(expr_stack *ts, int index, wrapper *value) {
    if(ts->head == NULL) {
        warning(E_S, "Stack empty");
        return 0;
    }

    d_expr_ptr element = ts->head;
    if(element == NULL) {
        error(E_S, ERR_INTERN, "Malloc failed");
        return 0;
    }

    int i = 0;
    while(i < index) {
        element = element->pre_elem_ptr;
        i++;
        if(element == NULL) {
            warning(E_S, "Index out of bounds");
            return 0;
        }
    }
    if(value != NULL) {
        *value = element->value;
    }
    return 1;
}

int es_push(expr_stack *ts, wrapper value) {
    d_expr_ptr new = malloc(sizeof(struct d_expr));
    if(new == NULL) {
        error(E_S, ERR_INTERN, "Malloc failed");
        return 0;
    }

    new->value = value;

    if (es_empty(ts) == 1){
        ts->head = new;
        new->pre_elem_ptr = NULL;
    } else {
        new->pre_elem_ptr = ts->head;
        ts->head = new;
    }
    return 1;
}

// Index 0 is top
int es_push_at(expr_stack *ts, int index, wrapper value) {
    d_expr_ptr new = malloc(sizeof(struct d_expr));
    if(new == NULL) {
        error(E_S, ERR_INTERN, "Malloc failed");
        return 0;
    }
    new->value = value;

    if(index == 0) {
        free(new);
        return es_push(ts, value);
    }

    d_expr_ptr element = ts->head;
    if(element == NULL) {
        warning(E_S, "Index out of bounds");
        return 0;
    }

    int i = 1;
    while(i < index) {
        element = element->pre_elem_ptr;
        i++;
        if(element == NULL) {
            warning(E_S, "Index out of bounds");
            return 0;
        }
    }

    new->pre_elem_ptr = element->pre_elem_ptr;
    element->pre_elem_ptr = new;
    return 1;
}

// delete value
int es_pop(expr_stack *ts) {
    if(ts->head == NULL) {
        warning(E_S, "Stack empty");
        return 0;
    }

    d_expr_ptr new_head = ts->head->pre_elem_ptr;
    free(ts->head);
    ts->head = new_head;
    return 1;
}

// print whole stack
// for debug
void es_print(expr_stack *ts){
    debug("STACK:");

    if(es_empty(ts) == 1) {
        return;
    }

    d_expr_ptr actual = ts->head;
    while (actual != NULL) {
        if(actual->value.token != NULL) {
            debug("%d", actual->value.token->lexeme);
        } else if(actual->value.expr != NULL) {
            debug("%d", ((token)(actual->value.expr))->lexeme);
        } else {
            debug("NULL");
        }
        actual = actual->pre_elem_ptr;
    }
}


