
%option reentrant bison-bridge
%option noyywrap 

digit 	[0-9]
letter	[A-Za-z]


%{
	#include <stdlib.h>
//	#include "lnf_filter.tab.h"
	#include "libnf.h"
	#include "lnf_filter_gram.h"
	#include "lnf_filter.h"
    YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t yyscanner);
	int yyparse(yyscan_t yyscanner);
	void yyerror(yyscan_t yyscanner, char *);
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
{digit}+		{
					//printf("cislo: %s\n", yytext);
					yylval->value = atoi(yytext);
					return NUMBER;
				}

"src ip"		{
					printf("srcip: %s\n", yytext);
					yylval->string = yytext;
					return STRING;
				}
"src port"		{
					printf("srcport: %s\n", yytext);
					yylval->string = yytext;
					return STRING;
				}
{letter}+		{
					//printf("retezec: %s\n", yytext);
					yylval->string = yytext;
					return STRING;
				}
[\n]			{
					//printf("konec radku.\n");
					;
				}
#.*             { ; }	//comments
[ \t]			{ ; }	//whitespace 

.				yyerror(yyscanner, "invalid character");

%%

/*
int yywrap(void) {
	return 1;
}
*/

void yyerror(yyscan_t yyscanner, char *s) {
	fprintf(stderr, "%s\n", s);
}

int lnf_filter_init(lnf_filter_t **filterp, char *expr) {

    lnf_filter_t *filter;
	yyscan_t scanner;
	YY_BUFFER_STATE buf;

    filter = malloc(sizeof(lnf_filter_t));

    if (filter == NULL) {
        return LNF_ERR_NOMEM;
    }


	yylex_init(&scanner);
	printf("EXR: %s\n", expr);
    buf = yy_scan_string(expr, scanner);
    yyparse(scanner);

    if (buf != NULL) {
        yy_delete_buffer(buf, scanner);
    }

	yylex_destroy(scanner);

	*filterp = filter;

    return LNF_OK;
}

/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec) {

//    filter->engine->nfrecord = (uint64_t *)rec->master_record;

    return  0;
}

void lnf_filter_free(lnf_filter_t *filter) {

	/* TODO nfdump do not have code to release filter :-( */
    if (filter == NULL) {
        return;
    }

    free(filter);
}


