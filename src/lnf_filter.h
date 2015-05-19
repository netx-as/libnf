
//#define OP_EQ	0x01
//#define OP_LE	0x01

int lnf_filter2_init(lnf_filter_t **filterp, char *expr);
void lnf_filter2_free(lnf_filter_t *filter);


typedef struct lnf_filter_node_s {

	char *cmpval;				/* pointer to allocated data to compare from filter */
	char *oper;					/* pointer to compare function */

	struct expr_node_s *left;
	struct expr_node_s *right;

} lnf_filter_node_t;

