
#ifndef _LNF_FILTER_H
#define _LNF_FILTER_H

#include <libnf_internal.h>
#include <libnf.h>

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
	lnf_oper_t oper;            /* operation */

	struct lnf_filter_node_s *left;
	struct lnf_filter_node_s *right;

} lnf_filter_node_t;


/* scanner instance */
typedef void* yyscan_t;


/* error function for scanner */
void yyerror(yyscan_t yyscanner, lnf_filter_t *filter, char *);


lnf_filter_node_t* lnf_filter_new_leaf(yyscan_t scanner, char *fieldstr, lnf_oper_t oper, char *valstr);
lnf_filter_node_t* lnf_filter_new_node(yyscan_t scanner, lnf_filter_node_t* left, lnf_oper_t oper, lnf_filter_node_t* right);

#endif /* _LNF_FILTER_H */

