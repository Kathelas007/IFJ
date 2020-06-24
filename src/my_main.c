#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "scanner.h"
#include "error.h"
#include "stack.h"
#include "dynamic_string.h"
#include "generator.h"


#define print_token(token) do{ \
    printf("%s ", str_token[token->lexeme]);\
    }while(0);

#define output "POPS GF@$$RETURN_VALUE\n\
write GF@$$RETURN_VALUE\n\
WRITE string@\\010"

const char *str_token[] = {
        [T_DEF] = "def", //0
        [T_IF] = "if",
        [T_ELSE] = "else",
        [T_WHILE] = "while",
        [T_RETURN] = "return",
        [T_INDENT] = "indent", // 5
        [T_DEDENT] = "dedent",
        [T_PASS] = "pass",
        [T_INPUT_S] = "input_s()",
        [T_INPUT_I] = "input_i()",
        [T_INPUT_F] = "input_f()", //10
        [T_PRINT] = "print()",
        [T_LEN] = "len()",
        [T_SUBSTR] = "substr()",
        [T_ORD] = "ord()",
        [T_CHR] = "ch",// 15
        [T_ID] = "ID",
        [T_NONE] = "None",
        [T_INT] = "int",
        [T_STRING] = "str",
        [T_DOUBLE] = "dbl",//20
        [T_PLUS] = "+",
        [T_MINUS] = "-",
        [T_MUL] = "*",
        [T_DIVISION] = "/",             //      division for both int and float arguments
        [T_F_DIVISION] = "//", //25     //FLOOR division for both int and float arguments
        [T_GE] = ">=",
        [T_GT] = ">",
        [T_LE] = "<=",
        [T_LT] = "<",
        [T_EQUAL] = "==",//30
        [T_N_EQUAL] = "!=",
        [T_ASSIGNMENT] = "=",
        [T_L_BRACKET] = "(",
        [T_R_BRACKET] = ")",
        [T_COLON] = ":",//35
        [T_COMMA] = ",",
        [T_EOF] = "EOF\n",
        [T_EOL] = "\\n\n", // 38
        [T_LEX_UNKNOWN] = "UNKNOWN"
};


int main() {

/*    int a = scanner_init();
    token t = get_token();

    print_token(t);
    int i = 0;
    while (t != NULL && t->lexeme != T_EOF && t->lexeme != T_LEX_UNKNOWN) {
        free_token(t);
        t = get_token();
        print_token(t);
        i++;
    }

    free_token(t);
    a = scanner_destroy();*/

    string* fnc = str_init_chars("naaame");


    string* a = str_init_chars("auto");
    string* b = str_init_chars("b");
    string* c = str_init_chars("c");

    string* v = str_init_chars("12");

    string* i = str_init_chars("i");
    string* n = str_init_chars("n");


    generator_init();
    gen_begin();

    gen_def_var(a);
    gen_def_var(i);
    gen_def_var(n);

    gen_inputs();
    gen_assign(a, 1);

    gen_inputi();
    gen_assign(i, 1);

    gen_inputi();
    gen_assign(n, 1);


    gen_id_param(a, 1);
    gen_id_param(i, 1);
    gen_id_param(n, 1);

    gen_substr();

    gen_assign(a, 1);
    gen_id_param(a, 1);
    gen_print();

//    gen_id_param(n, 1);
//    gen_id_param(n, 1);

    //gen_end();
    generator_destroy();

    str_destroy(fnc);

    str_destroy(a);
    str_destroy(b);
    str_destroy(c);
    str_destroy(v);
    str_destroy(i);
    str_destroy(n);

    return 0;
}
