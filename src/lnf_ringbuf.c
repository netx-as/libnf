
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

/* initialise ring buffer */
int lnf_ring_init(lnf_ring_t **ringp, char *name, int flags) {

    lnf_ring_t *ring;
	pthread_mutexattr_t mutex_attr;
	size_t shm_size = sizeof(lnf_ring_shm_t) + sizeof(lnf_ring_entry_t) * LNF_RINGBUF_SIZE;

    ring = calloc(1, sizeof(lnf_ring_t));

    if (ring == NULL) {
        return LNF_ERR_NOMEM;
    }

	strncpy(ring->shm_name, name, LNF_MAX_STRING);

	/* force init - unlink shared memmory */
	if (flags & LNF_RING_FORCE_INIT) {
		if (shm_unlink(ring->shm_name) < 0) {
			printf("XXX Can't unlink shm %s (errno: %d, %s)", name, errno, strerror(errno));
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
		printf("XXX SHM new\n");
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

	/* set atributs */
	if (flags & LNF_WRITE) {

	}

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&ring->shm->lock, &mutex_attr);

	pthread_mutex_lock(&ring->shm->lock);
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
	pthread_mutex_lock(&ring->shm->lock);
	
	if (ent->status != LNF_RING_ENT_READ || ent->sequence <= ring->last_read_sequence) {

		if (ring->blocking) {
			pthread_mutex_unlock(&ring->shm->lock);
			usleep(LNF_RING_BLOCK_USLEEP);
			goto WAIT_READ;
		} else {
			return LNF_EOF;
		}
	}

	/* set status to read and unlock */
	ent->num_readers += 1;

	pthread_mutex_unlock(&ring->shm->lock);

	/* read data */
	ret = lnf_rec_set_raw(rec, (char *)&ent->data, LNF_MAX_RAW_LEN);
	ring->last_read_sequence = ent->sequence;

	/* decrease number of readers */
	pthread_mutex_lock(&ring->shm->lock);
	ent->num_readers -= 1;
	pthread_mutex_unlock(&ring->shm->lock);

	if (ret == LNF_OK) {
		ring->read_pos = lnf_ring_next(ring, ring->read_pos);
	} 

	return ret;
}

/* write record to ing buffer */
int lnf_ring_write(lnf_ring_t *ring, lnf_rec_t *rec) {

	lnf_ring_entry_t *ent;
	size_t size;
	int ret;


WAIT_READERS:
	pthread_mutex_lock(&ring->shm->lock);

	ent = &ring->shm->entries[ring->shm->write_pos];

	if (ent->num_readers > 0) { 

		pthread_mutex_unlock(&ring->shm->lock);
		goto WAIT_READERS;
	}

	ent->status = LNF_RING_ENT_WRITE;

	pthread_mutex_unlock(&ring->shm->lock);

	/* write data to buffer */
	ret = lnf_rec_get_raw(rec, LNF_REC_RAW_TLV, (char *)&ent->data, LNF_MAX_RAW_LEN, &size);

	if (ret != LNF_OK) {

		pthread_mutex_lock(&ring->shm->lock);
		ent->status = LNF_RING_ENT_EMPTY;
		pthread_mutex_unlock(&ring->shm->lock);

		return ret;
	} 

	/* change status to ready */
	pthread_mutex_lock(&ring->shm->lock);

	ent->status = LNF_RING_ENT_READ;

	printf("XXX write %d\n", ring->shm->write_pos);
	ring->shm->last_write_sequence++;
	ent->sequence = ring->shm->last_write_sequence;
	ring->shm->write_pos = lnf_ring_next(ring, ring->shm->write_pos);


	pthread_mutex_unlock(&ring->shm->lock);

	return LNF_OK;
}


/* release all resources allocated by ring buffer */
void lnf_ring_free(lnf_ring_t *ring) {

	int conn_count;

	if (ring == NULL) {
		return;
	}

	pthread_mutex_lock(&ring->shm->lock);
	ring->shm->conn_count--;

	conn_count = ring->shm->conn_count;

	pthread_mutex_unlock(&ring->shm->lock);

	if (ring->shm != NULL) {
		munmap(ring->shm, sizeof(lnf_ring_shm_t) + sizeof(lnf_ring_entry_t) * LNF_RINGBUF_SIZE);
	}

	if (conn_count <= 0) { 
		shm_unlink(ring->shm_name);
		printf("XXX shm unlink\n");
	}
	free(ring);
}

