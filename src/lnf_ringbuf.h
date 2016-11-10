
#ifndef _LNF_RINGBUF_H
#define _LNF_RINGBUF_H

#include <libnf_internal.h>
#include <libnf.h>

#define LNF_RINGBUF_SIZE 1024	/* number of items in ring buffer */

/* status of entry in ring buffer */
typedef enum { 
	LNF_RING_ENT_EMPTY = 0x0,	/* entry is empty */
	LNF_RING_ENT_WRITE = 0x1,	/* data is being written to entry */
	LNF_RING_ENT_READY = 0x2,	/* data is ready to read  */
} lnf_ring_entry_status_t;


/* representation of one entry in ring buffer */
typedef struct lnf_ring_entry_s {

	lnf_ring_entry_status_t status; 
	long sequence; 
	char* data[LNF_MAX_RAW_LEN + 1];
	
} lnf_ring_entry_t;


/* structures in sharred memmory */
typedef struct lnf_ring_shm_s {

	pthread_mutex_t lock;			/* shm mutex */
	int last_write_sequence;		/* last sequence number of written record */
	int size;						/* number of record in ring buffer */
	int first_pos;					/* reference to first written record */
	int last_pos;					/* reference to last written record */
	lnf_ring_entry_t entries[];		/* entires of ring buffer */

} lnf_ring_shm_t;


typedef struct lnf_ring_s {

//	char filename[PATH_MAX];		/* filename */
	int last_read_sequence; 		/* sequence of last read record */
	int fd;							/* file descriptor to shared file */
	lnf_ring_shm_t *shm_ring;		/* pointer to shared memmory area */
	 		
} lnf_ring_t;


#endif /* _LNF_RINGBUF_H */

