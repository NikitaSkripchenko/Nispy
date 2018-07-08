#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include <string.h>
static char buffer[2048];

char* readline(char* prompt){
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}
void add_history(char* unused){}
#else
#include <editline/readline.h>
#endif

//Build a parser

// mpc_parser_t* Adjective = mpc_or(4,
//  mpc_sym("wow"), mpc_sym("many"),
//  mpc_sym("so"), mpc_sym("such")
// );
// 
// mpc_parser_t* Noun = mpc_or)(5,
// 	mpc_sym("lisp"), mpc_sym("language"),
// 	mpc_sym("book"), mpc_sym("build"),
// 	mpc_sym("c")
// );
// 
// mpc_parser_t* Phrase = mpc_and(2, mpcf_strfold, Adjective, Noun, free);
// 
// mpc_parser_t* Doge = mpc_many(mpcf_strfold, Phrase);

mpc_parcer_t * BeforePoint = mpc_new("before");
mpc_parcer_t* AfterPoint = mpc_new("after");
mpc_parcer_t* Whole = mpc_new("whole");

mpca_lang(MPCA_LANG_DEFAULT, 
"\
	before: \"[0-9]\"\; \
	after:  \"[0-9]\"; \
	whole:  <before> \.\ <after>*;\",
	BeforePoint,AfterPoint, Whole);

mpc_parcer_t* Http = mpc_new("http");
mpc_parcer_t * Web = mpc_new("www");
mpc_parcer_t * Domain = mpc_new("domain");
mpc_parcer_t * Host = mpc_new("host");
mpc_parcer_t* Site = mpc_new("site");
mpca_lang(MPCA_LANG_DEFAULT,
"\
	http : \"http://\" | \"https://\";\
	web : \"web.\"; \
	domain: \_t*"buildyourownlisp\"; \
	host: \".com\" ;\
	site: <http> <web> <domain> <host>",
	Http, Web, Domain, Host, Site);
	
	mpc_cleanup(4, Http, Web, Domain, Host);
	
	
mpc_parcer_t * Adjective = mpc_new("adjective");
mpc_parcer_t * Noun = mpc_new("noun");
mpc_parcer_t *  = mpc_new("adjective");
mpc_parcer_t * Phrase = mpc_new("phrase");
mpc_parcer_t * Doge = mpc_new("doge");

mpca_lang(MPCA_LANG_DEFAULT,
"\
	adjective : \"wow\" | \"many\" \
			  | \"so\"  | \"such\"; \
	noun	  :	\"lisp\"| \"language\" \
			  | \"book\"| \"build\" \| \"c\"; \
	phrase    : <adjective> <noun>; \
	doge 	  : <phrase>*;\ ",
	Adjective, Noun, Phrase, Doge);
	
	mpc_cleanup(4, Adjective, Noun, Phrase, Doge);
	
	
	
int main(int argc, char** argv){
	puts("Nisp version 0.0.0.0.1");
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	printf("%d-%d-%d %d:%d:%d\n",tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour,tm.tm_min, tm.tm_sec);
	puts("Press Ctrl+c to exit\n");
	while(1){
		char* input = readline("lispy> ");
		add_history(input);
		printf("Echo: %s\n", input);
		free(input);
	}
	return 0;
}
