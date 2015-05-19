
#include "libnf_internal.h"
#include "libnf.h" 
#include "lnf_filter.h"


lnf_filter_node_t* expr_tree_new_leaf(char *fieldstr, int oper, char *data) {
	int field, numbits, numbits6;
	lnf_filter_node_t *node;

	/* fieldstr is set - trie to find field id and relevant _fget function */
	if ( fieldstr != NULL ) {
		field = lnf_fld_parse(fieldstr, &numbits, &numbits6); 
		if (field == LNF_FLD_ZERO_) {
			lnf_seterror("Unknown field %s", fieldstr); 
			return NULL;
		}
	}


	node = malloc(sizeof(lnf_filter_node_t));

	if (node == NULL) {
		return NULL;
	}

//	node->dataval = NULL;
	node->oper = NULL;

	node->left = NULL;
	node->right = NULL;


	return node;

}

