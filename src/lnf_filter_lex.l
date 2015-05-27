
%option outfile="lex.yy.c"
%option prefix="v2_"
%option reentrant bison-bridge
%option noyywrap 

digit 	[0-9]
letter	[A-Za-z.:/]
ws		[ \t]


%{
	#include <stdlib.h>
	#include <string.h>
	#include "libnf_internal.h"
	#include "libnf.h"
	#include "lnf_filter_gram.h"
	#include "lnf_filter.h"
//    YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t yyscanner);
//	int yyparse(yyscan_t yyscanner, lnf_filter_t *filter);
//	int yylex(YYSTYPE *yylval, void *scanner);
//	int yylineno;
%}

%%

and|"&&"		{ return AND; }
or|"||"			{ return OR; }
not|"!"			{ return NOT; }
"="|"=="|eq		{ return EQ; }
">"|gt			{ return GT; }
"<"|lt			{ return LT; }
"("				{ return LP; }
")"				{ return RP; }

"src"{ws}+"ip"	{
					strncpy(yylval->string, "srcip", LNF_MAX_STRING - 1);
					return STRING;
				}
"src"{ws}+"net"	{
					strncpy(yylval->string, "srcip", LNF_MAX_STRING - 1);
					return STRING;
				}
"src"{ws}+"port" {
					strncpy(yylval->string, "srcport", LNF_MAX_STRING - 1);
					return STRING;
				}
({letter}|{digit})+	{
					strncpy(yylval->string, yytext, LNF_MAX_STRING - 1);
					return STRING;
				}
[\n]			{
					//printf("konec radku.\n");
					;
				}
#.*             { ; }	//comments
{ws}+			{ ; }	//whitespace 

.				v2_error(yyscanner, NULL, "invalid character");

%%

void v2_error(yyscan_t yyscanner, lnf_filter_t *filter, char *s) {
	fprintf(stderr, "%s\n", s);
}

