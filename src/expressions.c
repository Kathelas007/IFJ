/*
*********************************************
    Projekt:    IFJ 2019
    Tým:        086
    Varianta:   II
    Členové:    Antonín Hubík    (xhubik03)
                Daša Nosková     (xnosko05)
                David Holas      (xholas11)
                Kateřina Mušková (xmusko00)
    
    Soubor:     expressions.c
    Autor:      David Holas      (xholas11)
    Úpravy:     
*********************************************
*/

#include "expressions.h"

#include "expr_stack.h"
#include "dynamic_string.h"
#include "error.h"
#include "scanner.h"
#include "generator.h"

#include <stdlib.h>
#include <string.h>

#define E_A "Expression Analyser"

#define EXPR_PREFIX 5500
/// Additional symbols to lexemes
enum symbol {
	// Nonterminal
	E_E = EXPR_PREFIX,

	// Stack bottom
	E_END,	// $

	// Table
	H_BEG,	// <
	H_END,	// >
	H_SHF,	// =
	H_ERR

};
typedef int symbol; // Unifies both enums (Makes conversion between enum lexeme and enum symbol implicit)

/// Expression nonterminal
typedef struct expression {
	symbol lexeme;
	data_type type;
} *expr;

static symtab *global_symtab = NULL;
static symtab *local_symtab = NULL;
static symtab *tmp_symtab = NULL;



/***********************************************
*	Rules
************************************************/

#define RULES_LEN 17
#define MAX_HANDLE_LEN 3

typedef struct rule {
	char *name;
	wrapper (*resolve)(wrapper[MAX_HANDLE_LEN]);
	symbol nonterminal;
	int handle_len;
	symbol handle[MAX_HANDLE_LEN];
} rule;

// Headers for rules, they are responsible for freeing handle
wrapper id_to_e(wrapper[MAX_HANDLE_LEN]);
wrapper const_to_e(wrapper[MAX_HANDLE_LEN]);
wrapper ar_operation(wrapper[MAX_HANDLE_LEN]);
wrapper plus_operation(wrapper[MAX_HANDLE_LEN]);
wrapper idiv_operation(wrapper[MAX_HANDLE_LEN]);
wrapper div_operation(wrapper[MAX_HANDLE_LEN]);
wrapper bool_operation(wrapper[MAX_HANDLE_LEN]);
wrapper eq_operation(wrapper[MAX_HANDLE_LEN]);
wrapper brackets(wrapper[MAX_HANDLE_LEN]);

#define rule_init(resolve_func, nonterminal, handle_len, ...) \
	{#resolve_func, resolve_func, nonterminal, handle_len, {__VA_ARGS__}}

static rule rules[RULES_LEN] = {
	rule_init(id_to_e, E_E, 1, T_ID),
	rule_init(const_to_e, E_E, 1, T_NONE),
	rule_init(const_to_e, E_E, 1, T_INT),
	rule_init(const_to_e, E_E, 1, T_STRING),
	rule_init(const_to_e, E_E, 1, T_DOUBLE),

	rule_init(plus_operation, E_E, 3, E_E, T_PLUS, E_E),
	rule_init(ar_operation, E_E, 3, E_E, T_MINUS, E_E),
	rule_init(ar_operation, E_E, 3, E_E, T_MUL, E_E),
	rule_init(idiv_operation, E_E, 3, E_E, T_DIVISION, E_E),
	rule_init(div_operation, E_E, 3, E_E, T_F_DIVISION, E_E),

	rule_init(bool_operation, E_E, 3, E_E, T_GE, E_E),
	rule_init(bool_operation, E_E, 3, E_E, T_GT, E_E),
	rule_init(bool_operation, E_E, 3, E_E, T_LE, E_E),
	rule_init(bool_operation, E_E, 3, E_E, T_LT, E_E),

	rule_init(eq_operation, E_E, 3, E_E, T_EQUAL, E_E),
	rule_init(eq_operation, E_E, 3, E_E, T_N_EQUAL, E_E),

	rule_init(brackets, E_E, 3, T_L_BRACKET, E_E, T_R_BRACKET)
};

void free_handle(wrapper handle[MAX_HANDLE_LEN]) {
	for(int i=0;i<MAX_HANDLE_LEN;i++) {
		free_wrapper(handle[i]);
	}
}

int compare_handles(symbol a[], symbol b[], int len) {
	for(int i=0;i<len;i++) {
		if(a[i] != b[i]) {
			return 0;
		}
	}
	return 1;
}

/// Finds rule for given handle
int find_rule(symbol handle[], int len, rule *rule) {
	for(int i=0;i<RULES_LEN;i++) {
		if(rules[i].handle_len == len) {
			if(compare_handles(rules[i].handle, handle, len)) {
				*rule = rules[i];
				return 1;
			}
		}
	}
	return 0;
}

/// Maps handle of wrappers into handle of smybols
void map(wrapper wrapper_handle[MAX_HANDLE_LEN], symbol sym_handle[MAX_HANDLE_LEN]) {
	for(int i=0;i<MAX_HANDLE_LEN;i++) {
		if(wrapper_handle[i].token != NULL) {
			sym_handle[i] = wrapper_handle[i].token->lexeme;
		} else if(wrapper_handle[i].expr != NULL) {
			sym_handle[i] = wrapper_handle[i].expr->lexeme;
		}
	}
}

/***********************************************
*	End Rules
************************************************/



/***********************************************
*	Precedens Table
************************************************/

#define TERMINAL_NUM (T_R_BRACKET - T_PLUS + 2 + 1)
// Adds E_END and i (any operand), +1 because its size not index

						/* stack_terminal *//*  input  */
static symbol precedens_table[TERMINAL_NUM][TERMINAL_NUM] = {
				/* i *//* + *//* - *//* * *//* / *//*// *//*>= *//* > *//*<= *//* < *//*== *//*!= *//* ( *//* ) *//* $ */
	{	/* i */ H_ERR, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_ERR, H_END, H_END	},
	{	/* + */ H_BEG, H_END, H_END, H_BEG, H_BEG, H_BEG, H_END, H_END, H_END, H_END, H_END, H_END, H_BEG, H_END, H_END	},
	{	/* - */ H_BEG, H_END, H_END, H_BEG, H_BEG, H_BEG, H_END, H_END, H_END, H_END, H_END, H_END, H_BEG, H_END, H_END	},
	{	/* * */ H_BEG, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_BEG, H_END, H_END	},
	{	/* / */ H_BEG, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_BEG, H_END, H_END	},
	{	/* // */H_BEG, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_BEG, H_END, H_END	},
	{	/* >= */H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_BEG, H_END, H_END	},
	{	/* > */ H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_BEG, H_END, H_END	},
	{	/* <= */H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_BEG, H_END, H_END	},
	{	/* < */ H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_BEG, H_END, H_END	},
	{	/* == */H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_BEG, H_END, H_END	},
	{	/* != */H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_ERR, H_BEG, H_END, H_END	},
	{	/* ( */ H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_SHF, H_ERR	},
	{	/* ) */ H_ERR, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_END, H_ERR, H_END, H_END	},
	{	/* $ */ H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_BEG, H_ERR, H_ERR	}
};








/***********************************************
*	Expressions Logic
************************************************/

#define guard(cond, label) \
	if(!(cond)) goto label

token symbol_token(symbol);

int stack_init(expr_stack *stack) {
	es_init(stack);
	token bottom = symbol_token(E_END);
	if(bottom == NULL) {
		return 0;
	}
	if( !es_push(stack, wrap_token(bottom))) {
		free_token(bottom);
		return 0;
	}
	return 1;
}

void stack_destroy(expr_stack *stack) {
	while(!es_empty(stack)) {
		wrapper wrapper;
		es_copy(stack, &wrapper);
		free_wrapper(wrapper);
		es_pop(stack);
	}
	es_destroy(stack);
}

/// Is symbol valid end to an expression
int is_expression_end(symbol symbol) {
	switch(symbol) {
		case T_EOL:
		case T_COLON:
		case T_EOF:
			return 1;
		default:
			return 0;
	}
	return 0;
}

int is_terminal(symbol symbol) {
	return (symbol >= T_ID && symbol <= T_R_BRACKET) || symbol == E_END || is_expression_end(symbol);
}

/// Creates fake token from given symbol, so it can be pushed on stack
token symbol_token(symbol symbol) {
	token out = malloc(sizeof(struct token));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		return NULL;
	}
	out->lexeme = symbol;
	return out;
}

/// Uses token given by parameter repeat if not NULL
/// Otherwise uses next token from scanner
token choose_next_token(token *repeat) {
	if(*repeat == NULL) {
		return get_token();
	} else {
		token out = *repeat;
		*repeat = NULL;
		return out;
	}
}

int get_highest_terminal(expr_stack *stack, token *terminal, int *index) {
	int i = 0;
	wrapper wrapper;
	while(es_copy_at(stack, i, &wrapper)) {
		if(wrapper.token != NULL) {
			if(is_terminal(wrapper.token->lexeme)) {
				*index = i;
				*terminal = wrapper.token;
				return 1;
			}
		}
		i++;
	}
	*index = -1;
	*terminal = NULL;
	return 0; // Should never happen 
}

/// Converts input/terminal symbols into precendens table indexes
/// Returns symbol given by indexes
symbol get_precedens(symbol terminal, symbol input) {
	if(!is_terminal(terminal) || !is_terminal(input)) {
		return H_ERR;
	}
	symbol row = 0;
	symbol col = 0;

	if(terminal == E_END) {
		row = TERMINAL_NUM - 1;
	} else if(terminal >= T_ID && terminal <= T_DOUBLE) {
		row = 0;
	} else {
		row = (terminal - T_PLUS) + 1;
	}

	if(is_expression_end(input)) {
		col = TERMINAL_NUM - 1;
	} else if(input >= T_ID && input <= T_DOUBLE) {
		col = 0;
	} else {
		col = (input - T_PLUS) + 1;
	}

	return precedens_table[row][col];
}

/// Finds and resolves next rule by analyzing stack
int use_rule(expr_stack *stack) {
	int i = 0;
	wrapper handle[MAX_HANDLE_LEN] = { wrap_NULL() };
	wrapper wrapper = wrap_NULL();
	while(1) {
		if( !es_copy(stack, &wrapper) || i >= MAX_HANDLE_LEN + 1) {
			return 0;
		}
		if(is_NULL(wrapper)) {
			return 0;
		}
		if(wrapper.token != NULL && (symbol) wrapper.token->lexeme == H_BEG) {
			free_token(wrapper.token);
			es_pop(stack);
			break;
		}
		es_pop(stack);
		for(int j=MAX_HANDLE_LEN-1;j>0;j--) {
			handle[j] = handle[j-1];
		}
		handle[0] = wrapper;
		i++;
	}
	rule rule;
	symbol sym_handle[MAX_HANDLE_LEN] = { 0 };
	map(handle, sym_handle);
	if(!find_rule(sym_handle, i, &rule)) {
		return 0;
	}

	wrapper = rule.resolve(handle);
	if(is_NULL(wrapper)) {
		return 0;
	}

	if( !es_push(stack, wrapper) ) {
		free_wrapper(wrapper);
		// Malloc failed, error already printed
		return 0;
	}
	debug("Expr rule used: %s", rule.name);
	//debug("Result type %d", wrapper.expr->type);
	return 1;
}

/***********************************************
*	Entry point
************************************************/

int eval_expression(symtab *global_tab, symtab *local_tab, symtab *tmp_tab, token first, token second, token *repeat, data_type *result_type) {
	if(global_tab == NULL || local_tab == NULL || tmp_tab == NULL) {
		error(E_A, ERR_INTERN, "No symbol table passed!");
		return -1;
	}
	global_symtab = global_tab;
	local_symtab = local_tab;
	tmp_symtab = tmp_tab;

	expr_stack stack;

	if( !stack_init(&stack)) {
		return -1;
	}
	
	wrapper end_wrap;
	es_copy(&stack, &end_wrap);

	token terminal = end_wrap.token;
	guard(terminal != NULL, just_destroy);
	token input = choose_next_token(&first);
	guard(input != NULL, just_destroy);

	int index = 0;
	while(!es_empty(&stack)) {		
		switch(get_precedens(terminal->lexeme, input->lexeme)) {
			case H_SHF:
				guard(es_push(&stack, wrap_token(input)), free_input);
				input = choose_next_token(&second);
				guard(input != NULL, just_destroy);
				break;
			case H_BEG:
			{
				token beg = symbol_token(H_BEG);
				guard(beg != NULL, free_input);
				if( !es_push_at(&stack, index, wrap_token(beg))) {
					free_token(beg);
					goto free_input;
				}
				guard(es_push(&stack, wrap_token(input)), free_input);
				input = choose_next_token(&second);
				guard(input != NULL, just_destroy);
				break;
			}
			case H_END:
				if(use_rule(&stack)) {
					break;
				}
				debug("Fallthrough");
				/* Fallthrough */
			default:
				error(E_A, ERR_SYN_AN, "Invalid expression");
				free_token(input);
				stack_destroy(&stack);
				if(first != NULL) {
					free_token(first);
				}
				if(second != NULL) {
					free_token(second);
				}
				return -1;
				
		}
		get_highest_terminal(&stack, &terminal, &index);
		guard(terminal != NULL, free_input);
		if(is_expression_end(input->lexeme) && (symbol) terminal->lexeme == E_END) {
			if(repeat != NULL) {
				*repeat = input;
			} else {
				free_token(input);
			}
			wrapper result;
			guard(es_copy(&stack, &result), just_destroy);

			expr out = result.expr;
			guard(out != NULL, just_destroy);
			if(result_type != NULL) {
				*result_type = out->type;
			}

			stack_destroy(&stack);
			return 0;
		}
	}

	// Error labels (used by guard macros)
	free_input:
		free_token(input);
	just_destroy:
		stack_destroy(&stack);
		if(first != NULL) {
			free_token(first);
		}
		if(second != NULL) {
			free_token(second);
		}
	//just_error:
		error(E_A, ERR_INTERN, "Malloc failed");
	
	return -1;
}


/***********************************************
*	Expressions Logic End
************************************************/




















/***********************************************
*	Dynamic checks
************************************************/

/*
Generate necessary dynamic checks
On Success: return wrapped 'out' expression and generates dynamic checks
On Failure: return wrapped NULL and free 'out' expression

Does not free handle!!
*/

wrapper ar_dyn_check(expr out, data_type l_type, symbol operator, data_type r_type) {
	if(l_type == DT_DOUBLE) {
		gen_check_float(FT_TOP,
			gen_none(),
		// else
			gen_check_int(FT_TOP,
				gen_conversion(FT_TOP),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
		out->type = DT_DOUBLE;
	} else if(r_type == DT_DOUBLE) {
		gen_check_float(FT_BELOW,
			gen_none(),
		// else
			gen_check_int(FT_BELOW,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
		out->type = DT_DOUBLE;
	} else if(l_type == DT_INT) {
		gen_check_int(FT_TOP,
			gen_none(),
		// else
			gen_check_float(FT_TOP,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
		out->type = DT_UNDEF;
	} else if(r_type == DT_INT) {
		gen_check_int(FT_BELOW,
			gen_none(),
		// else
			gen_check_float(FT_BELOW,
				gen_conversion(FT_TOP),
			//else
				gen_exit(ERR_SEM_RUN)
			)
		);
		out->type = DT_UNDEF;
	} else if(l_type == DT_UNDEF && r_type == DT_UNDEF) {
		gen_check_float(FT_TOP,
			gen_check_float(FT_BELOW,
				gen_none(),
			// else
				gen_check_int(FT_BELOW,
					gen_conversion(FT_BELOW),
				// else
					gen_exit(ERR_SEM_RUN)
				)
			),
		// else
			gen_check_int(FT_TOP, 
				gen_check_int(FT_BELOW,
					gen_none(),
				// else
					gen_check_float(FT_BELOW,
						gen_conversion(FT_TOP),
					// else
						gen_exit(ERR_SEM_RUN)
					)
				),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
		out->type = DT_UNDEF;
	} else {
		error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
		free(out);
		return wrap_NULL();
	}

    gen_stack_operation(operator);

	if(ERR_FLAG == OK) {
		return wrap_expr(out);
	}
	free(out);
	return wrap_NULL();
}

wrapper plus_dyn_check(expr out, data_type l_type, symbol operator, data_type r_type) {
	if(l_type == DT_STRING) {
		gen_check_string(FT_TOP, 
			gen_none(),
		// else
			gen_exit(ERR_SEM_RUN)
		);

		gen_concat();
		out->type = DT_STRING;
	} else if(r_type == DT_STRING) {
		gen_check_string(FT_BELOW, 
			gen_none(),
		// else
			gen_exit(ERR_SEM_RUN)
		);

		gen_concat();
		out->type = DT_STRING;
	} else if(l_type == DT_DOUBLE) {
		gen_check_float(FT_TOP,
			gen_none(),
		// else
			gen_check_int(FT_TOP,
				gen_conversion(FT_TOP),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
        gen_stack_operation(operator);

		out->type = DT_DOUBLE;
	} else if(r_type == DT_DOUBLE) {
		gen_check_float(FT_BELOW,
			gen_none(),
		// else
			gen_check_int(FT_BELOW,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);

        gen_stack_operation(T_PLUS);
		out->type = DT_DOUBLE;
	} else if(l_type == DT_INT) {
		gen_check_int(FT_TOP,
			gen_none(),
		// else
			gen_check_float(FT_TOP,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);

        gen_stack_operation(operator);
		
		out->type = DT_UNDEF;
	} else if(r_type == DT_INT) {
		gen_check_int(FT_BELOW,
			gen_none(),
		// else
			gen_check_float(FT_BELOW,
				gen_conversion(FT_TOP),
			//else
				gen_exit(ERR_SEM_RUN)
			)
		);

        gen_stack_operation(operator);
		out->type = DT_UNDEF;
	} else if(l_type == DT_UNDEF && r_type == DT_UNDEF) {
		gen_check_string(FT_TOP,
			gen_check_string(FT_BELOW,
				gen_concat(),
			// else
				gen_exit(ERR_SEM_RUN)
			),
		// else
			gen_check_float(FT_TOP,
				gen_check_float(FT_BELOW,
                                gen_stack_operation(operator),
				// else
					gen_check_int(FT_BELOW,
						gen_conversion(FT_BELOW);
                                 gen_stack_operation(operator),
					// else
						gen_exit(ERR_SEM_RUN)
					)
				),
			// else
				gen_check_int(FT_TOP, 
					gen_check_int(FT_BELOW,
						gen_stack_operation(operator),
					// else
						gen_check_float(FT_BELOW,
							gen_conversion(FT_TOP);
							gen_stack_operation(operator),
						// else
							gen_exit(ERR_SEM_RUN)
						)
					),
				// else
					gen_exit(ERR_SEM_RUN)
				)
			)
		);

		out->type = DT_UNDEF;
	} else {
		error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
		free(out);
		return wrap_NULL();
	}

	if(ERR_FLAG == OK) {
		return wrap_expr(out);
	}
	free(out);
	return wrap_NULL();
}

wrapper idiv_dyn_check(expr out, data_type l_type, symbol operator, data_type r_type) {
	if(l_type == DT_INT) {
		gen_check_int(FT_TOP,
			gen_none(),
		// else
			gen_exit(ERR_SEM_RUN)
		);
	} else if(r_type == DT_INT) {
		gen_check_int(FT_BELOW,
			gen_none(),
		// else
			gen_exit(ERR_SEM_RUN)
		);
	} else if(l_type == DT_UNDEF && r_type == DT_UNDEF) {
		gen_check_int(FT_TOP,
			gen_check_int(FT_BELOW,
				gen_none(),
			// else
				gen_exit(ERR_SEM_RUN)
			),
		// else
			gen_exit(ERR_SEM_RUN)
		);
	} else {
		error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
		free(out);
		return wrap_NULL();
	}

	gen_zero_check(DT_INT);
    gen_stack_operation(operator);

	if(ERR_FLAG == OK) {
		return wrap_expr(out);
	}
	free(out);
	return wrap_NULL();
}

wrapper div_dyn_check(expr out, data_type l_type, symbol operator, data_type r_type) {
	if(l_type == DT_DOUBLE) {
		gen_check_float(FT_TOP,
			gen_none(),
		// else
			gen_check_int(FT_TOP,
				gen_conversion(FT_TOP),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else if(r_type == DT_DOUBLE) {
		gen_check_float(FT_BELOW,
			gen_none(),
		// else
			gen_check_int(FT_BELOW,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else if(l_type == DT_INT) {
		gen_check_int(FT_TOP,
			gen_conversion(FT_TOP),
		// else
			gen_check_float(FT_TOP,
				gen_none(),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);

		gen_conversion(FT_BELOW);
	} else if(r_type == DT_INT) {
		gen_check_int(FT_BELOW,
			gen_conversion(FT_BELOW),
		// else
			gen_check_float(FT_BELOW,
				gen_none(),
			//else
				gen_exit(ERR_SEM_RUN)
			)
		);

		gen_conversion(FT_TOP);
	} else if(l_type == DT_UNDEF && r_type == DT_UNDEF) {
		gen_check_float(FT_TOP,
			gen_check_float(FT_BELOW,
				gen_none(),
			// else
				gen_check_int(FT_BELOW,
					gen_conversion(FT_BELOW),
				// else
					gen_exit(ERR_SEM_RUN)
				)
			),
		// else
			gen_check_int(FT_TOP, 
				gen_conversion(FT_TOP);
				gen_check_int(FT_BELOW,
					gen_conversion(FT_BELOW),
				// else
					gen_check_float(FT_BELOW,
						gen_none(),
					// else
						gen_exit(ERR_SEM_RUN)
					)
				),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else {
		error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
		free(out);
		return wrap_NULL();
	}

	gen_zero_check(DT_DOUBLE);
    gen_stack_operation(operator);

	if(ERR_FLAG == OK) {
		return wrap_expr(out);
	}
	free(out);
	return wrap_NULL();
}

wrapper bool_dyn_check(expr out, data_type l_type, symbol operator, data_type r_type) {
	if(l_type == DT_STRING) {
		gen_check_string(FT_TOP, 
			gen_none(),
		// else
			gen_exit(ERR_SEM_RUN)
		);
	} else if(r_type == DT_STRING) {
		gen_check_string(FT_BELOW, 
			gen_none(),
		// else
			gen_exit(ERR_SEM_RUN)
		);
	} else if(l_type == DT_DOUBLE) {
		gen_check_float(FT_TOP,
			gen_none(),
		// else
			gen_check_int(FT_TOP,
				gen_conversion(FT_TOP),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else if(r_type == DT_DOUBLE) {
		gen_check_float(FT_BELOW,
			gen_none(),
		// else
			gen_check_int(FT_BELOW,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else if(l_type == DT_INT) {
		gen_check_int(FT_TOP,
			gen_none(),
		// else
			gen_check_float(FT_TOP,
				gen_conversion(FT_BELOW),
			// else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else if(r_type == DT_INT) {
		gen_check_int(FT_BELOW,
			gen_none(),
		// else
			gen_check_float(FT_BELOW,
				gen_conversion(FT_TOP),
			//else
				gen_exit(ERR_SEM_RUN)
			)
		);
	} else if(l_type == DT_UNDEF && r_type == DT_UNDEF) {
		gen_check_string(FT_TOP,
			gen_check_string(FT_BELOW,
				gen_none(),
			// else
				gen_exit(ERR_SEM_RUN)
			),
		// else
			gen_check_float(FT_TOP,
				gen_check_float(FT_BELOW,
					gen_none(),
				// else
					gen_check_int(FT_BELOW,
						gen_conversion(FT_BELOW),
					// else
						gen_exit(ERR_SEM_RUN)
					)
				),
			// else
				gen_check_int(FT_TOP, 
					gen_check_int(FT_BELOW,
						gen_none(),
					// else
						gen_check_float(FT_BELOW,
							gen_conversion(FT_TOP),
						// else
							gen_exit(ERR_SEM_RUN)
						)
					),
				// else
					gen_exit(ERR_SEM_RUN)
				)
			)
		);
	} else {
		error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
		free(out);
		return wrap_NULL();
	}

    gen_stack_operation(operator);

	if(ERR_FLAG == OK) {
		return wrap_expr(out);
	}
	free(out);
	return wrap_NULL();
}

wrapper eq_dyn_check(expr out, data_type l_type, symbol operator, data_type r_type) {
	if(l_type == DT_STRING) {
		gen_check_string(FT_TOP, 
			gen_none(),
		// else
			gen_types()
		);
	} else if(r_type == DT_STRING) {
		gen_check_string(FT_BELOW, 
			gen_none(),
		// else
			gen_types()
		);
	} else if(l_type == DT_DOUBLE) {
		gen_check_float(FT_TOP,
			gen_none(),
		// else
			gen_check_int(FT_TOP,
				gen_conversion(FT_TOP),
			// else
				gen_types()
			)
		);
	} else if(r_type == DT_DOUBLE) {
		gen_check_float(FT_BELOW,
			gen_none(),
		// else
			gen_check_int(FT_BELOW,
				gen_conversion(FT_BELOW),
			// else
				gen_types()
			)
		);
	} else if(l_type == DT_INT) {
		gen_check_int(FT_TOP,
			gen_none(),
		// else
			gen_check_float(FT_TOP,
				gen_conversion(FT_BELOW),
			// else
				gen_types()
			)
		);
	} else if(r_type == DT_INT) {
		gen_check_int(FT_BELOW,
            gen_stack_operation(T_PLUS),
		// else
			gen_check_float(FT_BELOW,
				gen_conversion(FT_TOP),
			//else
				gen_types()
			)
		);
	} else {
		gen_check_string(FT_TOP,
			gen_check_string(FT_BELOW,
				gen_none(),
			// else
				gen_types()
			),
		// else
			gen_check_float(FT_TOP,
				gen_check_float(FT_BELOW,
					gen_none(),
				// else
					gen_check_int(FT_BELOW,
						gen_conversion(FT_BELOW),
					// else
						gen_types()
					)
				),
			// else
				gen_check_int(FT_TOP, 
					gen_check_int(FT_BELOW,
						gen_none(),
					// else
						gen_check_float(FT_BELOW,
							gen_conversion(FT_TOP),
						// else
							gen_types()
						)
					),
				// else
					gen_types()
				)
			)
		);
	}

    gen_stack_operation(operator);

	if(ERR_FLAG == OK) {
		return wrap_expr(out);
	}
	free(out);
	return wrap_NULL();
}








/***********************************************
*	Resolve functions for rules
************************************************/

/*
Convert rule's right side wrappers ('handle') into wrapped rule's left side
Performs static type check, calls corresponding dynamic type check function.

DO free handle
*/

wrapper id_to_e(wrapper handle[MAX_HANDLE_LEN]) {
	token token = handle[0].token; // Cannot be NULL, because of the rule

	string *id = token->value.id_key;

	int is_global = 0;
	symt_item *item = NULL;
	item = symtab_find(local_symtab, id);
	if(item == NULL) {
		item = symtab_find(global_symtab, id);
		if(item == NULL) {
			error(E_A, ERR_SEM_DEF, "Using undefined variable in expression");
			free_handle(handle);
			return wrap_NULL();
		}
		
		if(symtab_find_insert(tmp_symtab, item->id) == NULL) {
			error(E_A, ERR_INTERN, "Could not insert item into symtable");
			free_handle(handle);
			return wrap_NULL();
		}
		is_global = 1;
	}

	id_type id_type = ID_U;
	if(get_item_type(item, &id_type) != 0) {
		error(E_A, ERR_INTERN, "Could not read id_type");
		free_handle(handle);
		return wrap_NULL();
	}
	if(id_type != ID_V) {
		error(E_A, ERR_SEM_DEF, "Variable identifier is already used by function");
		free_handle(handle);
		return wrap_NULL();
	}

	data_type data_type = DT_UNDEF;
	if(get_var_t(item, &data_type) != 0) {
		error(E_A, ERR_INTERN, "Variable has no type");
		free_handle(handle);
		return wrap_NULL();
	}

	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}

	out->lexeme = E_E;
	out->type = data_type;

	if(data_type == DT_UNDEF) {
		gen_isdef_check(id, is_global);
	}

	gen_push_id(id, is_global);

	free_handle(handle);
	return wrap_expr(out);
	
}

wrapper const_to_e(wrapper handle[MAX_HANDLE_LEN]) {
	token token = handle[0].token; // Cannot be NULL, because of the rule

	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}

	out->lexeme = E_E;
	out->type = token->lexeme - T_NONE;
	
	string *literal = NULL;
	switch(out->type) {
		case DT_NONE:
			literal = str_init_chars("nil");
			break;
	    case DT_INT:
	    	literal = str_init_fmt("%d", token->value.integer);
	    	break;
	    case DT_STRING:
	    	literal = str_init();
	    	str_cpy(literal, token->value.string_struct);
	    	break;
	    case DT_DOUBLE:
	    	literal = str_init_fmt("%a", token->value.floating_point);
	    	break;
	    default:
	    	free_handle(handle);
	    	return wrap_NULL();
	}

	gen_push_constant(out->type, literal);
	str_destroy(literal);
	free_handle(handle);
	return wrap_expr(out);
}

wrapper ar_operation(wrapper handle[MAX_HANDLE_LEN]) {
	expr l_expr = handle[0].expr;
	token operator = handle[1].token;
	expr r_expr = handle[2].expr;
	
	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}
	out->lexeme = E_E;

	if(l_expr->type == DT_UNDEF || r_expr->type == DT_UNDEF) {
		wrapper res = ar_dyn_check(out, l_expr->type, operator->lexeme, r_expr->type);
		free_handle(handle);
		return res;
	}

	if(l_expr->type == r_expr->type) {
		if(l_expr->type == DT_INT || l_expr->type == DT_DOUBLE) {
			out->type = l_expr->type;
		} else {
			error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
			free(out);
			free_handle(handle);
			return wrap_NULL();
		}
	} else if (l_expr->type == DT_DOUBLE && r_expr->type == DT_INT) {
		out->type = DT_DOUBLE;

		gen_conversion(FT_TOP);		
	} else if (l_expr->type == DT_INT && r_expr->type == DT_DOUBLE) {
		out->type = DT_DOUBLE;
		
		gen_conversion(FT_BELOW);
	} else {
		error(E_A, ERR_SEM_RUN, "Could not perform any implicit conversion between given types");
		free(out);
		free_handle(handle);
		return wrap_NULL();
	}

    gen_stack_operation(operator->lexeme);

	free_handle(handle);
	return wrap_expr(out);
}

wrapper plus_operation(wrapper handle[MAX_HANDLE_LEN]) {
	expr l_expr = handle[0].expr;
	expr r_expr = handle[2].expr;

	expr out = malloc(sizeof(struct expression));
		if(out == NULL) {
			error(E_A, ERR_INTERN, "Malloc failed");
			free_handle(handle);
			return wrap_NULL();
		}
		out->lexeme = E_E;

	if(l_expr->type == DT_UNDEF || r_expr->type == DT_UNDEF) {
		wrapper res = plus_dyn_check(out, l_expr->type, T_PLUS, r_expr->type);
		free_handle(handle);
		return res;
	}


	if(l_expr->type == DT_STRING && r_expr->type == DT_STRING) {

		out->type = DT_STRING; 

		gen_concat();

		free_handle(handle);
		return wrap_expr(out);
	} else {
		free(out);
		return ar_operation(handle);
	}
}

wrapper idiv_operation(wrapper handle[MAX_HANDLE_LEN]) {
	expr l_expr = handle[0].expr;
	expr r_expr = handle[2].expr;

	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}
	out->lexeme = E_E;

	out->type = DT_INT;

	if(l_expr->type == DT_UNDEF || r_expr->type == DT_UNDEF) {
		wrapper res = idiv_dyn_check(out, l_expr->type, T_DIVISION, r_expr->type);
		free_handle(handle);
		return res;
	}

	if(l_expr->type == DT_INT && r_expr->type == DT_INT) {

		gen_zero_check(DT_INT);
        gen_stack_operation(T_DIVISION);

		free_handle(handle);
		return wrap_expr(out);
	}

	error(E_A, ERR_SEM_RUN, "Could not perform any implicit conversion between given types");
	free(out);
	free_handle(handle);
	return wrap_NULL();
}

wrapper div_operation(wrapper handle[MAX_HANDLE_LEN]) {
	expr l_expr = handle[0].expr;
	expr r_expr = handle[2].expr;

	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}
	out->lexeme = E_E;

	out->type = DT_DOUBLE;

	if(l_expr->type == DT_UNDEF || r_expr->type == DT_UNDEF) {
		wrapper res = div_dyn_check(out, l_expr->type, T_F_DIVISION, r_expr->type);
		free_handle(handle);
		return res;
	}

	if(l_expr->type == DT_DOUBLE && r_expr->type == DT_DOUBLE) {
		gen_none();
	} else if(l_expr->type == DT_INT && r_expr->type == DT_INT) {
		
		gen_conversion(FT_TOP);
		gen_conversion(FT_BELOW);
	} else if (l_expr->type == DT_DOUBLE && r_expr->type == DT_INT) {

		gen_conversion(FT_TOP);
	} else if (l_expr->type == DT_INT && r_expr->type == DT_DOUBLE) {
		
		gen_conversion(FT_BELOW);
	} else {
		error(E_A, ERR_SEM_RUN, "Could not perform any implicit conversion between given types");
		free(out);
		free_handle(handle);
		return wrap_NULL();
	}

	gen_zero_check(DT_DOUBLE);
    gen_stack_operation(T_F_DIVISION);

	free_handle(handle);
	return wrap_expr(out);
}

wrapper bool_operation(wrapper handle[MAX_HANDLE_LEN]) {
	expr l_expr = handle[0].expr;
	token operator = handle[1].token;
	expr r_expr = handle[2].expr;
	
	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}
	out->lexeme = E_E;
	out->type = DT_BOOL;

	if(l_expr->type == DT_UNDEF || r_expr->type == DT_UNDEF) {
		wrapper res = bool_dyn_check(out, l_expr->type, operator->lexeme, r_expr->type);
		free_handle(handle);
		return res;
	}

	if(l_expr->type == r_expr->type) {
		if(!(l_expr->type == DT_INT || l_expr->type == DT_DOUBLE || l_expr->type == DT_STRING)) {
			error(E_A, ERR_SEM_RUN, "Operation is not supported with given operands");
			free(out);
			free_handle(handle);
			return wrap_NULL();
		}
	} else if (l_expr->type == DT_DOUBLE && r_expr->type == DT_INT) {

		gen_conversion(FT_TOP);	
	} else if (l_expr->type == DT_INT && r_expr->type == DT_DOUBLE) {
		
		gen_conversion(FT_BELOW);
	} else {
		
		error(E_A, ERR_SEM_RUN, "Could not perform any implicit conversion between given types");
		free(out);
		free_handle(handle);
		return wrap_NULL();
	}

    gen_stack_operation(operator->lexeme);

	free_handle(handle);
	return wrap_expr(out);
}

wrapper eq_operation(wrapper handle[MAX_HANDLE_LEN]) {
	expr l_expr = handle[0].expr;
	token operator = handle[1].token;
	expr r_expr = handle[2].expr;

	expr out = malloc(sizeof(struct expression));
	if(out == NULL) {
		error(E_A, ERR_INTERN, "Malloc failed");
		free_handle(handle);
		return wrap_NULL();
	}
	out->lexeme = E_E;
	out->type = DT_BOOL;

	if(l_expr->type == DT_UNDEF || r_expr->type == DT_UNDEF) {
		wrapper res = eq_dyn_check(out, l_expr->type, operator->lexeme, r_expr->type);
		free_handle(handle);
		return res;
	}

	if (l_expr->type == DT_DOUBLE && r_expr->type == DT_INT) {

		gen_conversion(FT_TOP);		
	} else if (l_expr->type == DT_INT && r_expr->type == DT_DOUBLE) {
		
		gen_conversion(FT_BELOW);
	} else if(l_expr->type != r_expr->type) {
		
		gen_types();
	}

    gen_stack_operation(operator->lexeme);

	free_handle(handle);
	return wrap_expr(out);
}

wrapper brackets(wrapper handle[MAX_HANDLE_LEN]) {
	free_wrapper(handle[0]);
	free_wrapper(handle[2]);
	return handle[1];
}

