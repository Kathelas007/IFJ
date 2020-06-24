/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     dynamic_string.h
    Autor:      Kateřina Mušková (xmusko00)
                David Holas      (xholas11)
    Úpravy:     Antonín Hubík    (xhubik03)
*********************************************
*/

#ifndef STRING_H
#define STRING_H

#define MEM_BLOCK_SIZE 32

#include <stdlib.h>

typedef struct {
    size_t len; // In Bytes, NOT including \0 !!
    char *str;
    size_t capacity; // In blocks !!
} string;


/// Init empty string
/// NULL on malloc fail
string* str_init();

/// Init string containing char array
/// NULL on malloc fail
string *str_init_chars(char *);

/// Init string from formatting char array.
/// See: snprintf
/// NULL on malloc fail
string *str_init_fmt(const char *fmt, ...);

/// Destroy string
/// 0 on success
/// -1 on invalid pointer
int str_destroy(string *);

/// Clears content of string, keeps capacity depending on second parameter
/// 0 on success
/// -1 on invalid pointer or malloc fail
int str_clear(string *, int);

/// 1 if string is empty
/// 0 if string is not empty
/// -1 on invalid pointer
int str_is_empty(string *);

/// Returns length of the string, NOT including \0
size_t str_len(string *);

/// Compares two strings
/// 1 if are identical
/// 0 if are not identical
/// -1 on invalid pointers
int str_cmp(string *, string *);

/// Compares string and char array
/// See: str_cmp
int str_cmp_chars(string *, const char *);

/// Copies 'src' into 'dest', content of 'dest' is overwritten
/// 0 on success
/// -1 on invalid pointers or malloc fail
int str_cpy(string *dest , string *src);

/// Copies char array into string, content of string is overwritten
/// See: str_cpy
int str_cpy_chars(string*, const char*);

///	Concatenates two strings, result is in 'dest'
/// 0 on success
/// -1 on invalid pointers or malloc fail
int str_cat(string *dest, string *src);

/// Adds char at the end of string
/// 0 on success
/// -1 on invalid pointer or malloc fail
int str_cat_char(string *, char);

/// Adds char array at the end of string
/// 0 on success
/// -1 on invalid pointer or malloc fail
int str_cat_chars(string *, const char *);

/// Deletes last char in string
/// 0 on success/zero length
/// -1 on invalid pointer or malloc fail
int str_del_char(string *);

/// Deletes number of last chars in string
///	If length of string is smaller than number of chars to be deleted, then deletes all
/// 0 on success
/// -1 on invalid pointer or malloc fail
int str_del_chars(string *, int);

/// Gets last char in string
/// char pointer will be pointing at that char
/// 0 on success
/// -1 on invalid pointers
int str_get_last(string *, char *);

/// Gets char at index in string
/// char pointer will be pointing at that char
/// 0 on success
/// -1 on invalid pointers or index out of bounds 
int str_get_at(string *, unsigned long, char *);

/// Mallocs new char array from string
/// Returned array must be freed!!
/// NULL on malloc fail
char *str_as_chars(string *);

/// Prints just string content into given file
/// 0 on success
/// -1 on invalid pointers
int str_print(FILE *, string *);

/// Prints debug information about string
/// 0 on success
/// -1 on invalid pointers
int str_debug(string *);

int str_to_int(string *, int *, int);

int str_to_dbl(string *, double *);

int str_to_upper(string *);

#endif //_STRING_H
