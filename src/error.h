/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     error.h
    Autor:      Kateřina Mušková (xmusko00)
    Úpravy:     David Holas      (xholas11)
*********************************************
*/

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include <stdio.h>

#define OK              0
#define ERR_LEX_AN      1   //lexical analysis
#define ERR_SYN_AN      2   //syntax analysis
#define ERR_SEM_DEF     3   //semantic analysis - undefined/redefining fnc, var
#define ERR_SEM_RUN     4   //semantic analysis - type compatibility in expressions
#define ERR_SEM_PAR     5   //semantic analysis - wrong number of params
#define ERR_SEM_OTH     6   //semantic analysis - other errors
#define ERR_ZERO_DIV    9   //dividing by zero
#define ERR_INTERN      99  //intern error


extern int ERR_FLAG;

#ifdef DEBUG

/// Sets ERR_FLAG and prints error, if DEBUG then prints filename:line:function as well
#define error(module_name, err_flag, fmt, ...) do{ \
	if(ERR_FLAG == OK) { \
		ERR_FLAG = err_flag; \
		fprintf(stderr, "ERROR %d: %s:%d:%s: " fmt "\n", err_flag, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} \
} while(0)

#define warning(module_name, fmt, ...) \
	fprintf(stderr, "WARNING: %s:%d:%s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


#define debug(fmt, ...) \
	fprintf(stderr, "DEBUG: %s: " fmt "\n", __FILE__, ##__VA_ARGS__)
	
#else

#define error(module_name, err_flag, fmt, ...) do {\
	if(ERR_FLAG == OK) { \
		ERR_FLAG = err_flag; \
		fprintf(stderr, "ERROR %d: %s: " fmt "\n", err_flag, module_name, ##__VA_ARGS__); \
	} \
} while(0)

#define warning(module_name, fmt, ...) \
	fprintf(stderr, "WARNING: %s: " fmt "\n", module_name, ##__VA_ARGS__)

#define debug(fmt, ...)

#endif // DEBUG

#endif // ERROR_H
