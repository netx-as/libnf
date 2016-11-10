
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
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <libnf_internal.h>
#include <libnf.h>
#include "lnf_ringbuf.h" 

//pthread_mutex_t lnf_nfdump_filter_match_mutex;    /* mutex for operations match filter  */

// http://stackoverflow.com/questions/2584678/how-do-i-synchronize-access-to-shared-memory-in-lynxos-posix
// http://stackoverflow.com/questions/19464102/posix-shared-memory-initialization

/* initialise ring buffer */
int lnf_ring_init(lnf_ring_t **ringp, char *filename, int flags) {

    lnf_ring_t *ring;
	pthread_mutexattr_t mutex_attr;
	size_t shm_size = sizeof(lnf_ring_shm_t) + sizeof(lnf_ring_entry_t) * LNF_RINGBUF_SIZE;

    ring = malloc(sizeof(lnf_ring_t));

    if (ring == NULL) {
        return LNF_ERR_NOMEM;
    }

	/* try to open and map shared memory object */
	ring->fd = shm_open(filename, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
	if (ring->fd == 0) {
		ftruncate(ring->fd, shm_size);	
	} else {
		ring->fd = shm_open(filename, O_RDWR, S_IRUSR|S_IWUSR);
	}

	if (ring->fd != 0) {
		free(ring);
		return LNF_ERR_OTHER;
	}

	ring->shm_ring = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, ring->fd, 0);

	if (ring->shm_ring == NULL) {
		free(ring);
		return LNF_ERR_OTHER;
	}

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&ring->shm_ring->lock, &mutex_attr);
	
	*ringp = ring;

    return LNF_OK;
}


/* read record from ring buffer */
int lnf_ring_read(lnf_ring_t *ring, lnf_rec_t *rec) {

}

/* write record to ing buffer */
int lnf_ring_write(lnf_ring_t *ring, lnf_rec_t *rec) {

}


/* release all resources allocated by ring buffer */
void lnf_ring_free(lnf_ring_t *ring) {

	if (ring == NULL) {
		return;
	}

	if (ring->shm_ring != NULL) {
		munmap(ring->shm_ring, sizeof(lnf_ring_shm_t) + sizeof(lnf_ring_entry_t) * LNF_RINGBUF_SIZE);
	}

//	shm_unlink(ring->filename);
	free(ring);
}

