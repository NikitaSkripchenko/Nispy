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

enum {LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};

typedef struct lval {
	int type;
	long num;
	int count;
	struct lval** cell;
	char* err;
	char* sym;
} lval;

lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;	
}

lval* lval_err(char* m){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m)+1);
	strcpy(v->err, m);
	return v;
}

lval* lval_sym(char* m){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(m)+1);
	strcpy(v->sym, m);
	return v;
}

lval* lval_sexpr(void){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_qexpr(void){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

////READING
void lval_print(lval* v);
void lval_del(lval* v);
lval* lval_add(lval* v, lval* x);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
#define LASSERT(args, cond, err)\
if (!(cond)) {lval_del(args); return lval_err(err); }
lval* builtin_head(lval* v);
lval* builtin_tail(lval* v);
lval* builtin_list(lval* v);
lval* builtin_eval(lval* v);
lval* lval_join(lval* x, lval* y);
lval* builtin_op(lval* v, char* op);
lval* builtin_join(lval* v);
lval* builtin(lval* v, char* func);
lval* lval_eval_sexpr(lval* v);

///READING
void lval_del(lval* v) {

  switch (v->type) {
    case LVAL_NUM: break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    
    /* If Qexpr or Sexpr then delete all elements inside */
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      /* Also free the memory allocated to contain the pointers */
      free(v->cell);
    break;
  }
  
  free(v);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}
	
void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    
    lval_print(v->cell[i]);
    
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
  }
}


void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE
		? lval_num(x)
		: lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {

  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  
  if (strstr(t->tag, "qexpr")) {x = lval_qexpr(); }
  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

//just pop it out
lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(lval*) * (v->count-i-1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

//take and delete the rest
lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

void lval_print(lval* v);

lval* builtin_head(lval* v){
	/* Check Error Conditions */
	LASSERT(v, v->count == 1, "Error: Func Head passed to many args");

	LASSERT (v, v->cell[0]->type == LVAL_QEXPR, "Error: Func Head passed incorrect type");

	LASSERT(v, v->cell[0]->count != 0, "Error: Func Head passed {}");

	//Otherwise take first arg
	lval* a = lval_take(v, 0);
	
	//And delete all args that not head and return
	while (a->count > 1){
		lval_del(lval_pop(a, 1));
	}
	return a;
}

lval* builtin_tail(lval* v){
	/* Check Error Conditions */
	LASSERT(v, v->cell[0]->count != 0, "Error: Func Tail passed {}");

	LASSERT(v, v->count == 1, "Error Func Tail passed too many args");

	LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Error: Func Tail passed incorrect types");

	lval* a = lval_take(v, 0);
	
	lval_del(lval_pop(a, 0));
	return a;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* lval_eval(lval* v) {
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  return v;
}

lval* builtin_eval(lval* v){
	LASSERT(v, v->count == 1, 
	"Error: Func Eval passed to many args");
	LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
	 "Error: Func Eval passed incorrect type");
	
	lval* x = lval_take(v, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(x);
}

lval* lval_join(lval* x, lval* y) {

  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);  
  return x;
}

lval* builtin_op(lval* v, char* op){
	for (int i = 0; i < v->count; ++i){
		if(v->cell[i]->type != LVAL_NUM){
			lval_del(v);
			return lval_err("Error: Non number!");
		}
	}
	
	//pop the first
	lval* x = lval_pop(v, 0);
	
	if((strcmp(op, "-") == 0) && v->count == 0){
		x->num = -x->num;
	}
	
	while(v->count > 0){
		lval* y = lval_pop(v, 0);
		if(strcmp(op, "%") == 0){
			if(y->num == 0){
				lval_del(x);
				lval_del(y);
				x = lval_err("Division by Zero");
				break;
			}
		x->num %= y->num;}
		if(strcmp(op, "+") == 0){ x->num += y->num;}	
		if(strcmp(op, "-") == 0){ x->num -= y->num;}
		if(strcmp(op, "*") == 0){ x->num *= y->num;}
		if(strcmp(op, "/") == 0){ 
			//if y = 0
			if( y->num == 0){
				lval_del(x);
				lval_del(y);
				x = lval_err("Division by Zero");
				break;
			}
			x->num /= y->num;
			}
		lval_del(y);
	}
	lval_del(v);
	return x;
}

lval* builtin_join(lval* v){
	for (int i = 0; i < v->count ;++i){
		LASSERT(v, v->cell[i]->type == LVAL_QEXPR, "Error: Func Join passed incorrect types");
	}
	
	lval* x = lval_pop(v, 0);
	
	while(v->count){
		x = lval_join(x, lval_pop(v, 0));
	}
	lval_del(v);
	return x;
}

lval* builtin(lval* v, char* func){
	if(strcmp("list", func) == 0 ) {return builtin_list(v);}
	if(strcmp("tail", func) == 0 ) {return builtin_tail(v);}
	if(strcmp("head", func) == 0 ) {return builtin_head(v);}
	if(strcmp("join", func) == 0 ) {return builtin_join(v);}
	if(strcmp("eval", func) == 0 ) {return builtin_eval(v);}
	if(strstr("+-/*%", func )) {return builtin_op(v, func);}
	lval_del(v);
	return lval_err("Unknown Function");
}

lval* lval_eval_sexpr(lval* v){
	for(int i = 0; i < v->count; ++i){
		v->cell[i] = lval_eval(v->cell[i]);
	}
	
	//errors
	for(int i = 0; i < v->count; ++i){
		if(v->cell[i]->type == LVAL_ERR){
			return lval_take(v,i);
		}
	}

	//empty
	if(v->count == 0)
		return v;
	
	if(v->count == 1)
		return lval_take(v, 0);
	
	lval* f = lval_pop(v, 0);
	if(f->type != LVAL_SYM){
		lval_del(f);
		lval_del(v);
		return lval_err("S-expr does not start with symbol!");
	}
	
	lval* result = builtin(v, f->sym);
	lval_del(f);
	return result;
}

int main(int argc, char** argv){

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Qexpr = mpc_new("qexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Nispy = mpc_new("nispy");

	mpca_lang(MPCA_LANG_DEFAULT,
		" 														\
			number : /-?[0-9]+/ ; 							\
			symbol	: \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | '+' | '-' | '*' | '/' | '%' ; 				\
			sexpr:  '('  <expr>* ')'; 						\
			qexpr: '{' <expr>* '}'; 	\
			expr : <number> | <symbol> |<sexpr>| <qexpr>; 			\
			nispy : /^/ <expr>* /$/ ;			\
		",
			Number, Symbol, Sexpr, Qexpr, Expr, Nispy);

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
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
// 			mpc_ast_print(r.output);//printing the regex itself
// 			mpc_ast_delete(r.output);
		}
		else{
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Nispy);
	return 0;
}
