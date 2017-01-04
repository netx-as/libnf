
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


/* implementation of timedlock for platform where the function is not available */
int lnf_ring_lock(lnf_ring_t *ring) {
	int retcode;
	int counter = 0;
	
	while ((retcode = pthread_mutex_trylock(&ring->shm->lock)) == EBUSY) {

		/* increment stuck counter */
		if (counter++ > LNF_RING_STUCK_LIMIT) {
			ring->stuck_counter++;
			return retcode;
		}

/*	
		if (counter > 10 && counter % 100) {	
			printf("XXX STUCK %d\n", counter);
		}
*/
		usleep(LNF_RING_BLOCK_USLEEP);

	}
 
	return retcode;
}

/* initialise ring buffer */
int lnf_ring_init(lnf_ring_t **ringp, char *name, int flags) {

    lnf_ring_t *ring;
	pthread_mutexattr_t mutex_attr;
	int ringbuf_init = 0;
	size_t shm_size = sizeof(lnf_ring_shm_t) + sizeof(lnf_ring_entry_t) * LNF_RINGBUF_SIZE;

    ring = calloc(1, sizeof(lnf_ring_t));

    if (ring == NULL) {
        return LNF_ERR_NOMEM;
    }

	strncpy(ring->shm_name, name, LNF_MAX_STRING);

	/* force init - unlink shared memmory */
	if (flags & LNF_RING_FORCE_INIT) {
		if (shm_unlink(ring->shm_name) < 0) {
		//	printf("XXX Can't unlink shm %s (errno: %d, %s)", name, errno, strerror(errno));
		}
	}

	if (flags & LNF_RING_FORCE_RELEASE) {
		ring->force_release = 1;
	}

	if (! (flags & LNF_RING_NO_BLOCK) ) {
		ring->blocking = 1;
	}

	/* try to open and map shared memory object */
	ring->fd = shm_open(name, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
	
	if (ring->fd > 0) {
		ftruncate(ring->fd, shm_size);
		ringbuf_init = 1;
	} else {
		ring->fd = shm_open(name, O_RDWR, S_IRUSR|S_IWUSR);
	}
	

	if (ring->fd < 0) {
		lnf_seterror("Can't open shm %s (errno: %d, %s)", name, errno, strerror(errno));
		free(ring);
		return LNF_ERR_OTHER_MSG;
	}

	ring->shm = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, ring->fd, 0);

	if (ring->shm == NULL) {
		free(ring);
		return LNF_ERR_OTHER;
	}


	if (ringbuf_init) {
		pthread_mutexattr_init(&mutex_attr);
		pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&ring->shm->lock, &mutex_attr);
	}

	lnf_ring_lock(ring);
	ring->shm->size = LNF_RINGBUF_SIZE;
	ring->shm->conn_count++;
	pthread_mutex_unlock(&ring->shm->lock);
	
	*ringp = ring;

    return LNF_OK;
}

int lnf_ring_next(lnf_ring_t *ring, int index) {

	index++;
	if (index >= ring->shm->size) {
		index = 0;
	}

	return index;
}


/* read record from ring buffer */
int lnf_ring_read(lnf_ring_t *ring, lnf_rec_t *rec) {

	lnf_ring_entry_t *ent;
	int ret;

	ent = &ring->shm->entries[ring->read_pos];

WAIT_READ:
	lnf_ring_lock(ring);
	
	if (ent->status != LNF_RING_ENT_READ || ent->sequence <= ring->last_read_sequence) {

		if (ring->blocking) {
			pthread_mutex_unlock(&ring->shm->lock);
			usleep(LNF_RING_BLOCK_USLEEP);
			goto WAIT_READ;
		} else {
			pthread_mutex_unlock(&ring->shm->lock);
			return LNF_EOF;
		}
	}

	/* set status to read and unlock */
	ent->num_readers += 1;

	pthread_mutex_unlock(&ring->shm->lock);

	/* read data */
	ret = lnf_rec_set_raw(rec, (char *)&ent->data, LNF_MAX_RAW_LEN);

	if (ring->last_read_sequence + 1 != ent->sequence) {
		ring->lost_counter += ent->sequence - ring->last_read_sequence + 1;
	}

	ring->last_read_sequence = ent->sequence;


	/* decrease number of readers */
	lnf_ring_lock(ring);
	ent->num_readers -= 1;
	pthread_mutex_unlock(&ring->shm->lock);

	if (ret == LNF_OK) {
		ring->read_pos = lnf_ring_next(ring, ring->read_pos);
		ring->total_counter++;
	} 

	return ret;
}

/* write record to ing buffer */
int lnf_ring_write(lnf_ring_t *ring, lnf_rec_t *rec) {

	lnf_ring_entry_t *ent;
	size_t size;
	int ret;
	int stuck_counter = 0;

WAIT_READERS:
	lnf_ring_lock(ring);

	ent = &ring->shm->entries[ring->shm->write_pos];

	if (ent->num_readers > 0) { 

		/* increment stuck counter */
		if (stuck_counter++ > LNF_RING_STUCK_LIMIT) {
			ring->stuck_counter++;
			ent->num_readers = 0;
		}

		pthread_mutex_unlock(&ring->shm->lock);
		usleep(LNF_RING_BLOCK_USLEEP);
		goto WAIT_READERS;
	}

	ent->status = LNF_RING_ENT_WRITE;

	pthread_mutex_unlock(&ring->shm->lock);

	/* write data to buffer */
	ret = lnf_rec_get_raw(rec, LNF_REC_RAW_TLV, (char *)&ent->data, LNF_MAX_RAW_LEN, &size);
	ret = LNF_OK;

	if (ret != LNF_OK) {

		lnf_ring_lock(ring);
		ent->status = LNF_RING_ENT_EMPTY;
		pthread_mutex_unlock(&ring->shm->lock);

		return ret;
	} 

	/* change status to ready */
	lnf_ring_lock(ring);

	ent->status = LNF_RING_ENT_READ;

	ring->shm->last_write_sequence++;
	ent->sequence = ring->shm->last_write_sequence;
	ring->shm->write_pos = lnf_ring_next(ring, ring->shm->write_pos);


	pthread_mutex_unlock(&ring->shm->lock);

	return LNF_OK;
}

/* get ring info */
int lnf_ring_info(lnf_ring_t *ring, int info, void *data, size_t size) {

	size_t reqsize;
	char buf[LNF_INFO_BUFSIZE];

	/* for NULL lnf_mem */
	switch (info) {
		case LNF_RING_TOTAL:
			*((uint64_t *)buf) = ring->total_counter;
            reqsize = sizeof(uint64_t);
			break;
		case LNF_RING_LOST:
			*((uint64_t *)buf) = ring->lost_counter;
            reqsize = sizeof(uint64_t);
			break;
		case LNF_RING_STUCK:
			*((uint64_t *)buf) = ring->stuck_counter;
            reqsize = sizeof(uint64_t);
			break;
	}

	/* the requested item was one of the above */
	if ( reqsize != 0 ) {
		if ( reqsize <= size ) {
			memcpy(data, buf, reqsize);
			return LNF_OK;
		} else {
			return LNF_ERR_NOMEM;
		}
	}


	if ( reqsize != 0 ) {
		if ( reqsize <= size ) {
			memcpy(data, buf, reqsize);
			return LNF_OK;
		} else {
			return LNF_ERR_NOMEM;
		}
	} else {
		return LNF_ERR_OTHER;
	}
}


/* release all resources allocated by ring buffer */
void lnf_ring_free(lnf_ring_t *ring) {

	int conn_count;

	if (ring == NULL) {
		return;
	}

	lnf_ring_lock(ring);
	ring->shm->conn_count--;

	conn_count = ring->shm->conn_count;

	pthread_mutex_unlock(&ring->shm->lock);

	if (ring->shm != NULL) {
		munmap(ring->shm, sizeof(lnf_ring_shm_t) + sizeof(lnf_ring_entry_t) * LNF_RINGBUF_SIZE);
	}

	if (conn_count <= 0) { 
		shm_unlink(ring->shm_name);
	}
	free(ring);
}

