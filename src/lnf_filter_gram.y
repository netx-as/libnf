
%defines
%pure-parser
%lex-param   { yyscan_t scanner }
%parse-param { yyscan_t scanner }
%parse-param { lnf_filter_t *filter }
%name-prefix = "v2_"

%{
	#include <stdio.h>
	#include "libnf_internal.h"
	#include "libnf.h"
	#include "lnf_filter.h"

	#define YY_EXTRA_TYPE lnf_filter_t

%}

%union {
	uint64_t	t_uint;
	double		t_double;
	char 		string[LNF_MAX_STRING];
	void		*node;
};

%token AND OR NOT 
%token EQ LT GT  
%token LP RP
%token <string> STRING
%type <node> expr filter 

%left	OR
%left	AND
%left 	NOT

%%

filter:
	expr 			 	{ filter->root = $1; }
	|					{ filter->root = NULL; }
	;

expr:
	NOT expr	 		{ $$ = lnf_filter_new_node(scanner, NULL, LNF_OP_NOT, $2); if ($$ == NULL) { YYABORT; }; }
	| expr AND expr	 	{ $$ = lnf_filter_new_node(scanner, $1, LNF_OP_AND, $3); if ($$ == NULL) { YYABORT; }; }
	| expr OR expr	 	{ $$ = lnf_filter_new_node(scanner, $1, LNF_OP_OR, $3); if ($$ == NULL) { YYABORT; }; }
	| LP expr RP 		{ $$ = $2; }
	| STRING STRING		{ $$ = lnf_filter_new_leaf(scanner, $1, LNF_OP_EQ, $2); if ($$ == NULL) { YYABORT; } }
	| STRING EQ STRING	{ $$ = lnf_filter_new_leaf(scanner, $1, LNF_OP_EQ, $3); if ($$ == NULL) { YYABORT; } }
	| STRING LT STRING	{ $$ = lnf_filter_new_leaf(scanner, $1, LNF_OP_LT, $3); if ($$ == NULL) { YYABORT; } }
	| STRING GT STRING	{ $$ = lnf_filter_new_leaf(scanner, $1, LNF_OP_GT, $3); if ($$ == NULL) { YYABORT; } }
	;

%%

