/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)

    Soubor:     symtable.h
    Autor:      Antonín Hubík	 (xhubik03)
    Úpravy:
*********************************************
*/

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdio.h>
#include <stdlib.h>

#include "dynamic_string.h"
#include "error.h"

typedef enum data_type {
	DT_NONE,
    DT_INT,
    DT_STRING,
    DT_DOUBLE,
    DT_BOOL,
    DT_UNDEF
} data_type;

typedef enum id_type {
	ID_U,
	ID_V,
	ID_F
} id_type;

typedef struct fun_par {
	string *par_id;
	data_type par_type;
} fun_par;

typedef struct fun_pars {
	unsigned par_arr_s;
	fun_par par_arr[];
  } fun_pars;

typedef struct symt_item {
	string *id; //key string (name of identifier)
	id_type iden_t; //0, 1, 2
	unsigned is_init_def;
	data_type data_ret_t;
	fun_pars* pars;
	struct symt_item* next; //pointer to next item in list
} symt_item;

typedef struct symtab {
	unsigned long size; //number of elements in table
	unsigned long index_width; //number of elements in spinal array
	symt_item* ptr_arr[]; //array of pointers to beginnings of linear lists
} symtab;	

//if a variable is in table, insert its attributes, returns 0 on succes, otherwise -1
int add_var_at(symt_item* item, unsigned is_init, data_type data_t);

//if var item exists, sets it and returns 0, else returns -1
int set_is_init(symt_item* item, unsigned is_init);

//if var item exists, sets it and returns0, else returns -1
int set_var_t(symt_item* item, data_type data_t);

//if attribute exists, returns 0, else returns -1
//if attribute exists, passes 0 (not init) or 1 (init) via arg 2
int get_is_init(symt_item* item, unsigned* is_init);

//if attribute exists, returns 0, else returns -1
//if attribute exists, passes data type via arg 2
int get_var_t(symt_item *item, data_type* data_t);

//if a function is in table, insert its attributes, returns 0 on success, otherwise -1
int add_fun_at(symt_item* item, unsigned is_def, data_type ret_t, unsigned par_arr_s, fun_par* par_arr);

//if fun attribute exists, sets it and returns 0, else returns -1
int set_is_def(symt_item* item, unsigned is_def);

//if fun attribute exists, sets it and returns 0, else returns -1
int set_ret_t(symt_item* item, data_type ret_t);

//if item exists, adds attribute and returns 0, else returns -1
int set_args(symt_item* item, unsigned pars_cnt, fun_par* par_arr);

//if function exists, returns 0, else returns -1
//if function exists, passes 0 (not def) or 1 (def) via arg 2
int get_is_def(symt_item* item, unsigned* is_def);

//if function exists, returns 0, else returns -1
//if function exists, passes ret type via arg 2
int get_ret_t(symt_item* item, data_type* ret_t);

//returns number of arguments in function
// !!! If function does not exist, returns 0 as if there were no args in existing function !!!
unsigned get_args_cnt(symt_item *item);

//if attributes exist, returns number of loaded arguments (until first NULL pointer to string)
//if attributes exist, passes pointer their array via arg 3 (pointer to existing array of args)
//par_arr must be initialized in some way, otherwise this won't work
unsigned get_args(symt_item* item, unsigned arg_cnt, fun_par* par_arr);

//if item exists, returns 0, else returns -1
//if item exists, passes item type via arg 2
int get_item_type(symt_item* item, id_type* iden_t);

//takes size of indexing array, creates a table of symbols with explicit ordering
symtab* symtab_init(unsigned long index_width);

//if item with given key exists, finds it and returns pointer to it
symt_item* symtab_find(symtab* table, string* id);

//checks, whether key is already present, if not, inserts it, returns pointer to its element
symt_item* symtab_find_insert(symtab* table, string* key);

//returns pointer to the first item in table
symt_item* symtab_first_item(symtab* table);

//for iterating through table from given place
//returns pointer to next item in table
symt_item* symtab_next_item(symtab* table, symt_item* item);

//returns number of items in table
unsigned long symtab_size(symtab* table);

//removes an item with a given key, returns 0 if removed existing element
int symtab_find_remove(symtab* table, string* key);

//frees memory allocated by item, string in item and attributes in item
void symtab_delete_item(symt_item* item);

//resets a table to its initial state after symtab_init()
void symtab_reset(symtab* table);

//frees all memory allocated by the table passed through pointer in the argument
void symtab_destroy(symtab* table);

#endif