/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/

%defines
%pure-parser
%lex-param   { yyscan_t scanner }
%parse-param { yyscan_t scanner }
%parse-param { ff_filter_t *filter }
%name-prefix = "v2_"

%{
	#include <stdio.h>
	#include "libnf_internal.h"
	#include "libnf.h"
	#include "ff_filter.h"
	#include "ff_filter_internal.h"

	#define YY_EXTRA_TYPE ff_filter_t

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
	NOT expr	 		{ $$ = ff_filter_new_node(scanner, filter, NULL, FF_OP_NOT, $2); if ($$ == NULL) { YYABORT; }; }
	| expr AND expr	 	{ $$ = ff_filter_new_node(scanner, filter, $1, FF_OP_AND, $3); if ($$ == NULL) { YYABORT; }; }
	| expr OR expr	 	{ $$ = ff_filter_new_node(scanner, filter, $1, FF_OP_OR, $3); if ($$ == NULL) { YYABORT; }; }
	| LP expr RP 		{ $$ = $2; }
	| STRING STRING		{ $$ = ff_filter_new_leaf(scanner, filter, $1, FF_OP_EQ, $2); if ($$ == NULL) { YYABORT; } }
	| STRING EQ STRING	{ $$ = ff_filter_new_leaf(scanner, filter, $1, FF_OP_EQ, $3); if ($$ == NULL) { YYABORT; } }
	| STRING LT STRING	{ $$ = ff_filter_new_leaf(scanner, filter, $1, FF_OP_LT, $3); if ($$ == NULL) { YYABORT; } }
	| STRING GT STRING	{ $$ = ff_filter_new_leaf(scanner, filter, $1, FF_OP_GT, $3); if ($$ == NULL) { YYABORT; } }
	;

%%

