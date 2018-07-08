////////////// HEADER
#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#endif

///////////// HEADER

enum {LVAL_NUM, LVAL_ERR};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

typedef struct{
	int type;
	int err;
	long num;
} lval;

lval lval_num(long x){
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;	
}

lval lval_err(int x){
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}


void lval_print(lval v){
	switch (v.type) {
		case LVAL_NUM: printf("%li", v.num); break;

		case LVAL_ERR:
			if(v.err == LERR_DIV_ZERO){
				 printf("Error: Division By Zero!");
			}else if(v.err == LERR_BAD_OP){
				printf("Error: Invalid Operator!");
			}else if(v.err == LERR_BAD_NUM){
				printf("Error: Invalid Number!");
			}
		break;
	}
}
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op,lval y){
	//handle symbols
	if(x.type == LVAL_ERR){return x;}
	if(y.type == LVAL_ERR){return y;}
	
	//all is ok
	if(strcmp(op, "+") == 0){ return lval_num(x.num + y.num);}	
	if(strcmp(op, "-") == 0){ return lval_num(x.num - y.num);}
	if(strcmp(op, "*") == 0){ return lval_num(x.num * y.num);}
	if(strcmp(op, "/") == 0){ 
		//if y = 0
		return y.num == 0 
		? lval_err(LERR_DIV_ZERO) 
		: lval_num(x.num / y.num);
	}
	
	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){
	//if number return (base case)
	if(strstr(t->tag, "number")){
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	};
	
	//operator is always second child
	char* op = t->children[1]->contents;
	lval x = eval(t->children[2]);
	
	int i = 3;
	while(strstr(t->children[i]->tag, "expr")){
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	return x;
}

// long number_of_nodes(mpc_ast_t* t){
// 	//base
// 	if(t->children_num == 0){
// 		return 0;
// 	}else if (t->children >= 1){
// 		long total = 0;
// 		for (int i = 0; i < t->children_num; ++i){
// 		total += number_of_nodes(t->children[i]);
// 		}
// 		return total;
// 	}
// 	return 0;
// }

int main(int argc, char** argv){
	mpc_parser_t* Number 	= mpc_new("number");
	mpc_parser_t* Operator 	= mpc_new("operator");
	mpc_parser_t* Expr 		= mpc_new("expr");
	mpc_parser_t* Nispy 	= mpc_new("nispy");

	mpca_lang(MPCA_LANG_DEFAULT,
		" 														\
			number		: /-?[0-9]+/ ; 							\
			operator	: '+' | '-' | '*' | '/' ; 				\
			expr		: <number> | '(' <operator> <expr>+ ')';\
			nispy		: /^/ <operator> <expr>+ /$/ ;			\
		",
			Number, Operator, Expr, Nispy);
		
	puts("Nisp version 0.0.0.0.1");
	puts("Press Ctrl+c to exit\n");
	
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	printf("%d-%d-%d %d:%d:%d\n",tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour,tm.tm_min, tm.tm_sec);
	
	while(1){
		char* input = readline("nispy> ");
		add_history(input); 
		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Nispy, &r)){
			lval result = eval(r.output);
			lval_print(result);
			mpc_ast_print(r.output);//printing the regex itself
			mpc_ast_delete(r.output);
		}else{
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}
	mpc_cleanup(4, Number, Operator, Expr, Nispy);
	return 0;
}
