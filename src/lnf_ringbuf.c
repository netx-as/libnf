
#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "libnf.h" 
#include "lnf_ringbuf.h" 

//pthread_mutex_t lnf_nfdump_filter_match_mutex;    /* mutex for operations match filter  */

// http://stackoverflow.com/questions/2584678/how-do-i-synchronize-access-to-shared-memory-in-lynxos-posix
// http://stackoverflow.com/questions/19464102/posix-shared-memory-initialization

/* initialise ring buffer */
int lnf_ring_init(lnf_ring_t **ringp) {

    lnf_ringbuf_t *ring;

    ring = malloc(sizeof(lnf_ring_t));

    if (ring == NULL) {
        return LNF_ERR_NOMEM;
    }


	*ringp = ring;

    return LNF_OK;
}


/* read record from ring buffer */
int lnf_ring_read(lnf_ring_t *ring, lnf_rec_t *rec) {

}

/* write record to ing buffer */
int lnf_ring_write(lnf_ring_t *rin, lnf_rec_t *rec) {

}


/* release all resources allocated by ring buffer */
void lnf_ring_free(lnf_ring_t *ringbuf) {

	if (ring == NULL) {
		return;
	}

	free(ring);
}

