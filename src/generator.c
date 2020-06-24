/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     generator.c
    Autor:      David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    Úpravy:     
*********************************************
*/

#include <stdio.h>
#include <stdlib.h>

#include "generator.h"

#include "dynamic_string.h"
#include "error.h"
#include "scanner.h"

// kontrola DT u BIF, ?? dasa
// u READ kontrola vstupu, uz opravili ifjcode
// zkontolovat gl x lc ID uziti
// zmenit formatovani u ifjcode


#define C_G "Code Generator"

#define FNC "$fnc_"
#define LAB_IF_END "$if_end_"
#define LAB_ELSE "$else_"
#define LAB_WHILE "$while_"
#define LAB_WHILE_END "$while_end_"
#define LAB_TYPE_CHECK "$type_check_"
#define LAB_EXPR "$expr_"

#define LAB_ERR_SEM_DEF "$$ERR_SEM_DEF"
#define LAB_ERR_SEM_RUN "$$ERR_SEM_RUN"
#define LAB_ERR_ZERO_DIV "$$ERR_ZERO_DIV"

#define LF "LF@"
#define GF "GF@"

#define str_cat_nl(str) str_cat_char(str, '\n')


/*
****************************************
                Buffer
****************************************
*/

typedef struct buffer_item {
    string *content;
    int frame_beginnig;
    struct buffer_item *prev;
    struct buffer_item *next;
} *buffer_item;

typedef struct buffer {
    buffer_item front;
    buffer_item rear;
} buffer;


static buffer main_buffer;
static buffer func_buffer;
static buffer param_buffer;


/// Inits doubly linked buffer
void buffer_init(buffer *b) {
    if (b == NULL) {
        return;
    }

    b->front = NULL;
    b->rear = NULL;
}

/// Destroys doubly linked buffer
void buffer_destroy(buffer *b) {
    if (b == NULL) {
        return;
    }

    buffer_item i = b->front;
    while (i != NULL) {
        buffer_item victim = i;
        i = i->next;
        str_destroy(victim->content);
        free(victim);
    }
}

/// Prints and empties buffer
void buffer_clear_print(buffer *b) {
    if (b == NULL) {
        return;
    }

    buffer_item i = b->front;
    while (i != NULL) {
        buffer_item victim = i;
        i = i->next;

        str_print(stdout, victim->content);
        str_destroy(victim->content);
        free(victim);
    }
    buffer_init(b);
}

/// Empties buffer
void buffer_clear(buffer *b) {
    if (b == NULL) {
        return;
    }

    buffer_item i = b->front;
    while (i != NULL) {
        buffer_item victim = i;
        i = i->next;

        str_destroy(victim->content);
        free(victim);
    }
    buffer_init(b);
}

/// Fronts buffer
string *buffer_front(buffer *b) {
    if (b == NULL || b->front == NULL) {
        return NULL;
    }

    buffer_item i = b->front;
    b->front = i->next;

    string *s = str_init();
    str_cat(s, i->content);

    str_destroy(i->content);
    free(i);

    if (b->front == NULL) {
        buffer_init(b);
    } else {
        b->front->prev = NULL;
    }

    return s;
}

/// Inserts item at the end of buffer
/// To mark item as frame beginnig pass true
int buffer_insert(buffer *b, string *content, int frame_beginnig) {
    if (b == NULL) {
        str_destroy(content);
        return 0;
    }

    buffer_item new = malloc(sizeof(struct buffer_item));
    if (new == NULL) {
        str_destroy(content);
        error(C_G, ERR_INTERN, "Malloc failed");
        return 0;
    }
    //new->content = content;
    new->content = str_init();
    str_cpy(new->content, content);
    new->frame_beginnig = frame_beginnig;
    new->prev = NULL;
    new->next = NULL;
    str_destroy(content);

    if (b->rear == NULL) {
        b->rear = new;
        b->front = new;
    } else {
        b->rear->next = new;
        new->prev = b->rear;
        b->rear = new;
    }
    return 1;
}

/// Goes through buffer starting from the end
/// Inserts item directly behind (closer to rear) first found item marked as frame beginning
int buffer_insert_at_fb(buffer *b, string *content) {
    if (b == NULL) {
        str_destroy(content);
        return 0;
    }

    if (b->front == NULL && b->rear == NULL) {
        return buffer_insert(b, content, 0);
    }

    buffer_item new = malloc(sizeof(struct buffer_item));
    if (new == NULL) {
        str_destroy(content);
        error(C_G, ERR_INTERN, "Malloc failed");
        return 0;
    }
    //new->content = content;
    new->content = str_init();
    str_cpy(new->content, content);
    new->frame_beginnig = 0;
    new->prev = NULL;
    new->next = NULL;
    str_destroy(content);

    buffer_item *i = &(b->rear);

    while (*i != NULL) {
        if ((*i)->frame_beginnig) {
            new->prev = (*i);
            new->next = (*i)->next;
            (*i)->next = new;
            if (*i == b->rear) {
                b->rear = new;
            }
            return 1;
        }
        i = &((*i)->prev);
    }

    b->front->prev = new;
    new->next = b->front;
    b->front = new;

    return 1;
}

/// Debug prints buffer
void buffer_debug(buffer *b) {
    if (b == NULL) {
        return;
    }

    buffer_item i = b->front;
    while (i != NULL) {
        buffer_item victim = i;
        str_debug(victim->content);
        debug("Frame beginning: %d\n", victim->frame_beginnig);
        i = i->next;
    }
}


/*
****************************************
                Generator
****************************************
*/

d_stack if_while_stack;

code_location location;

int label_index;

/// Constant strings
string *tmp_1;
string *tmp_2;
string *tmp_3;
string *tmp_count;
string *tmp_cond ;
string *tmp_type;
string *tmp_len;
string *ret_val;
string *st;
string *s0;
string *s1;
string *f0;
string *strue;
string *sfalse;
string *snill;


/// ########### Help non-generationg functions ############

string *unique_label() {
    return str_init_fmt("%s%d", "$lab_", label_index++);
}


void insert_at_fb(string *content) {
    if (location.main == 1) {
        buffer_insert_at_fb(&main_buffer, content);
    } else {
        buffer_insert_at_fb(&func_buffer, content);
    }
}

void mng_location(enum new_location loc) {

    switch (loc) {
        case CL_FNC_BEG:
            location.main = 0;
            break;
        case CL_FNC_END:
            location.main = 1;
            break;
        case CL_IF_WHILE_BEG:
            ds_push(&if_while_stack, location.if_while);
            location.if_while++;
            break;
        case CL_IF_WHILE_END:
            ds_pop(&if_while_stack);
            break;
        default:
            error(C_G, ERR_INTERN, "Enum out of bounds");
    }
}

void insert(string *content) {
    if (location.main == 1) {
        buffer_insert(&main_buffer, content, 0);
    } else {
        buffer_insert(&func_buffer, content, 0);
    }
}


void set_frame_beginning() {
    string *tmp = str_init_chars("");
    if (location.main == 1) {
        buffer_insert(&main_buffer, tmp, 1);
    } else {
        buffer_insert(&func_buffer, tmp, 1);
    }

    string *nl = str_init_chars("\n\n");
    buffer_insert_at_fb(&main_buffer, nl);

    string *nl2 = str_init_chars("\n\n");
    buffer_insert_at_fb(&func_buffer, nl2);
}

string *make_constant(data_type type, string *literal) {
    string *out = str_init();
    switch (type) {
        case DT_NONE:
            str_cat_chars(out, "nil@");
            break;
        case DT_INT:
            str_cat_chars(out, "int@");
            break;
        case DT_STRING:
            str_cat_chars(out, "string@");
            break;
        case DT_DOUBLE:
            str_cat_chars(out, "float@");
            break;
        case DT_BOOL:
            str_cat_chars(out, "bool@");
            break;
        default:
            return out;
    }

    str_cat(out, literal);
    return out;
}

string *make_id(string *id_name, int is_global) {
    string *out = str_init();
    if (is_global) str_cat_chars(out, GF);
    else str_cat_chars(out, LF);
    str_cat(out, id_name);
    return out;
}


/// ########### Help generating functions ############

enum ns_op_type {
    NT_RETURN,

    NT_PUSHS,
    NT_POPS,
    NT_INPUTS,
    NT_INPUTI,
    NT_INPUTF,
    NT_WRITE,
    NT_DEFVAR,
    NT_CALL,

    NT_MOVE,
    NT_STRLEN,
    NT_TYPE,

    NT_ADD,
    NT_SUB,
    NT_CONCAT,
    NT_GETCHAR,
    NT_SETCHAR,
    NT_STRI2INT,
    NT_LT,
    NT_GT,
    NT_EQ
};

const char *ns_op_type_to_chars [] = {
        [NT_RETURN] = "RETURN",

        [NT_PUSHS] = "PUSHS",
        [NT_POPS] = "POPS",
        [NT_INPUTS] = "READ",
        [NT_INPUTI] = "READ",
        [NT_INPUTF] = "READ",
        [NT_WRITE] = "WRITE",
        [NT_DEFVAR] = "DEFVAR",
        [NT_CALL] = "CALL",

        [NT_MOVE] = "MOVE",
        [NT_STRLEN] = "STRLEN",
        [NT_TYPE] = "TYPE",

        [NT_ADD] = "ADD",
        [NT_SUB] = "SUBSTR",
        [NT_CONCAT] = "CONCAT",
        [NT_GETCHAR] = "GETCHAR",
        [NT_SETCHAR] = "SETCHAR",
        [NT_STRI2INT] = "STRI2INT",
        [NT_LT] = "LT",
        [NT_GT] = "GT",
        [NT_EQ] = "EQ",
};

// generate private nonstack operation
void gen_priv_ns_op(enum ns_op_type type, string* s01, string* s02, string* s03){
    string* out = str_init();

    str_cat_chars(out, ns_op_type_to_chars[type]);

    if(type >= NT_PUSHS){
        str_cat_chars(out, " ");
        if(type == NT_CALL) str_cat_chars(out, FNC);
        str_cat(out, s01);
    }

    if(type >= NT_MOVE){
        str_cat_chars(out, " ");
        str_cat(out, s02);
    }

    if(type >= NT_ADD){
        str_cat_chars(out, " ");
        str_cat(out, s03);
    }

    if(type == NT_INPUTS) str_cat_chars(out, " string");
    if(type == NT_INPUTF) str_cat_chars(out, " float");
    if(type == NT_INPUTI) str_cat_chars(out, " int");

    str_cat_nl(out);
    if (type != NT_DEFVAR) insert(out);
    else insert_at_fb(out);
}
enum s_op_type {
    ST_ADDS,
    ST_SUBS,
    ST_MULS,
    ST_DIVS,
    ST_IDIVS,

    ST_LTS,
    ST_GTS,
    ST_EQS,

    ST_ORS,
    ST_ANDS,
    ST_NOTS,
    ST_INT2FLOATS,
    ST_CLEARS
};

const char *s_op_type_to_chars [] = {
        [ST_ADDS] = "ADDS",
        [ST_SUBS] = "SUBS",
        [ST_MULS] = "MULS",
        [ST_DIVS] = "DIVS",
        [ST_IDIVS] = "IDIVS",

        [ST_LTS] = "LTS",
        [ST_GTS] = "GTS",
        [ST_EQS] = "EQS",

        [ST_ORS] = "ORS",
        [ST_ANDS] = "ANDS",
        [ST_NOTS] = "NOTS",

        [ST_INT2FLOATS] = "INT2FLOATS",

        [ST_CLEARS] = "CLEARS"
};

void gen_priv_s_op(enum s_op_type type){
    string* out = str_init();

    str_cat_chars(out, s_op_type_to_chars[type]);
    str_cat_nl(out);
    insert(out);
}

enum fr_op_type {
    FT_CREATE,
    FT_PUSH,
    FT_POP
};

void gen_priv_frame(enum fr_op_type type){
    string* out = str_init();

    switch (type){
        case FT_CREATE:
            str_cat_chars(out, "CREATEFRAME");
            break;
        case FT_PUSH:
            str_cat_chars(out, "PUSHFRAME");
            break;
        case FT_POP:
            str_cat_chars(out, "POPFRAME");
    }
    str_cat_nl(out);
    insert(out);
}

void gen_new_line(){
    string* out = str_init();
    str_cat_nl(out);
    insert(out);
}

enum comment_highlight {
    CH_EXPRESSION_1,
    CH_EXPRESSION_2,
    CH_EXPRESSION_3,
    CH_NONEXPRESION,
    CH_PROGRAM_FLOW,
    CH_FNC_BEG,
    CH_FNC_END
};

void gen_priv_comment(char* comment, int important){
    string* out = str_init();
    string* hashes = NULL;

    str_cat_nl(out);
    if (important <= CH_EXPRESSION_2) hashes = str_init_chars("#");
    if (important == CH_EXPRESSION_3) hashes = str_init_chars("##");
    if (important == CH_NONEXPRESION) hashes = str_init_chars("####");
    if (important == CH_PROGRAM_FLOW) hashes = str_init_chars("########");

    if(important == CH_FNC_BEG){
        str_cat_chars(out, "######## FNC_");
        str_cat_chars(out, comment);
        str_cat_chars(out, "##################");
    } else if (important == CH_FNC_END){
        str_cat_chars(out, "##################################\n\n");
    } else {
        str_cat(out, hashes);

        str_cat_char(out, ' ');
        str_cat_chars(out, comment);
        str_cat_char(out, ' ');
        str_cat(out, hashes);

        str_destroy(hashes);
    }
    str_cat_nl(out);
    insert(out);

}

void gen_priv_label(char* prefix, string* name){
    string* out = str_init();

    str_cat_chars(out, "LABEL ");
    str_cat_chars(out, prefix);
    if(name != NULL) str_cat(out, name);
    str_cat_nl(out);
    insert(out);
}

void gen_priv_jump(char* prefix, string* name){
    string* out = str_init();

    str_cat_chars(out, "JUMP ");
    str_cat_chars(out, prefix);
    if(name != NULL) str_cat(out, name);

    str_cat_nl(out);
    insert(out);
}

void gen_priv_jump_if(int equal, char* prefix, string* name, string* op1, string* op2){
    string* out = str_init();

    if(equal) str_cat_chars(out, "JUMPIFEQ ");
    else str_cat_chars(out, "JUMPIFNEQ ");

    str_cat_chars(out, prefix);
    if(name != NULL) str_cat(out, name);
    str_cat_chars(out, " ");
    str_cat(out, op1);
    str_cat_chars(out, " ");
    str_cat(out, op2);

    str_cat_nl(out);
    insert(out);
}

void gen_priv_jump_if_s(int equal, char* prefix, string* name){
    string* out = str_init();

    if(equal) str_cat_chars(out, "JUMPIFEQS ");
    else str_cat_chars(out, "JUMPIFNEQS ");

    str_cat_chars(out, prefix);
    if(name != NULL) str_cat(out, name);

    str_cat_nl(out);
    insert(out);
}

void gen_priv_copy(enum func_target target, string *variable) {
    if(target == FT_BELOW) {
        gen_priv_ns_op(NT_POPS, tmp_3, NULL, NULL);
    }
    
    gen_priv_ns_op(NT_POPS, variable, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, variable, NULL, NULL);
    
    if(target == FT_BELOW) {
        gen_priv_ns_op(NT_PUSHS, tmp_3, NULL, NULL);
    }
}

/// ########### Handle generator ############

void generator_init() {
    buffer_init(&main_buffer);
    buffer_init(&func_buffer);
    buffer_init(&param_buffer);

    location.main = 1;
    location.if_while = 0;

    ds_init(&if_while_stack);

    label_index = 0;

    tmp_1 = str_init_chars("GF@$$tmp_1");
    tmp_2 = str_init_chars("GF@$$tmp_2");
    tmp_3 = str_init_chars("GF@$$tmp_3");
    tmp_count = str_init_chars("GF@$$tmp_count");
    tmp_type = str_init_chars("GF@$$tmp_type");
    tmp_len = str_init_chars("GF@$$tmp_len");
    tmp_cond = str_init_chars("GF@$$tmp_condition");
    ret_val = str_init_chars("GF@$$RETURN_VALUE");
    st = str_init_chars("string@");
    s0 = str_init_chars("int@0");
    s1 = str_init_chars("int@1");
    f0 = str_init_chars("float@0x0p+0");
    strue = str_init_chars("bool@true");
    sfalse = str_init_chars("bool@false");
    snill = str_init_chars("nil@nil");

}


void generator_destroy() {
    ds_destroy(&if_while_stack);

    buffer_destroy(&main_buffer);
    buffer_destroy(&func_buffer);
    buffer_destroy(&param_buffer);

    str_destroy(tmp_1);
    str_destroy(tmp_2);
    str_destroy(tmp_3);
    str_destroy(tmp_count);
    str_destroy(tmp_cond);
    str_destroy(tmp_type);
    str_destroy(tmp_len);
    str_destroy(ret_val);
    str_destroy(st);
    str_destroy(s0);
    str_destroy(s1);
    str_destroy(f0);
    str_destroy(strue);
    str_destroy(sfalse);
    str_destroy(snill);
}

/// ########### Nonexpressions ############

void gen_begin() {
    string *out = str_init();

    str_cat_chars(out, ".IFJcode19\n\n");
    insert(out);

    gen_new_line();
    set_frame_beginning();

    gen_priv_ns_op(NT_DEFVAR, tmp_cond, NULL, NULL);
    gen_priv_ns_op(NT_DEFVAR, tmp_count, NULL, NULL);
    gen_priv_ns_op(NT_DEFVAR, tmp_type, NULL, NULL);
    gen_priv_ns_op(NT_DEFVAR, tmp_len, NULL, NULL);
    gen_priv_ns_op(NT_DEFVAR, ret_val, NULL, NULL);

    gen_priv_ns_op(NT_DEFVAR, tmp_1, NULL, NULL);
    gen_priv_ns_op(NT_DEFVAR, tmp_2, NULL, NULL);
    gen_priv_ns_op(NT_DEFVAR, tmp_3, NULL, NULL);

    out = str_init();
    str_cat_chars(out, "\n");
    str_cat_chars(out, "######################### MAIN ##################################\n\n");

    insert(out);
    set_frame_beginning();
}

void gen_end() {
    gen_exit(OK);
    string *out = str_init_chars("######################### END MAIN ##############################\n\n");
    buffer_insert(&main_buffer, out, 0);

    gen_priv_label(LAB_ERR_SEM_DEF, NULL);
    gen_exit(ERR_SEM_DEF);
    insert(str_init_chars("\n"));

    gen_priv_label(LAB_ERR_SEM_RUN, NULL);
    gen_exit(ERR_SEM_RUN);
    insert(str_init_chars("\n"));

    gen_priv_label(LAB_ERR_ZERO_DIV, NULL);
    gen_exit(ERR_ZERO_DIV);
    insert(str_init_chars("\n"));

    string *b_fnc = str_init();
    str_cat_chars(b_fnc, "\n\n");

    buffer_insert(&func_buffer, b_fnc, 0);

    buffer_clear_print(&main_buffer);
    buffer_clear_print(&func_buffer);
}

/// ------------ variable definition -----

void gen_def_var(string *id) {
    string* id_name = make_id(id, location.main);
    gen_priv_ns_op(NT_DEFVAR, id_name, NULL, NULL);
    str_destroy(id_name);
}

/// ------------ assigment ---------------

void gen_assign(string *id, int is_global) {
    string* name = make_id(id, is_global);
    gen_priv_comment("ASSIGMENT", CH_NONEXPRESION);
    gen_priv_ns_op(NT_POPS, name, NULL, NULL);
    str_destroy(name);
}

void gen_non_assign(){
    gen_priv_s_op(ST_CLEARS);
}

/// ------------ if while ---------------

void gen_if_beg() {
    mng_location(CL_IF_WHILE_BEG);

    string *name = str_init_fmt("%d", if_while_stack.head->value);
    gen_bool_conversion();

    gen_priv_comment("IF BEGIN", CH_PROGRAM_FLOW);
    gen_priv_ns_op(NT_POPS, tmp_cond, NULL, NULL);
    gen_priv_jump_if(0, LAB_ELSE, name, tmp_cond, strue);

    str_destroy(name);
}

void gen_else() {
    string *name = str_init_fmt("%d", if_while_stack.head->value);

    gen_priv_comment("ELSE BEGIN", CH_PROGRAM_FLOW);
    gen_priv_jump(LAB_IF_END, name);
    gen_priv_label(LAB_ELSE, name);

    str_destroy(name);
}

void gen_if_end() {
    string *name = str_init_fmt("%d", if_while_stack.head->value);

    gen_priv_comment("IF END", CH_PROGRAM_FLOW);
    gen_priv_label(LAB_IF_END, name);

    mng_location(CL_IF_WHILE_END);
    str_destroy(name);
}

void gen_while_beg() {
    mng_location(CL_IF_WHILE_BEG);
    string *name = str_init_fmt("%d", if_while_stack.head->value);

    gen_priv_comment("WHILE BEGIN", CH_PROGRAM_FLOW);
    gen_priv_label(LAB_WHILE, name);

    str_destroy(name);
}

void gen_while_cond() {
    string *name = str_init_fmt("%d", if_while_stack.head->value);

    gen_bool_conversion();

    gen_priv_ns_op(NT_POPS, tmp_cond, NULL, NULL);
    gen_priv_jump_if(0, LAB_WHILE_END, name, tmp_cond, strue);

    str_destroy(name);
}

void gen_while_end() {
    string *name = str_init_fmt("%d", if_while_stack.head->value);

    gen_priv_jump(LAB_WHILE, name);
    gen_priv_label(LAB_WHILE_END, name);

    str_destroy(name);
}

/// ------------ fnc  ------------------


void gen_fnc_def_beg(string *fnc_name, string *param_names[], unsigned int count) {
    mng_location(CL_FNC_BEG);

    gen_priv_comment(fnc_name->str, CH_FNC_BEG);
    gen_priv_label(FNC, fnc_name);
    gen_priv_frame(FT_PUSH);
    gen_new_line();

    // set params
    set_frame_beginning();
    for (int i = ((int) count - 1); i >= 0; i--) {
        string* par = make_id(param_names[i], 0);
        gen_priv_ns_op(NT_DEFVAR, par, NULL, NULL);
        gen_priv_ns_op(NT_POPS, par, NULL, NULL);
        str_destroy(par);
    }
    set_frame_beginning();
}

void gen_fnc_def_end() {
    gen_priv_ns_op(NT_PUSHS, snill, NULL, NULL);
    gen_priv_ns_op(NT_RETURN, NULL, NULL, NULL);
    gen_priv_comment("", CH_FNC_END);
    mng_location(CL_FNC_END);
}

void gen_const_param(data_type type, string *literal) {
    string *con = make_constant(type, literal);
    buffer_insert(&param_buffer, con, 0);
}

void gen_id_param(string *id_name, int is_global) {
    string *id = make_id(id_name, is_global);
    buffer_insert(&param_buffer, id, 0);
}

void gen_return(int is_None) {
    gen_new_line();
    if (is_None) {
        gen_priv_ns_op(NT_PUSHS, snill, NULL, NULL);
    }
    gen_priv_ns_op(NT_RETURN, NULL, NULL, NULL);
}

void gen_fnc_call(string *fnc_name) {

    gen_priv_comment(fnc_name->str, CH_PROGRAM_FLOW);
    gen_priv_frame(FT_CREATE);

    string *param = buffer_front(&param_buffer);
    while (param != NULL) {
        gen_priv_ns_op(NT_PUSHS, param, NULL, NULL);
        str_destroy(param);
        param = buffer_front(&param_buffer);
    }

    gen_priv_ns_op(NT_CALL, fnc_name, NULL, NULL);
    gen_priv_frame(FT_POP);
}

/// ------------ build-in fnc ---------

void gen_param_ID_check(data_type type){
    string* param = str_init_chars(param_buffer.rear->content->str);

    gen_priv_comment("PARAM CHECK", CH_EXPRESSION_3);
    gen_priv_ns_op(NT_TYPE, tmp_type, param, NULL);

    string* s_type = str_init();
    if (type == DT_INT) str_cat_chars(s_type, "string@int");
    else if (type == DT_STRING) str_cat_chars(s_type, "string@string");

    gen_priv_jump_if(0, LAB_ERR_SEM_RUN, NULL, tmp_type, s_type);

    str_destroy(param);
    str_destroy(s_type);
}

void gen_print() {
    gen_priv_comment("PRINT", CH_PROGRAM_FLOW);

    string *nl = str_init_chars("string@\\010");
    string *space = str_init_chars("string@\\032");

    string *param = buffer_front(&param_buffer);
    while (param != NULL) {
        gen_priv_ns_op(NT_WRITE, param, NULL, NULL);
        str_destroy(param);
        param = buffer_front(&param_buffer);
        if (param != NULL) gen_priv_ns_op(NT_WRITE, space, NULL, NULL);
    }
    gen_priv_ns_op(NT_WRITE, nl, NULL, NULL);
    str_destroy(nl);
    str_destroy(space);
}


void gen_input_check(){
    gen_priv_comment("INPUT CHECK", CH_EXPRESSION_3);
    gen_priv_ns_op(NT_TYPE, tmp_type, ret_val, NULL);
    gen_priv_jump_if(1, LAB_ERR_SEM_RUN, NULL, ret_val, snill);
}

void gen_inputs() {
    gen_priv_comment("INPUTS", CH_PROGRAM_FLOW);
    gen_priv_ns_op(NT_INPUTS, ret_val, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, ret_val, NULL, NULL);
    gen_input_check();
}

void gen_inputi() {
    gen_priv_comment("INPUTI", CH_PROGRAM_FLOW);
    gen_priv_ns_op(NT_INPUTI, ret_val, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, ret_val, NULL, NULL);
    gen_input_check();
}

void gen_inputf() {\
    gen_priv_comment("INPUTF", CH_PROGRAM_FLOW);
    gen_priv_ns_op(NT_INPUTF, ret_val, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, ret_val, NULL, NULL);
    gen_input_check();
}

void gen_len() {
    gen_priv_comment("LEN", CH_PROGRAM_FLOW);
    string *s = buffer_front(&param_buffer);

    gen_priv_ns_op(NT_STRLEN, ret_val, s, NULL);
    gen_priv_ns_op(NT_PUSHS, ret_val, NULL, NULL);

    str_destroy(s);
}


void gen_substr() {
    gen_priv_comment("SUBSTR", CH_PROGRAM_FLOW);
    string *s = buffer_front(&param_buffer);
    string *i = buffer_front(&param_buffer);
    string *n = buffer_front(&param_buffer);

    string *index = unique_label();

    gen_priv_ns_op(NT_MOVE, ret_val, st, NULL);
    gen_priv_ns_op(NT_MOVE, tmp_count, i, NULL);

    gen_priv_ns_op(NT_STRLEN, tmp_1, s, NULL);
    gen_priv_ns_op(NT_ADD, tmp_2, i, n);

    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, s0, NULL, NULL);
    gen_priv_s_op(ST_LTS);

    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, tmp_1, NULL, NULL);
    gen_priv_s_op(ST_GTS);

    gen_priv_s_op(ST_ORS);
    gen_priv_ns_op(NT_PUSHS, n, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, s0, NULL, NULL);
    gen_priv_s_op(ST_LTS);

    gen_priv_s_op(ST_ORS);
    gen_priv_ns_op(NT_PUSHS, strue, NULL, NULL);

    gen_priv_jump_if_s(1, "nill$substr", index);

    gen_priv_ns_op(NT_LT, tmp_cond, tmp_1, tmp_2);
    gen_priv_jump_if(1, "else$substr", index, tmp_cond, strue);

    gen_priv_ns_op(NT_MOVE, tmp_1, tmp_2, NULL);
    gen_priv_label("else$substr", index);

    gen_priv_label("while$substr", index);
    gen_priv_ns_op(NT_LT, tmp_cond, tmp_count, tmp_1);

    gen_priv_jump_if(0, "while_end$substr", index, tmp_cond, strue);

    gen_priv_ns_op(NT_GETCHAR, tmp_3, s, tmp_count);
    gen_priv_ns_op(NT_CONCAT, ret_val, ret_val, tmp_3);
    gen_priv_ns_op(NT_ADD, tmp_count, tmp_count, s1);

    gen_priv_jump("while$substr", index);
    gen_priv_label("while_end$substr", index);
    gen_priv_ns_op(NT_PUSHS, ret_val, NULL, NULL);
    gen_priv_jump("substr$end", index);
    gen_priv_label("nill$substr", index);

    gen_priv_ns_op(NT_PUSHS, snill, NULL, NULL);
    gen_priv_label("substr$end", index);

    str_destroy(s);
    str_destroy(i);
    str_destroy(n);
    str_destroy(index);
}

void gen_ord() {
    gen_priv_comment("ORD", CH_PROGRAM_FLOW);
    string *s = buffer_front(&param_buffer);
    string *i = buffer_front(&param_buffer);
    string *index = unique_label();

    gen_priv_ns_op(NT_STRLEN, tmp_len, s, NULL);
    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, s0, NULL, NULL);
    gen_priv_s_op(ST_LTS);

    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, tmp_len, NULL, NULL);
    gen_priv_s_op(ST_LTS);

    gen_priv_s_op(ST_NOTS);
    gen_priv_s_op(ST_ORS);

    gen_priv_ns_op(NT_PUSHS, strue, NULL, NULL);

    gen_priv_jump_if_s(1, "end$ord", index);
    gen_priv_ns_op(NT_STRI2INT, ret_val, s, i);

    gen_priv_label("end$ord", index);
    gen_priv_ns_op(NT_PUSHS, ret_val, NULL, NULL);

    str_destroy(s);
    str_destroy(i);
    str_destroy(index);
}

void gen_chr() {
    gen_priv_comment("CHR", CH_PROGRAM_FLOW);
    string* out = str_init();
    string* i = buffer_front(&param_buffer);
    string* s255 = str_init_chars("int@255");

    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, s255, NULL, NULL);
    gen_priv_s_op(ST_GTS);
    gen_priv_ns_op(NT_PUSHS, s0, NULL, NULL);
    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);
    gen_priv_s_op(ST_GTS);
    gen_priv_s_op(ST_EQS);

    gen_priv_ns_op(NT_PUSHS, strue, NULL, NULL);
    gen_priv_jump_if_s(0, LAB_ERR_SEM_RUN, NULL);
    gen_priv_ns_op(NT_PUSHS, i, NULL, NULL);

    str_cat_chars(out, "INT2CHARS\n");
    insert(out);

    str_destroy(i);
    str_destroy(s255);
}


/// ########### Expressions ############

void gen_push_id(string *id_name, int is_global) {
    gen_priv_comment("OPERAND", CH_EXPRESSION_2);

    string *id = make_id(id_name, is_global);
    gen_priv_ns_op(NT_PUSHS, id, NULL, NULL);
    str_destroy(id);
}

void gen_push_constant(data_type type, string *literal) {
    gen_priv_comment("OPERAND", CH_EXPRESSION_2);

    string *constant = make_constant(type, literal);
    gen_priv_ns_op(NT_PUSHS, constant, NULL, NULL);
    str_destroy(constant);
}

void gen_stack_operation(enum lexeme operator) {
    gen_priv_comment("OPERATION", CH_EXPRESSION_3);
    int negate = 0;
    switch (operator) {
        case T_PLUS:
            gen_priv_s_op(ST_ADDS);
            break;
        case T_MINUS:
            gen_priv_s_op(ST_SUBS);
            break;
        case T_MUL:
            gen_priv_s_op(ST_MULS);
            break;
        case T_DIVISION:
            gen_priv_s_op(ST_IDIVS);
            break;
        case T_F_DIVISION:
            gen_priv_s_op(ST_DIVS);
            break;
        case T_LE:
            negate = 1;
            /* Fallthrough */
        case T_GT:
            gen_priv_s_op(ST_GTS);
            break;
        case T_GE:
            negate = 1;
            /* Fallthrough */
        case T_LT:
            gen_priv_s_op(ST_LTS);
            break;
        case T_N_EQUAL:
            negate = 1;
            /* Fallthrough */
        case T_EQUAL:
            gen_priv_s_op(ST_EQS);
            break;
        default:
            return;
    }

    if (negate) {
        gen_priv_s_op(ST_NOTS);
    }
}

void gen_concat() {
    gen_priv_comment("CONCATS", CH_EXPRESSION_3);
    
    gen_priv_ns_op(NT_POPS, tmp_2, NULL, NULL);
    gen_priv_ns_op(NT_POPS, tmp_1, NULL, NULL);

    gen_priv_ns_op(NT_CONCAT, tmp_3, tmp_1, tmp_2);

    gen_priv_ns_op(NT_PUSHS, tmp_3, NULL, NULL);
}

void gen_conversion(enum func_target target) {
    gen_priv_comment("CONVERSION", CH_EXPRESSION_1);
    if (target == FT_BELOW) {
        gen_priv_ns_op(NT_POPS, tmp_1, NULL, NULL);
    }

    gen_priv_s_op(ST_INT2FLOATS);

    if (target == FT_BELOW) {
        gen_priv_ns_op(NT_PUSHS, tmp_1, NULL, NULL);
    }
}

void gen_bool_conversion() {
    gen_priv_comment("CONDITION CONVERSION", CH_EXPRESSION_3);

    string *true_label = unique_label();
    string *false_label = unique_label();
    string *float_label = unique_label();
    string *string_label = unique_label();
    string *end_label = unique_label();

    string *type_string = str_init();
    string *zero_constant = str_init();

    string *true_constant = str_init_chars("bool@true");
    string *false_constant = str_init_chars("bool@false");

    gen_priv_ns_op(NT_POPS, tmp_1, NULL, NULL);
    gen_priv_ns_op(NT_TYPE, tmp_type, tmp_1, NULL);

    // If bool
    str_cpy_chars(type_string, "string@bool");
    gen_priv_jump_if(1, LAB_TYPE_CHECK, end_label, tmp_type, type_string);

    // If nil
    str_cpy_chars(type_string, "string@nil");
    gen_priv_jump_if(1, LAB_TYPE_CHECK, false_label, tmp_type, type_string);

    // If int
    str_cpy_chars(type_string, "string@int");

    gen_priv_jump_if(0, LAB_TYPE_CHECK, float_label, tmp_type, type_string);
    gen_priv_jump_if(1, LAB_TYPE_CHECK, false_label, tmp_1, s0);

    // If float
    gen_priv_label(LAB_TYPE_CHECK, float_label);

    str_cpy_chars(type_string, "string@float");

    gen_priv_jump_if(0, LAB_TYPE_CHECK, string_label, tmp_type, type_string);
    gen_priv_jump_if(1, LAB_TYPE_CHECK, false_label, tmp_1, f0);

    // If string
    gen_priv_label(LAB_TYPE_CHECK, string_label);

    str_cpy_chars(type_string, "string@string");

    gen_priv_jump_if(0, LAB_TYPE_CHECK, true_label, tmp_type, type_string);
    gen_priv_jump_if(1, LAB_TYPE_CHECK, false_label, tmp_1, st);

    // True
    gen_priv_label(LAB_TYPE_CHECK, true_label);
    gen_priv_ns_op(NT_MOVE, tmp_1, true_constant, NULL);
    gen_priv_jump(LAB_TYPE_CHECK, end_label);

    // False
    gen_priv_label(LAB_TYPE_CHECK, false_label);
    gen_priv_ns_op(NT_MOVE, tmp_1, false_constant, NULL);

    // End
    gen_priv_label(LAB_TYPE_CHECK, end_label);

    gen_priv_ns_op(NT_PUSHS, tmp_1, NULL, NULL);

    str_destroy(true_label);
    str_destroy(false_label);
    str_destroy(float_label);
    str_destroy(string_label);
    str_destroy(end_label);

    str_destroy(type_string);
    str_destroy(zero_constant);
    str_destroy(true_constant);
    str_destroy(false_constant);
}

void gen_types() {
    gen_priv_comment("VALUES2TYPES", CH_EXPRESSION_1);

    gen_priv_ns_op(NT_POPS, tmp_1, NULL, NULL);
    gen_priv_ns_op(NT_POPS, tmp_2, NULL, NULL);

    gen_priv_ns_op(NT_TYPE, tmp_type, tmp_2, NULL);
    gen_priv_ns_op(NT_PUSHS, tmp_type, NULL, NULL);

    gen_priv_ns_op(NT_TYPE, tmp_type, tmp_1, NULL);
    gen_priv_ns_op(NT_PUSHS, tmp_type, NULL, NULL);
}

void gen_exit(int err_flag) {
    string *title_string = str_init_fmt("EXIT %d", err_flag); 
    char *title = str_as_chars(title_string);
    if(title == NULL) {
        gen_priv_comment("EXIT", CH_EXPRESSION_2);
    } else {
        gen_priv_comment(title, CH_EXPRESSION_2);
        free(title);
    }

    string *out = str_init_chars("CLEARS\n");
    insert(out);
    out = str_init_fmt("EXIT int@%d\n", err_flag);
    insert(out);

    str_destroy(title_string);
}

void gen_zero_check(data_type type) {
    string *zero = NULL;
    switch (type) {
        case DT_INT:
            zero = str_init_chars("int@0");
            break;
        case DT_DOUBLE:
            zero = str_init_chars("float@0x0p+0");
            break;
        default:
            return;
    }

    gen_priv_comment("ZERO DIVISION CHECK", CH_EXPRESSION_3);

    gen_priv_copy(FT_TOP, tmp_1);

    gen_priv_jump_if(1, LAB_ERR_ZERO_DIV, NULL, tmp_1, zero);

    str_destroy(zero);
}

void gen_isdef_check(string *id_name, int is_global) {
    gen_priv_comment("IS DEFINED CHECK", CH_EXPRESSION_3);

    string *id = make_id(id_name, is_global);

    gen_priv_ns_op(NT_TYPE, tmp_type, id, NULL);
    gen_priv_jump_if(1, LAB_ERR_SEM_DEF, NULL, tmp_type, st);

    str_destroy(id);
}


/// ########### CHECK_TYPE MACRO ############


void gen_priv_check_type_begin(char *type, enum func_target target, string **else_label, string **end_label) {
    gen_priv_comment("DYNAMIC TYPE CHECK", CH_EXPRESSION_3);
    
    *else_label = unique_label();
    *end_label = unique_label();

    string *type_string = str_init_chars("string@");
    str_cat_chars(type_string, type);
    
    gen_priv_copy(target, tmp_1);
    
    gen_priv_ns_op(NT_TYPE, tmp_type, tmp_1, NULL);

    gen_priv_jump_if(0, LAB_TYPE_CHECK, *else_label, tmp_type, type_string);

    str_destroy(type_string);
}

void gen_priv_check_type_else(string *else_label, string *end_label) {
    gen_priv_jump(LAB_TYPE_CHECK, end_label);

    gen_priv_comment("ELSE", CH_EXPRESSION_3);

    gen_priv_label(LAB_TYPE_CHECK, else_label);
}

void gen_priv_check_type_end(string *else_label, string *end_label) {
    gen_priv_comment("END", CH_EXPRESSION_3);

    gen_priv_label(LAB_TYPE_CHECK, end_label);

    str_destroy(else_label);
    str_destroy(end_label);
}
