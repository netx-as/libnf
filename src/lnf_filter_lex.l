
%option reentrant bison-bridge
%option noyywrap 

digit 	[0-9]
letter	[A-Za-z.:]
ws		[ \t]


%{
	#include <stdlib.h>
	#include <string.h>
	#include "libnf_internal.h"
	#include "libnf.h"
	#include "lnf_filter_gram.h"
	#include "lnf_filter.h"
    YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t yyscanner);
	int yyparse(yyscan_t yyscanner, lnf_filter_t *filter);
	int yylex(YYSTYPE *yylval, void *scanner);
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

"src{ws}ip"		{
					printf("srcip: %s\n", yytext);
					strncpy(yylval->string, yytext, LNF_MAX_STRING - 1);
					return STRING;
				}
"src{ws}port"		{
					printf("srcport: %s\n", yytext);
					strncpy(yylval->string, yytext, LNF_MAX_STRING - 1);
					return STRING;
				}
({letter}|{digit})+	{
					//printf("retezec: %s\n", yytext);
					strncpy(yylval->string, yytext, LNF_MAX_STRING - 1);
					return STRING;
				}
[\n]			{
					//printf("konec radku.\n");
					;
				}
#.*             { ; }	//comments
{ws}+			{ ; }	//whitespace 

.				yyerror(yyscanner, NULL, "invalid character");

%%

void yyerror(yyscan_t yyscanner, lnf_filter_t *filter, char *s) {
	fprintf(stderr, "%s\n", s);
}

/* initialise filter */
int lnf_filter_init(lnf_filter_t **filterp, char *expr) {

    lnf_filter_t *filter;
	yyscan_t scanner;
	YY_BUFFER_STATE buf;
	int parseret;

    filter = malloc(sizeof(lnf_filter_t));

    if (filter == NULL) {
        return LNF_ERR_NOMEM;
    }

	filter->root = NULL;

	yylex_init(&scanner);
	printf("EXR: %s\n", expr);
    buf = yy_scan_string(expr, scanner);
    parseret = yyparse(scanner, filter);

    if (buf != NULL) {
        yy_delete_buffer(buf, scanner);
    }

	yylex_destroy(scanner);

	printf("FILTER R: %d %p\n", parseret, filter->root);
	/* error in parsing */
	if (parseret != 0) {
		free(filter);
		return LNF_ERR_OTHER_MSG;
	}

	*filterp = filter;

    return LNF_OK;
}

