/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _LIBNF_INTERNAL_H
#define _LIBNF_INTERNAL_H


#define _HAVE_LIBNF_STRUCT_H_ 1
/* 
* This .h file is also used to create some parts of libnf documentation 
* the text after the macro pod<double dot> will be placed into separate file libnf.h.pod 
* and then is included into basic libnf documentation 
*/

#include "config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <nffile.h>
#include <rbtree.h>
#include <nftree.h>
#include <nfx.h>
#include <nfxstat.h>
#include <bookkeeper.h>
#include <collector.h>
#ifdef LNF_THREADS
#include <pthread.h>
#endif
#include "bit_array.h"
#include "hash_table.h"
#include "ffilter.h"

#ifndef HAVE_HTONLL
#ifdef WORDS_BIGENDIAN
#	define ntohll(n)	(n)
#	define htonll(n)	(n)
#else
#	define ntohll(n)	(((uint64_t)ntohl(n)) << 32) + ntohl((n) >> 32)
#	define htonll(n)	(((uint64_t)htonl(n)) << 32) + htonl((n) >> 32)
#endif
#define HAVE_HTONLL 1
#endif

#define LNF_DEFAULT_EXPORTER_VERSION 9


/* list of maps used in file taht we create */
typedef struct lnf_map_list_s {
	bit_array_t				 bit_array;
	extension_map_t			*map;
	struct lnf_map_list_s	*next;
} lnf_map_list_t;



/* structure representing a filter */
typedef struct lnf_filter_s {
	FilterEngine_data_t	*engine;
	int					v2filter; /* is V2 - libnf only fiter */

	/* structures for new filter */
	//lnf_filter_node_t	*root;
	//void				*root;
	ff_t				*ff_filter;

} lnf_filter_t;


/* structure representing single record */
/* it contains two fields. Nfdump's master record */
/* and bit array representing set of fields that are activated in the master ricord */
/* master record or bit array SHOULD NEVER be used in the direct form */
/* the structure of the field might change across versions and there is */
/* a big probability that any future version wouldn't be compadible */
typedef struct lnf_rec_s {
	master_record_t *master_record;		/* reference to master record */
	bit_array_t *extensions_arr;		/* list of extensions available in the record */
	generic_exporter_t *exporter;		/* exporter information */
	generic_sampler_t *sampler;			/* sampler information */
#define LNF_REC_EXPORTER 0x1			/* is exporter set? */
#define LNF_REC_SAMPLER 0x2				/* is sampler set ? */
	int flags;
	uint32_t sequence_failures;			/* sequence faulures for exporter information */
	void **field_data;					/* list of pointers to data field */
										/* data field is represented by pointer */
										/* field_data[field] */
} lnf_rec_t;


/* representation of the one element for raw/binnary represenataion of the record */
typedef struct lnf_rec_raw_entry_s {
	uint16_t field;			/* field ID */
	uint16_t data_size;		/* size of data section */
	char 	 data[0];		/* data with variable length */
} lnf_rec_raw_entry_t;



/* Raw/binnary TLV representation of the record. Platform independend format */
/* for transfering records across non compatible platforms */
typedef struct lnf_rec_raw_s {
	uint8_t version;			/* version of the raw record for future extension */
	uint16_t size;				/* size of data section */
	lnf_rec_raw_entry_t entires[0];	/* data enties */
} lnf_rec_raw_t;


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
	char					*v1convert_buffer;		/* convert buffer for V1 record types */
//	master_record_t			*master_record;
//	lnf_rec_t				*lnf_rec;				/* temporary */
//	bit_array_t				extensions_arr;			/* structure initialised at the beginging to store bitt array of extensions from last record */
	uint64_t                processed_blocks;
	uint64_t                skipped_blocks;
	uint64_t                processed_bytes;
	char					*filename;				/* name of open file (for LOOP mode) */
	ino_t					inode;					/* inode of open file (for LOOP mode) */
	generic_exporter_t		*exporters;				/* linked list of exporters */
	generic_sampler_t		*samplers;				/* linked list of samplers */
	uint32_t				num_exporters;
} lnf_file_t;


extension_map_t * lnf_lookup_map(lnf_file_t *lnf_file, bit_array_t *ext );
int lnf_read_record(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec);

/* nfdump uses LogError - we map it to lnf_seterror */
void lnf_seterror(char *format, ...);



/****************************************************************/
/* Mem heap section - Aggergations, Sorting                     */
/****************************************************************/

/* define aggregation function prototype */
/* gets *a and *b and set result to *a */
typedef void (*lnf_mem_aggr_func_t)(char *a, char *b);

/* field list for aggregation and sorting */
typedef struct lnf_fieldlist_s {
	int field;
	int size;			/* size of the field ib bytes */
	int type;			/* field data type */
	int offset;			/* offset from the beggining of the record */
	int aggr_flag;
	int sort_flag;
	int numbits;
	int numbits6;
	lnf_mem_aggr_func_t	aggr_func;	/* function called for aggregation */
	void *next;
} lnf_fieldlist_t;

#define LNF_MAX_THREADS 128			/* maximum threads */

typedef char* lnf_mem_cursor_t;		/* cursor for reading lnf_mem structure */

/* general memheap structure */
typedef struct lnf_mem_s {
	lnf_fieldlist_t *key_list;		/* list of fields with key values */
	int	key_len;
	lnf_fieldlist_t *val_list;		/* list of fields with values (counters etc.) */
	int	val_len;
//	lnf_fieldlist_t *sort_list;		/* list of fields to sort */
//	int	sort_len;
	int fastaggr_mode;				/* mode is fast aggergation is enabled */
	int sort_field;					/* field identification for sorting */
	int sort_offset;				/* offset in the record field */
	int sort_flags;					/* search sort field in ket or aggregated value */
#define LNF_SORT_FLD_NONE 0x0
#define LNF_SORT_FLD_IN_KEY 0x1
#define LNF_SORT_FLD_IN_VAL 0x2
#define LNF_SORT_FLD_IN_FLAGS 0xF
#ifdef LNF_THREADS
	pthread_key_t thread_id_key;	/* key for thread specific id */
	pthread_mutex_t thread_mutex;
#else
	int* thread_id_key;				/* id when threads are not supported */
#endif
	int thread_status[LNF_MAX_THREADS];		/* status of the current thread */
#define LNF_TH_EMPTY 0x0			/* empty thread - no records written so far  */
#define LNF_TH_WRITE 0x1			/* writing record - set after first write */
#define LNF_TH_MERGE 0x2			/* writing done - ready for merge */
#define LNF_TH_MERGING 0x3			/* merging thread */
#define LNF_TH_MERGED 0x4			/* merged thread */
#define LNF_TH_CLEARED 0x4			/* merge process done - hash table destroyed and ID cleared */
	int numthreads;					/* participating number of threads */
	hash_table_t hash_table[LNF_MAX_THREADS];	/* thread specific instance */
//	char * hash_ptr;				/* row pointer for reading */
//	unsigned long read_index;		/* index for nex read */
	lnf_mem_cursor_t *read_cursor;	/* read pointer */
	int rearranged;					/* is the final hash table rearranged */
	int sorted;						/* is the table sorted ? */
	int statistics_mode;			/* is lnf_mem in statistics mode ? (have a pair item in kay) */
	int list_mode;					/* is lnf_mem in linked list mode */
	int nfdump_comp_statistics;		/* is lnf_mem nfdump compatible statistics mode */
	int hash_table_buckets;			/* initial number of buckets in hash table */
	lnf_rec_t	*lnf_rec;			/* temporary lnf_rec entry */
} lnf_mem_t;


#define LNF_MAX_KEY_LEN 512			/* maximum key length for hash table */
#define LNF_MAX_VAL_LEN 256			/* maximum aggregated values length for hash table */

int lnf_mem_thread_init(lnf_mem_t *lnf_mem);
void lnf_filedlist_free(lnf_fieldlist_t *list);
void lnf_clear_bits(char *buf, int buflen, int from);
int lnf_mem_fill_buf(lnf_fieldlist_t *fld, lnf_rec_t *rec, char *buf, int pairset);
void lnf_mem_fill_rec(lnf_fieldlist_t *fld, char *buf, lnf_rec_t *rec);
void lnf_mem_upd_calc_fields(lnf_mem_t *lnf_mem);


int lnf_filedlist_add_or_upd(lnf_fieldlist_t **list, lnf_fieldlist_t *snode, int *sizep, int maxsize, int *offset);
lnf_fieldlist_t* lnf_filedlist_lookup(lnf_fieldlist_t *list, int field);
void lnf_mem_fastaggr_callback(char *key, char *hval, char *uval, void *lnf_mem);
void lnf_mem_aggr_callback(char *key, char *hval, char *uval, void *lnf_mem);
int lnf_mem_sort_callback(char *key1, char *val1, char *key2, char *val2, void *p);
int lnf_mem_done(lnf_mem_t *lnf_mem);
int lnf_mem_read_next(lnf_mem_t *lnf_mem, char **pkey, char **pval);

/* structure for fast aggregation support */
typedef struct lnf_fastaggr_s {
	uint64_t	first;
	uint64_t	last;
	uint64_t	doctets;
	uint64_t	dpkts;
	uint64_t	aggr_flows;
} lnf_fastaggr_t;


/* filter operations - callbacks */
ff_error_t lnf_ff_lookup_func(ff_t *filter, const char *fieldstr, ff_lvalue_t *lvalue);
ff_error_t lnf_ff_data_func(ff_t *filter, void *rec, ff_extern_id_t id, char **data, size_t *size);


/****************************************************************/
/* Ring buffer section                                          */
/****************************************************************/

#define LNF_MAX_RAW_LEN	1024			/* duplicated in libnf.h */
#define LNF_MAX_STRING 512
#define LNF_RINGBUF_SIZE 4096			/* number of items in ring buffer */
#define LNF_RING_BLOCK_USLEEP 10		/* number of usesc to wait for next record when read is blocked */
#define LNF_RING_STUCK_LIMIT  10000		/* wait for N cycles of LNF_RING_BLOCK_USLEEP to detect dead reader (100ms) */

/* status of entry in ring buffer */
typedef enum { 
	LNF_RING_ENT_EMPTY = 0x0,	/* entry is empty */
	LNF_RING_ENT_WRITE = 0x1,	/* data is being written to entry */
	LNF_RING_ENT_READ = 0x2,	/* data is ready to read  */
} lnf_ring_entry_status_t;


/* representation of one entry in ring buffer */
typedef struct lnf_ring_entry_s {

	lnf_ring_entry_status_t status; 
	int num_readers; 
	long sequence; 
	char* data[LNF_MAX_RAW_LEN + 1];
	
} lnf_ring_entry_t;


/* structures in sharred memmory */
typedef struct lnf_ring_shm_s {

	pthread_mutex_t lock;			/* shm mutex */
	long last_write_sequence;		/* last sequence number of written record */
	int size;						/* number of record in ring buffer */
	int conn_count;					/* number of connected instances */
	int write_pos;					/* reference to first written record */
	lnf_ring_entry_t entries[];		/* entires of ring buffer */

} lnf_ring_shm_t;


typedef struct lnf_ring_s {

	long last_read_sequence; 		/* sequence of last read record */
	int read_pos;			 		/* position of last read entry */
	int fd;							/* file descriptor to shared file */
	int blocking;					/* read is in blocking/ non blocking mode */
	int force_release;				/* unlink shared mmerory - no matter how many process reads it */
	uint64_t stuck_counter;				/* counter of stuck states */
	uint64_t lost_counter;				/* counter of lost records */
	uint64_t total_counter;				/* counter of total records */
	char shm_name[LNF_MAX_STRING];	/* pointer to shared memmory area */
	lnf_ring_shm_t *shm;			/* pointer to shared memmory area */
	 		
} lnf_ring_t;

int lnf_ring_next(lnf_ring_t *ring, int index);
int lnf_ring_lock(lnf_ring_t *ring);

#endif /* _LIBNF_INTERNAL_H */


