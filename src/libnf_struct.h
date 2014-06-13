

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

