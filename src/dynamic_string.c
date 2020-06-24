/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     dynamic_string.c
    Autor:      Kateřina Mušková (xmusko00)
    Úpravy:     
*********************************************
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "dynamic_string.h"

#include "error.h"

#define D_S "Dynamic String"

int str_realloc(string *s, size_t size_to_fit) {
    if(s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    if((s->capacity * MEM_BLOCK_SIZE) - size_to_fit < MEM_BLOCK_SIZE) {
        return 0;
    }

    //debug("Realloc %lu -> %lu:", s->capacity, (size_to_fit-1)/MEM_BLOCK_SIZE + 1);
    size_t cap = (size_to_fit-1)/MEM_BLOCK_SIZE + 1;
    char *new = realloc(s->str, cap * MEM_BLOCK_SIZE);
    if(new == NULL) {
        error(D_S, ERR_INTERN, "Reallocation failed");
        return -1;
    }
    s->str = new;
    s->capacity = cap;

    return 0;
}

string *str_init() {
    string *s = malloc(sizeof(string));
    if (s == NULL) {
        error(D_S, ERR_INTERN, "Malloc failed");
        return NULL;
    }

    s->str = malloc(MEM_BLOCK_SIZE * sizeof(char));
    if (s->str == NULL) {
        error(D_S, ERR_INTERN, "Malloc failed");
        free(s);
        return NULL;
    }

    s->len = 0;
    s->capacity = 1;
    s->str[0] = '\0';

    return s;
}

string *str_init_chars(char *chars) {
    if(chars == NULL) {
        return str_init();
    }

    string *s = malloc(sizeof(string));
    if (s == NULL) {
        error(D_S, ERR_INTERN, "Malloc failed");
        return NULL;
    }

    unsigned long len = strlen(chars);

    s->capacity = len/MEM_BLOCK_SIZE + 1;
    s->str = malloc(s->capacity * MEM_BLOCK_SIZE);
    if (s->str == NULL) {
        error(D_S, ERR_INTERN, "Malloc failed");
        free(s);
        return NULL;
    }

    s->len = len;
    strcpy(s->str, chars);
    return s;
}

string *str_init_fmt(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t size = vsnprintf(NULL, 0, fmt, args) + 1; // Trailing \0
    va_end(args);

    char *temp = malloc(size);
    if(temp == NULL) {
        return str_init();
    }

    va_start(args, fmt);
    vsprintf(temp, fmt, args);
    va_end(args);

    string *out = str_init_chars(temp);
    free(temp);
    return out;
}

int str_destroy(string *s) {
    if (s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1; //means that pointer was already freed
    }

    if(s->str != NULL) {
        free(s->str);
    }
    free(s);
    s = NULL;
    return 0;
}

int str_clear(string *s, int keep_capacity) {
    if (s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    if(!keep_capacity) {    
        if(str_realloc(s, 1) == -1) {
            return -1;
        }
    }

    s->str[0] = '\0';
    s->len = 0;

    return 0;
}

int str_is_empty(string *s) {
    if (s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }
    if (s->len <= 0)
        return 1;
    return 0;
}

size_t str_len(string *s) {
    if(s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    return s->len;
}

int str_cmp(string *s1, string *s2) {
    if (s1 == NULL || s2 == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    if(strcmp(s1->str, s2->str)) {
        return 0;
    }
    return 1;
}

int str_cmp_chars(string *s, const char *a) {
    if(s == NULL || a == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    if(strcmp(s->str, a)) {
        return 0;
    }
    return 1;
}

int str_cpy(string *dest, string *src) {
    if (dest == NULL || src == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    if(str_realloc(dest, src->len + 1) == -1)  {
        return -1;
    }
    
    strcpy(dest->str, src->str);

    dest->len = src->len;
    
    return 0;
}

int str_cpy_chars(string *dest_s, const char* src_cs){
    if(dest_s == NULL || src_cs == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    size_t len = strlen(src_cs);
    if(str_realloc(dest_s, len + 1) == -1)  {
        return -1;
    }
    dest_s->len = len;
    
    strcpy(dest_s->str, src_cs);

    return 0;
}

int str_cat(string *dest, string *src) {
    if (dest == NULL || src == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    dest->len += src->len;
    if(str_realloc(dest, dest->len + 1) == -1) {
        dest->len -= src->len;   
        return -1;
    }

    strcat(dest->str, src->str);
    return 0;
}

int str_cat_char(string *s, char c) {
    if (s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    (s->len)++;
    if(str_realloc(s, s->len + 1) == -1) {
        (s->len)--;
        return -1;
    }

    s->str[s->len - 1] = c;
    s->str[s->len] = '\0';

    return 0;
}

int str_cat_chars(string *s, const char *chars) {
    if(s == NULL || chars == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    size_t len = s->len + strlen(chars);
    if(str_realloc(s, len + 1) == -1) {
        return -1;
    }
    s->len = len;

    strcat(s->str, chars);

    return 0;
}

int str_del_char(string *s) {
    if (s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    if (s->len <= 0)
        return 0;

    if(str_realloc(s, s->len) == -1) {
        return -1;
    } 

    s->len--;
    s->str[s->len] = '\0';

    return 0;
}

int str_del_chars(string *s, int num) {
    if (s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    s->len -= num;
    if(s->len <= 0) {
        return str_clear(s, 0);
    }

    memset(s->str + s->len, 0, num);
    
    if(str_realloc(s, s->len + 1) == -1) {
        s->len += num;
        return -1;
    }

    return 0;
}

int str_get_last(string *s, char *c) {
    if (s == NULL || c == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    *c = s->str[s->len - 1];
    return 0;
}

int str_get_at(string *s, size_t i, char *c) {
    if (s == NULL || i >= s->len || c == NULL) {
        warning(D_S, "NULL pointer passed or index out of bounds");
        return -1;
    }

    *c = s->str[i];
    return 0;
}

char *str_as_chars(string *s) {
    if(s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return NULL;
    }

    char *out = malloc(s->len + 1);
    if(out == NULL) {
        error(D_S, ERR_INTERN, "Malloc failed");
        return NULL;
    }

    strcpy(out, s->str);
    return out;
}

int str_print(FILE *file, string *s) {
    if(file == NULL || s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }
    fprintf(file, "%s", s->str);
    return 0;
}


int str_debug(string *s) {
    if(s == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    debug("\nSTRING: \nStr: %s\nLen: %lu\nCap: %lu", s->str, s->len, s->capacity);
    return 0;
}

int str_to_int(string *s, int *num, int base) {
    if(s == NULL || num == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    char *p_end = NULL;
    *num = (int) strtol(s->str, &p_end, base);
    if (p_end == s->str || *p_end != 0) {
        return -1;
    }
    return 0;
}

int str_to_dbl(string *s, double *num) {
    if(s == NULL || num == NULL) {
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }

    char *p_end = NULL;
    *num = strtod(s->str, &p_end);
    if (p_end == s->str || *p_end != 0) {
        return -1;
    }
    return 0;
}

int str_to_upper(string*s){
    if(s == NULL){
        warning(D_S, "NULL pointer passed to %s", __func__);
        return -1;
    }
    for(unsigned long i = 0; i<s->len; i++){
        if(s->str[i] >= 'a' && s->str[i] <= 'z'){
            s->str[i] = (s->str[i]) - 32;
        }
    }
    return 0;
}