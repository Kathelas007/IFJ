/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     stack.c
    Autor:      Kateřina Mušková (xmusko00)
    Úpravy:     
*********************************************
*/

#include <stdlib.h>
#include "error.h"
#include "stack.h"

#define I_S "Int Stack"
#define CHECK_NULL(ds) if(ds == NULL) {warning(I_S, "%s", "Null pointer to stack."); return -1;}
#define CHECK_EMPTY(ds) if(ds->head == NULL) {warning(I_S, "%s", "Stack is empty,"); return -1;}

int ds_init(d_stack *ds) {
    CHECK_NULL(ds)

    ds->head = NULL;
    return 0;
}

void ds_destroy(d_stack *ds) {
    while (ds_empty(ds) == 0) {
        ds_pop(ds);
    }
}

// 0 - false
// 1 - true
// -1 - error
int ds_empty(d_stack *ds) {
    CHECK_NULL(ds)
    if (ds->head == NULL)
        return 1;
    return 0;
}


int ds_len(d_stack *ds) {
    CHECK_NULL(ds)
    d_elem_ptr p = ds->head;
    int i = 0;

    while (p != NULL) {
        i++;
        p = p->pre_elem_ptr;
    }
    return i;
}

// get last value
// -1 error
// 0 ok
int ds_copy(d_stack *ds, int *value) {
    CHECK_NULL(ds)
    if (value == NULL) {
        warning(I_S, "%s", "NULL pointer to value.");
        return -1;
    }

    CHECK_EMPTY(ds)
    *value = ds->head->value;
    return 0;
}

// Index 0 is top
int ds_copy_at(d_stack *ds, int index, int *value) {
    CHECK_NULL(ds)
    if (value == NULL) {
        return -1;
    }

    CHECK_EMPTY(ds);
    d_elem_ptr element = ds->head;

    int i = 0;
    while (i < index) {
        element = element->pre_elem_ptr;
        i++;
        if (element == NULL) {
            return -1;
        }
    }
    *value = element->value;
    return 0;
}

int ds_actualize(d_stack *ds, int value) {
    CHECK_NULL(ds)
    CHECK_EMPTY(ds);
    ds->head->value = value;
    return 0;
}

int ds_push(d_stack *ds, int value) {
    CHECK_NULL(ds)

    d_elem_ptr new = malloc(sizeof(struct d_elem));
    if (new == NULL){
        error(I_S, ERR_INTERN,  "%s", "Malloc failed.");
        return -1;
    }

    new->value = value;

    if (ds_empty(ds) == 1) {
        ds->head = new;
        new->pre_elem_ptr = NULL;
    } else {
        new->pre_elem_ptr = ds->head;
        ds->head = new;
    }
    return 0;
}

// delete value
int ds_pop(d_stack *ds) {
    CHECK_NULL(ds)
    if (ds_empty(ds) == 1)
        return 0;

    d_elem_ptr new_head = ds->head->pre_elem_ptr;
    free(ds->head);
    ds->head = new_head;
    return 0;
}

// print whole stack
// for debug
void ds_print(d_stack *ds) {
    if (ds == NULL)
        return;

    debug("STACK:");

    if (ds_empty(ds) == 1) {
        return;
    }

    d_elem_ptr actual = ds->head;
    while (actual != NULL) {
        debug("%d", actual->value);
        actual = actual->pre_elem_ptr;
    }
}
