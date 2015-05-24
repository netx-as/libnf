
#include "config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "lnf_filter.h"
#include "libnf_internal.h"
#include "libnf.h" 
#include "lnf_filter_gram.h"

/* convert string into uint64_t */
/* FIXME: also converst string with units (64k -> 64000) */
int str_to_uint(char *str, int type, void **res, int *vsize) {

	uint64_t tmp64;
	uint32_t tmp32;
	uint32_t tmp16;
	uint32_t tmp8;
	void *tmp, *ptr;

	tmp64 = atol(str);

	switch (type) {
		case LNF_UINT64:
				*vsize = sizeof(uint64_t);
				tmp = &tmp64;
				break;
		case LNF_UINT32:
				*vsize = sizeof(uint32_t);
				tmp32 = tmp64;
				tmp = &tmp32;
				break;
		case LNF_UINT16:
				*vsize = sizeof(uint16_t);
				tmp16 = tmp64;
				tmp = &tmp16;
				break;
		case LNF_UINT8:
				*vsize = sizeof(uint16_t);
				tmp8 = tmp64;
				tmp = &tmp8;
				break;
		default: return 0;
	}

	ptr = malloc(*vsize);

	if (ptr == NULL) {
		return 0;
	}

	memcpy(ptr, tmp, *vsize);

	*res = ptr;

	return 1;	
	
}


/* add leaf entry into expr tree */
lnf_filter_node_t* lnf_filter_new_leaf(yyscan_t scanner, char *fieldstr, lnf_oper_t oper, char *valstr) {
	int field, numbits, numbits6;
	lnf_filter_node_t *node;
	uint64_t tmp64;
	uint32_t tmp32;
	uint16_t tmp16;
	uint8_t tmp8;

	printf("Adding node: %s | %d | %s\n", fieldstr, oper, valstr);

	/* fieldstr is set - trie to find field id and relevant _fget function */
	if ( fieldstr != NULL ) {
		field = lnf_fld_parse(fieldstr, &numbits, &numbits6); 
		if (field == LNF_FLD_ZERO_) {
			yyerror(scanner, NULL, "Unknown field"); 
			//lnf_seterror("Unknown field %s", fieldstr); 
			printf("error\n");
			return NULL;
		}
	}

	printf("OK\n");
	node = malloc(sizeof(lnf_filter_node_t));

	if (node == NULL) {
		return NULL;
	}

	node->type = lnf_fld_type(field);
	node->field = field;
	node->oper = oper;

	/* determine field type and assign data to lvalue */
	switch (lnf_fld_type(field)) {

		case LNF_UINT64:
		case LNF_UINT32:
		case LNF_UINT16:
		case LNF_UINT8:
				if (str_to_uint(valstr, lnf_fld_type(field), &node->value, &node->vsize) == 0) {
					printf("Can't convert '%s' for filed %s into numeric value\n", valstr, fieldstr);
					return NULL;
				}
				break;
	}

	node->left = NULL;
	node->right = NULL;

	return node;
}

/* add node entry into expr tree */
lnf_filter_node_t* lnf_filter_new_node(yyscan_t scanner, lnf_filter_node_t* left, lnf_oper_t oper, lnf_filter_node_t* right) {
	int field, numbits, numbits6;
	lnf_filter_node_t *node;

	printf("Adding (join) node: %p %d %p \n", left, oper, right);

	node = malloc(sizeof(lnf_filter_node_t));

	if (node == NULL) {
		return NULL;
	}

	node->vsize = 0;
	node->type = 0;
	node->oper = oper;

	node->left = left;
	node->right = right;

	return node;
}

/* evaluate node in tree or proces subtree */
/* return 0 - false; 1 - true; -1 - error  */
int lnf_filter_eval(lnf_filter_node_t *node, lnf_rec_t *rec) {
	int buf[LNF_MAX_STRING];
	int left, right, res;

	if (node == NULL) {
		return -1;
	}

	/* go deeper into tree */
	if (node->left != NULL ) { left = lnf_filter_eval(node->left, rec); }

	/* do not evaluate if the result is ovious */
	if (node->oper == LNF_OP_NOT)              { return !left; };
	if (node->oper == LNF_OP_OR  && left == 1) { return 1; };
	if (node->oper == LNF_OP_AND && left == 0) { return 0; };

	if (node->right != NULL ) { right = lnf_filter_eval(node->right, rec); }

	switch (node->oper) {
		case LNF_OP_NOT: return !right; break;
		case LNF_OP_OR:  return left || right; break;
		case LNF_OP_AND: return left && right; break;
	}

	/* operations on leaf -> comparsion */
	lnf_rec_fget(rec, node->field, &buf);

	/* simple comparsion */
	if (node->oper == LNF_OP_EQ || node->oper == LNF_OP_NE) {
//		uint16_t *tmp = node->value;
		res = memcmp(&buf, node->value, node->vsize);
//		printf("XXX 2 %d right:%d, vsize:%d\n", res, *tmp, node->vsize);
		if (node->oper == LNF_OP_EQ) {
			return (res == 0); /* EQ */
		} else {
			return (res != 0); /* NE */
		}
	}

	printf("XXX 10\n");
	return -1;
}


/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec) {

	return lnf_filter_eval(filter->root, rec);

}

/* release all resources allocated by filter */
void lnf_filter_free(lnf_filter_t *filter) {

	if (filter == NULL) {
		return;
	}

	free(filter);
}

