

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include "cpucores.h"
#include "flist.h"
#include "screen.h"

#define MAX_THREADS 50
#define LLUI long long unsigned int

/* global variable */
lnf_mem_t *memp;
int totalrows = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
flist_t *flist;
int fileidx = 0;

/*
typedef struct fields_s {
	int field;
	int type;
	int numbits;
	int numbits6;
} fields_t;

fields_t fields[LNF_FLD_TERM_] = { };
int numfields = 0;
*/


/* process one file */
int process_file(char *filename);
int process_file(char *filename) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
//	lnf_brec1_t brec;
	int i = 0;
	int tid;

	tid = (int)pthread_self();

	printf("[#%x] Processing %s\n", tid, filename);

	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "[#%x] Can not open file %s\n", tid, filename);
		return 0;
	}

	lnf_rec_init(&recp);

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;

		/* add to memory heap */
		lnf_mem_write(memp, recp);

		/*	
		{
			char sbuf[INET6_ADDRSTRLEN];
			char dbuf[INET6_ADDRSTRLEN];

			lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
	
			inet_ntop(AF_INET6, &brec.srcaddr, sbuf, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &brec.dstaddr, dbuf, INET6_ADDRSTRLEN);

			printf(" [#%x] %s :%d -> %s :%d %llu %llu %llu\n",  tid,
					sbuf, brec.srcport, 
					dbuf, brec.dstport,  
					(LLUI)brec.first, (LLUI)brec.bytes, (LLUI)brec.pkts);
		}
		*/
	}

	lnf_close(filep);

	printf("[#%x] Total input records in file %s : %d\n", tid, filename, i);

	return i; 
}


/* thread loop */
void *process_thread(void *p);
void *process_thread(void *p) {

	int rows;
	int tid;
	char filename[PATH_MAX];

	tid = (int)pthread_self();

	for (;;) {

		/* get next file */
		pthread_mutex_lock(&mutex);

		if (!flist_pop(&flist, filename)) {
			pthread_mutex_unlock(&mutex);
			goto DONE;
		}

		pthread_mutex_unlock(&mutex);

		/* process file */
		rows =  process_file(filename);

		pthread_mutex_lock(&mutex);
		totalrows += rows;
		pthread_mutex_unlock(&mutex);
	}

DONE:

	lnf_mem_merge_threads(memp);

	return NULL;
}


int main(int argc, char **argv) {

	lnf_rec_t *recp;
	pthread_t th[MAX_THREADS];
	int i = 0;
	int numthreads = 1;
    char c;

	flist_init(&flist);

	numthreads = get_cpu_cores();

	/* initalise one instance of memory heap (share by all threads) */
	memp = NULL;

	while ((c = getopt (argc, argv, "A:r:R:T:")) != -1) {
		switch (c) {
			case 'r':
			case 'R':
				flist_lookup_dir(&flist, optarg);
				break;
			case 'A':
				if (memp == NULL) {
					lnf_mem_init(&memp);
				}
				parse_aggreg(memp, optarg);
				break;
			case 'T': 
				numthreads = atoi(optarg);
				if (numthreads > MAX_THREADS) {
					numthreads = MAX_THREADS - 1;
				}
				break;
			case '?':
				printf("Usage: %s [ -A ] [ -R -r ] [ <filter> ] \n", argv[0]);
				printf(" -r : \n");
				printf(" -R : Input file or directory  \n");
				printf(" -A : aggregation\n");
				printf(" -T : num threads (default: number of CPU cores, %d on this system)\n", numthreads);
				exit(1);
		}
	}

	/*  prepare and run threads */
	pthread_mutex_init(&mutex, NULL);

	for ( i = 0 ; i < numthreads ; i++ ) {
		if ( pthread_create(&th[i], NULL, process_thread, NULL) < 0) {
			fprintf(stderr, "Can not create thread for %d\n", i);
			break;
		}
	}

	/* wait for threads */
	for ( i = 0; i < numthreads; i++ ) {
		if( pthread_join(th[i], NULL) ) {
			fprintf(stderr, "Error joining thread\n");
			break;
		}
	}

	/* print the records out */
	i = 0;
	lnf_rec_init(&recp);
	print_header();
	while (lnf_mem_read(memp, recp) != LNF_EOF) {
		i++;
		print_row(recp);
	}

	printf("Total flows %d, aggregated flows: %d\n", totalrows, i);

	lnf_mem_free(memp);
	lnf_rec_free(recp);


}


