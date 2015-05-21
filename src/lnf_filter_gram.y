
%pure-parser
%lex-param   { yyscan_t scanner }
%parse-param { yyscan_t scanner }

%{
	#include <stdio.h>
	#include "libnf.h"
	#include "lnf_filter.h"
	typedef void* yyscan_t;	
	void yyerror(yyscan_t yyscanner, char *);

typedef struct lnf_filter_entry_s {
	int datatype;	/* rval data type - LNF_STRING, LNF_UINT64, LNF_DOUBLE */
	char *data;		/* rval data pointer */
	int oper;		/* operation type EQ LT GT */
} lnf_filter_entry_t;

%}

%union {
	uint64_t	t_uint;
	double		t_double;
	char 		*t_string;
	lnf_filter_entry_t entry;
};

%token AND OR NOT 
%token EQ LT GT  
%token LP RP
%token <t_uint> NUMBER
%token <t_string> STRING
%token <entry> cmpval expr program

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
	| STRING EQ cmpval	{ printf("comparsion EQ\n"); lnf_filter_new_leaf($1, EQ, $2.type); }
	| STRING LT cmpval	{ printf("comparsion LT\n"); }
	| STRING GT cmpval	{ printf("comparsion GT\n"); }
	;

cmpval:
	STRING 				{ printf("STRING: %s\n", $1); }
	| NUMBER	 		{ $$.data = $1; $$.datatype = LNF_UINT64; printf("INTEGER: %d\n", $1); }
	

%%

//void * filterp;
/*
void yyerror(yyscan_t yyscanner, char *s) {
	fprintf(stderr, "%s\n", s);
}
*/
