
#ifndef _LNF_FILTER_H
#define _LNF_FILTER_H

#include <libnf_internal.h>
#include <libnf.h>
#include "lnf_filter_gram.h"

/* supported operations */
typedef enum {
	LNF_OP_NOT,
	LNF_OP_OR,
	LNF_OP_AND,
	LNF_OP_EQ,
	LNF_OP_NE,
	LNF_OP_LT,
	LNF_OP_GT,
} lnf_oper_t;

/* node of syntax tree */
typedef struct lnf_filter_node_s {
	int field;                  /* field ID */
	char *value;                /*buffer allocated for data */
	int vsize;                  /* size of data in lvalue */
	int type;                   /* data type for lvalue */
	int numbits;                /* number of bits for IP adres */
	lnf_oper_t oper;            /* operation */

	struct lnf_filter_node_s *left;
	struct lnf_filter_node_s *right;

} lnf_filter_node_t;


/* scanner instance */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif


YY_BUFFER_STATE lnf_filter_yy_scan_string(const char *str, yyscan_t yyscanner);
int lnf_filter_yyparse(yyscan_t yyscanner, lnf_filter_t *filter);
int lnf_filter_yylex(YYSTYPE *yylval, void *scanner);



/* error function for scanner */
void yyerror(yyscan_t yyscanner, lnf_filter_t *filter, char *);

/* conversion from string to numeric/bit value */
int str_to_uint(char *str, int type, char **res, int *vsize);
int str_to_addr(char *str, char **res, int *numbits);

/* add new node into parse tree */
lnf_filter_node_t* lnf_filter_new_leaf(yyscan_t scanner, char *fieldstr, lnf_oper_t oper, char *valstr);
lnf_filter_node_t* lnf_filter_new_node(yyscan_t scanner, lnf_filter_node_t* left, lnf_oper_t oper, lnf_filter_node_t* right);

/* evaluate filter */
int lnf_filter_eval(lnf_filter_node_t *node, lnf_rec_t *rec);

#endif /* _LNF_FILTER_H */

