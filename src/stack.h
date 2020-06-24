/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     stack.h
    Autor:      Kateřina Mušková (xmusko00)
    Úpravy:     
*********************************************
*/

#ifndef _STACK_H
#define _STACK_H

#include <stdio.h>
#include <stdlib.h>

typedef struct d_elem {
    int value;
    struct d_elem* pre_elem_ptr;
} *d_elem_ptr;

typedef struct {
    d_elem_ptr head;
} d_stack;

int ds_init(d_stack *);

void ds_destroy(d_stack *);

int ds_empty(d_stack *);

int ds_len(d_stack *ds);

int ds_copy(d_stack *, int *);

int ds_copy_at(d_stack *, int, int *);

int ds_actualize(d_stack *, int);

int ds_push(d_stack *, int);

int ds_pop(d_stack *);

void ds_print(d_stack *);

#endif //_STACK_H
