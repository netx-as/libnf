/*

 Copyright (c) 2013-2017, Tomas Podermanski, Imrich Štoffa

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
%lex-param      { yyscan_t scanner }
%lex-param      { ff_t *filter }
%parse-param    { yyscan_t scanner }
%parse-param    { ff_t *filter }
%name-prefix = "ff2_"

%{
	#include <stdio.h>
    #include <string.h>

	#include "ffilter.h"
	#include "ffilter_internal.h"
	#include "ffilter_gram.h"

	#define YY_EXTRA_TYPE ff_t

	void yyerror(yyscan_t scanner, ff_t *filter, char *msg)
	{
	    ff_set_error(filter, msg);
	}

%}

%union {
	uint64_t	t_uint;
	double		t_double;
	char 		string[FF_MAX_STRING];
	void		*node;
};

%token AND OR NOT
%token ANY EXIST
%token EQ LT GT ISSET
%token IN
%token <string> IDENT STRING QUOTED DIR DIR_2 PAIR_AND PAIR_OR
%token <string> BAD_TOKEN

%type <t_uint> cmp
%type <string> field value string consistent_val
%type <node> expr filter
%type <node> list

%left	OR
%left	AND
%left	NOT

%%

filter:
	expr                { filter->root = $1; }
	|                   { filter->root = NULL; }
	;

field:
	IDENT               { strncpy($$, $1, FF_MAX_STRING - 1); }
	| DIR IDENT         { snprintf($$, FF_MAX_STRING - 1, "%s%s", $1, $2); }
	| DIR_2 IDENT       { snprintf($$, FF_MAX_STRING - 1, "%s%s", $1, $2); }
	| DIR_2 DIR IDENT   { snprintf($$, FF_MAX_STRING - 1, "%s%s%s", $1,$2,$3); }
	| PAIR_OR IDENT     { snprintf($$, FF_MAX_STRING - 1, "%c%s", '|', $2); }
	| PAIR_AND IDENT    { snprintf($$, FF_MAX_STRING - 1, "%c%s", '&', $2); }
	;

string:
	IDENT               { strncpy($$, $1, FF_MAX_STRING - 1); /*TRY not to copy, only pass pointer*/ }
	| STRING            { strncpy($$, $1, FF_MAX_STRING - 1); }

value:
	consistent_val      { strncpy($$, $1, FF_MAX_STRING - 1); }
	| string string     { snprintf($$, FF_MAX_STRING - 1, "%s %s", $1, $2); }

consistent_val:
	string              { strncpy($$, $1, FF_MAX_STRING - 1); }
	| QUOTED            { $1[strlen($1)-1] = 0; snprintf($$, FF_MAX_STRING - 1, "%s", &$1[1]); /*Dequote*/}
	;

expr:
	ANY                 { $$ = ff_new_node(scanner, filter, NULL, FF_OP_YES, NULL); if ($$ == NULL) { YYABORT; }; }
	| NOT expr          { $$ = ff_new_node(scanner, filter, NULL, FF_OP_NOT, $2); if ($$ == NULL) { YYABORT; }; }
	| expr AND expr     { $$ = ff_new_node(scanner, filter, $1, FF_OP_AND, $3); if ($$ == NULL) { YYABORT; }; }
	| expr OR expr      { $$ = ff_new_node(scanner, filter, $1, FF_OP_OR, $3); if ($$ == NULL) { YYABORT; }; }
	| '(' expr ')'      { $$ = $2; }
	| EXIST field       { $$ = ff_new_leaf(scanner, filter, $2, FF_OP_EXIST, ""); if ($$ == NULL) { YYABORT; } }
	| field cmp value   { $$ = ff_new_leaf(scanner, filter, $1, $2, $3); if ($$ == NULL) { YYABORT; } }
	| field IN list     { $$ = ff_new_leaf(scanner, filter, $1, FF_OP_IN, $3); if ($$ == NULL) { YYABORT; } }
	| IDENT             { $$ = ff_new_leaf(scanner, filter, $1, FF_OP_NOOP, ""); if ($$ == NULL) { YYABORT; } }
	;

list:
	consistent_val ',' list { $$ = ff_new_mval(scanner, filter, $1, FF_OP_EQ, $3); if ($$ == NULL) { YYABORT; } }
	| consistent_val list    { $$ = ff_new_mval(scanner, filter, $1, FF_OP_EQ, $2); if ($$ == NULL) { YYABORT; } }
	| consistent_val ']'    { $$ = ff_new_mval(scanner, filter, $1, FF_OP_EQ, NULL); if ($$ == NULL) { YYABORT; } }
	;

cmp:
	ISSET       { $$ = FF_OP_ISSET; }
	| EQ        { $$ = FF_OP_EQ; }
	| LT        { $$ = FF_OP_LT; }
	| GT        { $$ = FF_OP_GT; }
	|           { $$ = FF_OP_NOOP; }
	;

%%

