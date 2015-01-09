

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
#include "progress.h"

#define MAX_THREADS 50
#define LLUI long long unsigned int
#define MAX_FILTER_LEN 1024

/* global variable */
lnf_mem_t *memp;
int totalrows = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
flist_t *flist;
int fileidx = 0;
progress_t *progressp;
char filter[1024];




/* process one file */
int process_file(char *filename, lnf_filter_t *filterp);
int process_file(char *filename, lnf_filter_t *filterp) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
//	lnf_brec1_t brec;
	int i = 0;
	int tid;
	int match;

	tid = (int)pthread_self();


	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "[#%x] Can not open file %s\n", tid, filename);
		return 0;
	}

	lnf_rec_init(&recp);

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;

		match = 1;

		if ( filterp != NULL ) {
			match = lnf_filter_match(filterp, recp);
		}

		/* add to memory heap */
		if ( match ) {
			if (memp != NULL) {
				lnf_mem_write(memp, recp);
			} else {
				print_row(recp);
			}
		}

	}

	lnf_close(filep);

//	printf("[#%x] Total input records in file %s : %d\n", tid, filename, i);

	return i; 
}


/* thread loop */
void *process_thread(void *p);
void *process_thread(void *p) {

	int rows;
	int tid;
	char filename[PATH_MAX];
	lnf_filter_t *filterp = NULL;

	tid = (int)pthread_self();


	if (filter[0] != '\0') {
		lnf_filter_init(&filterp, filter);

		if (filterp == NULL) {
			fprintf(stderr, "[#%x] Can not initialise filter %s\n", tid, filter);
			return 0;
		}
	}


	for (;;) {

		/* get next file */
		pthread_mutex_lock(&mutex);

		if (!flist_pop(&flist, filename)) {
			pthread_mutex_unlock(&mutex);
			goto DONE;
		}

		progress_inc(progressp, 1);

		pthread_mutex_unlock(&mutex);

		/* process file */
		rows =  process_file(filename, filterp);

		pthread_mutex_lock(&mutex);
		totalrows += rows;
		pthread_mutex_unlock(&mutex);
	}

DONE:

	if ( memp != NULL ) {
		lnf_mem_merge_threads(memp);
	}

	lnf_filter_free(filterp);
	return NULL;
}


int main(int argc, char **argv) {

	lnf_rec_t *recp;
	pthread_t th[MAX_THREADS];
	int i = 0;
	int numthreads = 1;
    char c;
	lnf_filter_t *filterp;
	

	flist_init(&flist);

	numthreads = get_cpu_cores();

	/* initalise one instance of memory heap (share by all threads) */
	memp = NULL;
	progressp = NULL;
	recp = NULL;
	filter[0] = '\0';

	/* fields in all outpusts */
	fields_add(LNF_FLD_FIRST);
	fields_add(LNF_FLD_CALC_DURATION);


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

	/* set filter */
	if (optind < argc ) {
		for ( ; optind < argc; optind++) {
			if (strlen(filter) + strlen(argv[optind]) > MAX_FILTER_LEN) {
				fprintf(stderr, "Not enough space for filter in buffer\n");
				exit(1);
			} 
			strcat(filter, argv[optind]);
			strcat(filter, " ");
			printf("filter: %s \n", filter);
		}

		lnf_filter_init(&filterp, filter);
		if (filterp == NULL) {
			fprintf(stderr, "Can not compile filter: %s\n", filter);
			exit(1);
		}
		lnf_filter_free(filterp);
	}


	/* init progress bar */
	if  (progress_init(&progressp, 0, NULL) != NULL) {
		progress_steps(progressp,  flist_count(&flist));
	}

	/* aggregated or not aggregated records */
	if (memp == NULL) {
    	fields_add(LNF_FLD_PROT);
    	fields_add(LNF_FLD_SRCADDR);
    	fields_add(LNF_FLD_SRCPORT);
    	fields_add(LNF_FLD_DSTADDR);
    	fields_add(LNF_FLD_DSTPORT);
	}

	/* default fields on the ond of the list */
    fields_add(LNF_FLD_DPKTS);
    fields_add(LNF_FLD_DOCTETS);
    fields_add(LNF_FLD_CALC_BPS);
    fields_add(LNF_FLD_CALC_BPP);
    fields_add(LNF_FLD_AGGR_FLOWS);
	print_header();

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
	if (memp != NULL) {
		i = 0;
		lnf_rec_init(&recp);
		while (lnf_mem_read(memp, recp) != LNF_EOF) {
			i++;
			print_row(recp);
		}
	}

	/* header */
	printf("Total flows %d, aggregated flows: %d\n", totalrows, i);

	lnf_mem_free(memp);
	lnf_rec_free(recp);


}


