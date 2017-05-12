

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <flist.h>
#include <time.h>
#include <pthread.h>
#include "output.h"
#include "output_ringbuf.h"



int output_start_ringbuf(output_t *output) {

	int flags = 0;

	if (pthread_mutex_init(&output->write_lock, NULL) != 0) {
		return 0;
	}

	if (lnf_ring_init(&output->ringp, output->ringname, flags) != LNF_OK) {
		return 1;
	} else {
		return 0;
	}
}


int output_row_ringbuf(output_t *output, lnf_rec_t *rec) {

	int ret;

	ret = lnf_ring_write(output->ringp, rec);

	if (ret != LNF_OK) { 
		return 1;
	} else {
		return 0;
	}
}

int output_finish_ringbuf(output_t *output) {

	pthread_mutex_destroy(&output->write_lock);
	lnf_ring_free(output->ringp);
	return 1;
}


