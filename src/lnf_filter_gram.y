
%pure-parser
%lex-param   { yyscan_t scanner }
%parse-param { yyscan_t scanner }

%{
	#include <stdio.h>
	#include "libnf.h"
	#include "lnf_filter.h"
	typedef void* yyscan_t;	
	void yyerror(yyscan_t yyscanner, char *);
%}

%union {
	int		 	value;
	char 		*string;
};

%token AND OR NOT 
%token EQ LT GT  
%token LP RP
%token <value> NUMBER
%token <string> STRING

%left	OR
%left	AND
%left 	NOT

%%

program:
	program expr 	 	{ printf("PROGRAM:\n"); }
	|
	;

expr:
	NOT expr 			{ printf("EXPR: ! \n"); }
	| expr AND expr 	{ printf("EXPR: + \n"); }
	| expr OR expr	 	{ printf("EXPR: - \n"); }
	| LP expr RP 		{ printf("EXPR: () \n"); }
	| STRING cmpval		{ printf("comparsion EQ - \n"); }
	| STRING EQ cmpval	{ printf("comparsion EQ\n"); }
	| STRING LT cmpval	{ printf("comparsion LT\n"); }
	| STRING GT cmpval	{ printf("comparsion GT\n"); }
	;

cmpval:
	STRING 				{ printf("STRING: %s\n", $1); }
	| NUMBER	 		{ printf("INTEGER: %d\n", $1); }
	

%%

//void * filterp;
/*
void yyerror(yyscan_t yyscanner, char *s) {
	fprintf(stderr, "%s\n", s);
}
*/
