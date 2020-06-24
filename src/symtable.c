/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     symtable.c
    Autor:      Antonín Hubík	 (xhubik03)
    Úpravy:     
*********************************************
*/

#include <stdio.h>

#include "symtable.h"

#define SYMT "Table of symbols"

//Implementation of BKDR hash function, might change
unsigned long symtab_hash_fn(string *id){
	
	unsigned long seed = 131;
	unsigned long hash = 0;

	unsigned long strleng = str_len(id);
	for(unsigned long i=0; i<strleng; i++){
		hash = (hash * seed) + id->str[i];
	}
	return hash;
}

//allocates memory for variable attributes
fun_pars* fun_pars_init(unsigned par_arr_s){
	fun_pars* f_pars = malloc(sizeof(fun_pars) + sizeof(fun_par[par_arr_s]));
	if(f_pars == NULL){
		error(SYMT, ERR_INTERN, "Malloc failed");
		return NULL;
	}

	f_pars->par_arr_s = par_arr_s;

	for(unsigned i=0; i<par_arr_s; i++){
		f_pars->par_arr[i].par_id = NULL;
		f_pars->par_arr[i].par_type = DT_NONE;
	}

	return f_pars;
}

//frees memory allocated by function parameters
void fun_pars_destroy(fun_pars* f_pars){
	if(f_pars == NULL){
		error(SYMT, ERR_INTERN, "Malloc failed");
		return;
	}

	for (unsigned i=0; i<f_pars->par_arr_s; i++){
		str_destroy(f_pars->par_arr[i].par_id);
	}

	free(f_pars);
}

//adds attributes to a stored variable and returns 0, else returns -1
int add_var_at(symt_item* item, unsigned is_init, data_type data_t){
	if (item == NULL || item->iden_t == ID_F)
		return -1;

	if (item->iden_t == ID_U){
		item->iden_t = ID_V;
	}

    //considers that if item stores variable data, it was init by this function
	item->is_init_def = is_init;
	item->data_ret_t = data_t;
	return 0;
}

//adds attributes to a stored function and returns 0, else returns -1
int add_fun_at(symt_item* item, unsigned is_def, data_type ret_t, unsigned par_arr_s, fun_par* par_arr){

	if (item == NULL || item->iden_t == ID_V)
		return -1;

	if (item->iden_t == ID_U){
		if (item->pars != NULL){
			return -1;
		}

		if (par_arr_s > 0 && par_arr == NULL){
			return -1;
		}
		item->iden_t = ID_F;
	}

	int s = set_args(item, par_arr_s, par_arr);
	if (s != 0)
		return s;

	item->is_init_def = is_def;
	item->data_ret_t = ret_t;

   	return 0;
}

int set_is_init(symt_item* item, unsigned is_init){

	if (item == NULL || item->iden_t == ID_F){
		return -1;
	}

	if (item->iden_t == ID_U){
		item->iden_t = ID_V;
	}

	item->is_init_def = is_init;
	return 0;
}

int set_var_t(symt_item* item, data_type data_t){

	if (item == NULL || item->iden_t == ID_F){
		return -1;
	}

	if (item->iden_t == ID_U){
		item->iden_t = ID_V;
	}

	item->data_ret_t = data_t;
	return 0;
}

int get_is_init(symt_item* item, unsigned* is_init){

	if (item == NULL || item->iden_t != ID_V || is_init == NULL)
		return -1;

	*is_init = item->is_init_def;
	return 0;
}

int get_var_t(symt_item *item, data_type* data_t){

	if (item == NULL || item->iden_t != ID_V || data_t == NULL)
		return -1;

	*data_t = item->data_ret_t;
	return 0;
}

int set_is_def(symt_item* item, unsigned is_def){

	if (item == NULL || item->iden_t == ID_V){
		return -1;
	}

	if (item->iden_t == ID_U){
		item->iden_t = ID_F;
	}

	item->is_init_def = is_def;
	return 0;
}

int set_ret_t(symt_item* item, data_type ret_t){

	if (item == NULL || item->iden_t == ID_V){
		return -1;
	}

	if (item->iden_t == ID_U){
		item->iden_t = ID_F;
	}

	item->data_ret_t = ret_t;
	return 0;
}

int set_args(symt_item* item, unsigned arg_cnt, fun_par* par_arr){

	if (item == NULL || item->iden_t == ID_V){
		return -1;
	}

	if (item->iden_t == ID_U){
		item->iden_t = ID_F;
	}

	if (item->pars == NULL){
		item->pars = fun_pars_init(arg_cnt);
		if (item->pars == NULL){
			error(SYMT, ERR_INTERN, "Malloc failed");
			return -1;
		}
		item->pars->par_arr_s = arg_cnt;
	}

	else if (item->pars->par_arr_s < arg_cnt){
		fun_pars* new_pars = realloc(item->pars, sizeof(fun_pars) + sizeof(fun_par[arg_cnt]));
		if(new_pars == NULL){
			error(SYMT, ERR_INTERN, "Realloc failed");
			return -1;
		}
		item->pars = new_pars;
		item->pars->par_arr_s = arg_cnt;

		for(unsigned i=0; i<arg_cnt; i++){
		item->pars->par_arr[i].par_id = NULL;
		item->pars->par_arr[i].par_type = DT_NONE;
		}
	}

	for (unsigned i = 0; i < arg_cnt; i++){
		if (item->pars->par_arr[i].par_id == NULL){
			item->pars->par_arr[i].par_id = str_init();
		}

		if (item->pars->par_arr[i].par_id == NULL){
			for(unsigned j=0; j<i; j++){
				str_destroy(item->pars->par_arr[i].par_id);
				item->pars->par_arr[i].par_type = DT_NONE;
			}
			return -1;
		}

		str_cpy(item->pars->par_arr[i].par_id, par_arr[i].par_id);
		item->pars->par_arr[i].par_type = par_arr[i].par_type;

	}
	return 0;
}

int get_is_def(symt_item* item, unsigned* is_def){

	if (item == NULL || item->iden_t != ID_F || is_def == NULL)
		return -1;

	*is_def = item->is_init_def;
	return 0;
}

int get_ret_t(symt_item *item, data_type* ret_t){

	if (item == NULL || item->iden_t != ID_F || ret_t == NULL)
		return -1;

	*ret_t = item->data_ret_t;
	return 0;
}

//get number of arguments in a function-type item
unsigned get_args_cnt(symt_item *item){

	if (item == NULL || item->pars == NULL)
		return 0;

	if (item->iden_t != ID_F){
		return 0;
	}

	return item->pars->par_arr_s;
}

//returns number of arguments copied to array of strings passed via arg 2
//par_arr must be initialized in some way, otherwise this won't work
unsigned get_args(symt_item* item, unsigned pars_cnt, fun_par* par_arr){

	if (item == NULL || par_arr == NULL){
		return 0;
	}

	if (item->iden_t != ID_F || item->pars == NULL){
		return 0;
	}
	
	unsigned i;
	for (i =0; i<pars_cnt; i++){
		if (par_arr[i].par_id ==  NULL){
			par_arr[i].par_id = str_init();
			if (par_arr[i].par_id == NULL){
				error(SYMT, ERR_INTERN, "Malloc failed");
				return i;
			}
		}

		str_cpy(par_arr[i].par_id, item->pars->par_arr[i].par_id);
		par_arr[i].par_type = item->pars->par_arr[i].par_type;
	}
	return i;
}

//Allocates memory for table and sets inital values
symtab* symtab_init(unsigned long index_width){

	if (index_width == 0) return NULL;
	symtab* table;
	table = malloc(sizeof(unsigned long)*2 + sizeof(symt_item*[index_width]));
	if(table==NULL){
		error(SYMT, ERR_INTERN, "Malloc failed");
		return table;
	}

	table->size = 0;
	table->index_width = index_width;

	for (unsigned long i = 0; i < index_width; i++) {
		table->ptr_arr[i] = NULL;
	}

	return table;
}

//looks for an element in table
//if found returns pointer to it, if not, returns NULL
symt_item* symtab_find(symtab* table, string* id){

	if (id == NULL || table ==  NULL) 
		return NULL;

	unsigned long index = symtab_hash_fn(id) % table->index_width;
	symt_item* item = table->ptr_arr[index];

	while(item != NULL) {
		if(str_cmp(id, item->id) == 1){
			break;
		}
		item =  item->next;
	}
	return item;
}

//Looks for an item with a matching key within the table
//If none is found allocates space for a new entry and adds item
symt_item* symtab_find_insert(symtab* table, string* id){

	if (id == NULL || table ==  NULL){
		return NULL;
	}

	unsigned long index = symtab_hash_fn(id) % table->index_width;
	symt_item* item = table->ptr_arr[index];

	while(item != NULL && str_cmp(id, item->id) != 1) {

		if (item->next == NULL) break;
		item =  item->next;
	}

	if (item != NULL && item->next != NULL)
		return item;

	symt_item* new_item = malloc(sizeof(symt_item));
	if (new_item == NULL){
		error(SYMT, ERR_INTERN, "Malloc failed");
		return NULL;
	}

	new_item->id = str_init();
	if (new_item->id == NULL){
		free(new_item);
		error(SYMT, ERR_INTERN, "Malloc failed");
		return NULL;
	}

	if(str_cpy(new_item->id, id) != 0){

		str_destroy(new_item->id);
		free(new_item);
		error(SYMT, ERR_INTERN, "Malloc failed");
		return NULL;
	}

	new_item->iden_t = ID_U;
	new_item->data_ret_t = DT_UNDEF;
	new_item->pars = NULL;
	new_item->next = NULL;

	if (item == NULL)
		table->ptr_arr[index] = new_item;

	else
		item->next = new_item;
	
	table->size =  table->size + 1;
	return new_item;
}

//if item exists, returns 0, else returns -1
//if item exists, passes item type via arg 2
int get_item_type (symt_item* item, id_type* iden_t){

	if (item == NULL || iden_t == NULL){
		return -1;
	}

	*iden_t = item->iden_t;
	return 0;
}

//returns pointer to the first item in table
symt_item* symtab_first_item(symtab* table){

	if (table == NULL){
		return NULL;
	}

	for(unsigned long i=0; i<table->index_width; i++){
		if (table->ptr_arr[i] != NULL){
			return table->ptr_arr[i];
		}
	}

	return NULL;
}

//returns pointer to next item in table
symt_item* symtab_next_item(symtab* table, symt_item* item){

	if (table == NULL || item == NULL){
		return NULL;
	}

	if (item->next != NULL){
		return item->next;
	}

	unsigned long line = (symtab_hash_fn(item->id) % table->index_width) +1;
	
	for(unsigned long i=line; i<table->index_width; i++){
		if(table->ptr_arr[i] != NULL){
			return table->ptr_arr[i];
		}
	}
	
	return NULL;
}

//frees memory allocated by item, string in item and attributes in item
void symtab_delete_item(symt_item* item){
	if (item == NULL) {
		return;
	}

	str_destroy(item->id);

	if(item->iden_t == ID_F) {
		fun_pars_destroy(item->pars);
	}

	free(item);
}

//if an item is found, it is completely removed from table, uses symtab_delete_item()
int symtab_find_remove(symtab* table, string* id){
	if (id == NULL || table ==  NULL) 
		return -1;

	unsigned long index = symtab_hash_fn(id) % table->index_width;
	symt_item* store;

	if (str_cmp(id, table->ptr_arr[index]->id) != 1) {

		symt_item* item = table->ptr_arr[index];

		if (item == NULL) 
		return 0;

		while (item->next != NULL && str_cmp(id, item->next->id) != 1) {

			if (item->next->next == NULL) break;
			item =  item->next;
		}

		if (item->next == NULL || str_cmp(item->next->id, id) != 1)
			return 0;

		store = item->next->next;
		symtab_delete_item(item->next);
		item->next = store;
	}
	else {
		store = table->ptr_arr[index]->next;
		symtab_delete_item(table->ptr_arr[index]);
		table->ptr_arr[index] = store;
	}

	return 1;
}

//Returns number of items in table
//If table does not exist, there are no items => returns 0
unsigned long symtab_size(symtab* table){

 	if (table == NULL) return 0;
 	return table->size;
}

//Restores the table to its initial state, sets size back to 0
void symtab_reset(symtab* table){

	if (table == NULL) return;
	symt_item* store;

	for (unsigned long i = 0; i < table->index_width; i++) {
		while(table->ptr_arr[i]!=NULL){
			store = table->ptr_arr[i]->next;
			symtab_delete_item(table->ptr_arr[i]);
			table->ptr_arr[i] = store;
		}
		table->ptr_arr[i] = NULL;
	}
	table->size = 0;
}

//Frees all the memory allocated for the table, uses symtab_reset()
//sets pointer to table to NULL
void symtab_destroy(symtab* table){

	symtab_reset(table);
	free(table);
}