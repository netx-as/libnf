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

%option outfile="lex.yy.c"
%option prefix="ff2_"
%option reentrant bison-bridge
%option noyywrap
%option nounput
%option noinput

digit 	[0-9]
letter	[A-Za-z]
punct   [:/\.\-]


ident [A-Za-z][A-Za-z0-9\-]*

ws	[ \t]

%{
	#include <stdlib.h>
	#include <string.h>
	#include "ffilter_internal.h"
	#include "ffilter_gram.h"
	#include "ffilter.h"
//	#define YY_FATAL_ERROR(msg) (printf("LEXXX> %s\n", msg))
%}

%%

and|"&&"		{ return AND; }
or|"||"			{ return OR; }
not|"!"			{ return NOT; }

exist           { return EXIST; }

"in"{ws}+"["	{ return IN; }

"="|"=="|eq		{ return EQ; }
">"|gt			{ return GT; }
"<"|lt			{ return LT; }
"&"|like	    { return ISSET; }

"("|")"|"["|"]"|"," { return yytext[0]; }

src{ws}+and{ws}+dst|dst{ws}+and{ws}+src { strncpy(yylval->string, "&", FF_MAX_STRING - 1); return PAIR_AND; }
src{ws}+or{ws}+dst|dst{ws}+or{ws}+src   { strncpy(yylval->string, "|", FF_MAX_STRING - 1); return PAIR_OR; }
in|out                              { strncpy(yylval->string, yytext, FF_MAX_STRING - 1); return DIR_2; }
src|prev|ingress                    { strncpy(yylval->string, yytext, FF_MAX_STRING - 1); return DIR; }
dst|next|egress                     { strncpy(yylval->string, yytext, FF_MAX_STRING - 1); return DIR; }

any 				{ return ANY; }

{ident}|\"{ident}([ ]{ident})+\"    { //Identifier or multi word identifier in double quotes
					strncpy(yylval->string, yytext, FF_MAX_STRING - 1);
					return IDENT; }

({letter}|{digit}|{punct})+ {
                    strncpy(yylval->string, yytext, FF_MAX_STRING - 1);
					return STRING; }

\"[^"]+\"   {
                    strncpy(yylval->string, yytext, FF_MAX_STRING - 1);
					return QUOTED; }

#.*         { ; }	//comments
{ws}+       { ; }	//whitespace
[\n]        { ; }	//end of line

.			{ /*fprintf(stderr, "%s: %s\n", "invalid character", yytext); return BAD_TOKEN*/; }

%%

