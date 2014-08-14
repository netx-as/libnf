

#define _HAVE_LIBNF_STRUCT_H_ 1
/* 
* This .h file is also used to create some parts of libnf documentation 
* the text after the macro pod<double dot> will be placed into separate file libnf.h.pod 
* and then is included into basic libnf documentation 
*/

#include <stdio.h>
#include <stdlib.h>
#include <nffile.h>
#include <rbtree.h>
#include <nftree.h>
#include <nfx.h>
#include "bit_array.h"
#include "hash_table.h"


/* list of maps used in file taht we create */
typedef struct lnf_map_list_s {
	bit_array_t				 bit_array;
	extension_map_t			*map;
	struct lnf_map_list_s	*next;
} lnf_map_list_t;


/* structure representing a filter */
typedef struct lnf_filter_s {
	FilterEngine_data_t	*engine;
} lnf_filter_t;


/* structure representing single record */
/* it contains two fields. Nfdump'ps master record */
/* and bit array representing set of fields that are activated in the master ricord */
/* master record or bit array SHOULD NEVER be used in the direct form */
/* the structure of the field might change across versions and there is */
/* a big probability that any future version wouldn't be compadible */
typedef struct lnf_rec_s {
	master_record_t *master_record;		/* reference to master record */
	bit_array_t *extensions_arr;		/* list of extensions available in the record */
} lnf_rec_t;


/* structure representing single nfdump file */
/* every nfdump file which is open for either read or write is identified by this handle */
typedef struct lnf_file_s {
	nffile_t 				*nffile;				/* ptr to nfdump's nffile structure */
	int 					flags;
	int 					blk_record_remains;		/* records to be processed in the current block */
	extension_map_list_t 	*extension_map_list;	/* ptr to extmap structure */
	lnf_map_list_t			*lnf_map_list;			/* internal list of maps (used by lnf) */
	int						max_num_extensions;		/* the max number of extensions */
	common_record_t 		*flow_record;			/* ptr to buffer/next record */
//	master_record_t			*master_record;
//	lnf_rec_t				*lnf_rec;				/* temporary */
//	bit_array_t				extensions_arr;			/* structure initialised at the beginging to store bitt array of extensions from last record */
	uint64_t                processed_blocks;
	uint64_t                skipped_blocks;
	uint64_t                processed_records;
	uint64_t                current_processed_blocks;
	uint64_t                processed_bytes;
} lnf_file_t;


extension_map_t * lnf_lookup_map(lnf_file_t *lnf_file, bit_array_t *ext );

/* nfdump uses LogError - we map it to lnf_seterror */
void lnf_seterror(char *format, ...);



/****************************************************************/
/* Mem heap section - Aggergations, Sorting                     */
/****************************************************************/

/* field list for aggregation and sorting */
typedef struct lnf_fieldlist_s {
	int field;
	int size;			/* size of the field ib bytes */
	int offset;			/* offset from the beggining of the record */
	int aggr_flag;
	int sort_flag;
	int numbits;
	int numbits6;
	void *next;
} lnf_fieldlist_t;


/* general memheap structure */
typedef struct lnf_mem_s {
	lnf_fieldlist_t *key_list;		/* list of fields with key values */
	int	key_len;
	lnf_fieldlist_t *val_list;		/* list of fields with values (counters etc.) */
	int	val_len;
//	lnf_fieldlist_t *sort_list;		/* list of fields to sort */
//	int	sort_len;
	int  hash_table_init;			/* is the hash table initialised ? */
	unsigned long hash_index;		/* row index for reading */
	hash_table_t hash_table;
	int rearranged;					/* is the final hash table rearranged ? */
	int sort_field;					/* field identification for sorting */
	int sort_offset;				/* offset in the record field */
	int sort_flags;					/* search sort field in ket or aggregated value */
#define LNF_SORT_FLD_NONE 0x0
#define LNF_SORT_FLD_IN_KEY 0x1
#define LNF_SORT_FLD_IN_VAL 0x2
} lnf_mem_t;


#define LNF_MAX_KEY_LEN 512			/* maximum key length for hash table */
#define LNF_MAX_VAL_LEN 256			/* maximum aggregated values length for hash table */

int lnf_filedlist_add(lnf_fieldlist_t **list, lnf_fieldlist_t *snode, int *sizep, int maxsize, int *offset);
int lnf_mem_done(lnf_mem_t *lnf_mem);

