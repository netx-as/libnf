

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include "cpucores.h"
#include "flist.h"
#include "screen.h"
#include "progress.h"

#define MAX_THREADS 50				/* maximum number of threads */
#define NUM_THREADS_FACTOR 0.7		/* defalt number of threads = real number of thread * THREADS_FACTOR */
#define LLUI long long unsigned int
#define MAX_FILTER_LEN 1024

/* global variable */
lnf_mem_t *memp;
int totalrows = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
flist_t *flist;
int fileidx = 0;
unsigned long outputflows = 0;
progress_t *progressp;
lnf_filter_t *filterp;
char filter[1024];

#define NFDUMPP_FILTER_DEFAULT 0
#define NFDUMPP_FILTER_NFDUMP 1
#define NFDUMPP_FILTER_LIBNF 2
int filter_type =  NFDUMPP_FILTER_DEFAULT;			/* filter to use */


struct option longopts[] = {
	{ "num-threads",		required_argument,	NULL,	1},
	{ "filter-type",		required_argument,	NULL,	2 },
	{ 0, 0, 0, 0 }
};


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
				outputflows++;
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
//	lnf_filter_t *filterp = NULL;

	tid = (int)pthread_self();

/*
	if (filter[0] != '\0') {
		switch (filter_type) {
			case NFDUMPP_FILTER_DEFAULT: 
					lnf_filter_init(&filterp, filter); 
					break;
			case NFDUMPP_FILTER_NFDUMP: 
					lnf_filter_init_v1(&filterp, filter); 
					break;
			case NFDUMPP_FILTER_LIBNF: 
					lnf_filter_init_v2(&filterp, filter); 
					break;
			default:
					fprintf(stderr, "This should never hapen line: %d\n", __LINE__);
					exit(1);
					break;
		}
					
		lnf_filter_init(&filterp, filter);

		if (filterp == NULL) {
			fprintf(stderr, "[#%x] Can not initialise filter %s\n", tid, filter);
			return 0;
		}
	}
*/


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

//	lnf_filter_free(filterp);
	return NULL;
}


int main(int argc, char **argv) {

	lnf_rec_t *recp;
	pthread_t th[MAX_THREADS];
	int i = 0;
	int numthreads = 1;
	int sortfield = 0;
	int sortbits4 = 0;
	int sortbits6 = 0;
    char c;
//	lnf_filter_t *filterp;
	

	flist_init(&flist);

	numthreads = get_cpu_cores() * NUM_THREADS_FACTOR;

	/* initalise one instance of memory heap (share by all threads) */
	memp = NULL;
	filterp = NULL;
	progressp = NULL;
	recp = NULL;
	filter[0] = '\0';

	/* fields in all outpusts */
	fields_add(LNF_FLD_FIRST);
	fields_add(LNF_FLD_CALC_DURATION);


	while ((c = getopt_long(argc, argv, "A:O:r:R:T:W;", longopts, NULL)) != -1) {
		switch (c) {
			case 1: 
			case 'T': 	/* T option will be removed in future */
				numthreads = atoi(optarg);
				if (numthreads > MAX_THREADS) {
					fprintf(stderr, "Maximim allowed threads is %d\n", MAX_THREADS);
					exit(1);
				//	numthreads = MAX_THREADS - 1;
				} 
				break;
			case 2: 
				if (strcmp(optarg, "nfdump") == 0) {
					filter_type = NFDUMPP_FILTER_NFDUMP;			
				} else if (strcmp(optarg, "libnf") == 0) {
					filter_type = NFDUMPP_FILTER_LIBNF;			
				} else {
					fprintf(stderr, "Invalid filter type \"%s\". Allowed options nfdump or libnf. \n", optarg);
					exit(1);
				}
				break;
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
			case 'O':
				sortfield = lnf_fld_parse(optarg, &sortbits4, &sortbits6);
				if (sortfield == 0) {
					fprintf(stderr, "Unknow or unsupported sort field: %s\n", optarg);
					exit(1);
				}
				break;
			case '?':
				printf("Usage: %s [ -A ] [ -R -r ] [ <filter> ] \n", argv[0]);
				printf(" -r : \n");
				printf(" -R : Input file or directory  (multiple -r -R options is allowed)\n");
				printf(" -A : aggregation\n");
				printf(" -O : sort order\n");
				printf(" --num-threads = <num> : num threads (default: %.0f%% number of CPU cores, %d on this system)\n", 
							NUM_THREADS_FACTOR * 100, numthreads);
				printf(" --filter-type = nfdump|libnf : use original nfdump filter or new libnf implementation \n");
				printf("\n");
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

		filterp = NULL;
		switch (filter_type) {
			case NFDUMPP_FILTER_DEFAULT: 
					lnf_filter_init(&filterp, filter); 
					break;
			case NFDUMPP_FILTER_NFDUMP: 
					lnf_filter_init_v1(&filterp, filter); 
					break;
			case NFDUMPP_FILTER_LIBNF: 
					lnf_filter_init_v2(&filterp, filter); 
					break;
			default:
					fprintf(stderr, "This should never hapen line: %d\n", __LINE__);
					exit(1);
					break;
		}
					
		if (filterp == NULL) {
			fprintf(stderr, "Can not compile filter: %s\n", filter);
			exit(1);
		}
//		lnf_filter_free(filterp);
	}


	/* init progress bar */
	if  (progress_init(&progressp, 0, NULL) != NULL) {
		progress_steps(progressp,  flist_count(&flist));
	}

	/* aggregated or not aggregated records */
	if (memp == NULL) {
		/* not aggregated, but sorted */
		if (sortfield > 0) {
			lnf_mem_init(&memp);
			/* switch memp into list mode */
			lnf_mem_setopt(memp, LNF_OPT_LISTMODE, NULL, 0);
			lnf_mem_fastaggr(memp, LNF_FAST_AGGR_BASIC);
			lnf_mem_fadd(memp, LNF_FLD_PROT, LNF_AGGR_KEY, 0, 0);
			lnf_mem_fadd(memp, LNF_FLD_SRCADDR, LNF_AGGR_KEY, 24, 128);
			lnf_mem_fadd(memp, LNF_FLD_SRCPORT, LNF_AGGR_KEY, 0, 0);
			lnf_mem_fadd(memp, LNF_FLD_DSTADDR, LNF_AGGR_KEY, 24, 128);
			lnf_mem_fadd(memp, LNF_FLD_DSTPORT, LNF_AGGR_KEY, 0, 0);
		}
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

	/* set sort firld */
	if (sortfield > 0) {
		int defaultaggr = 0;
		int defaultsort = 0;
		lnf_fld_info(sortfield, LNF_FLD_INFO_AGGR, &defaultaggr, sizeof(int));
		lnf_fld_info(sortfield, LNF_FLD_INFO_SORT, &defaultsort, sizeof(int));
		lnf_mem_fadd(memp, sortfield, defaultaggr|defaultsort, sortbits4, sortbits6);
	}


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
			outputflows++;
		}
	}

	/* header */
	printf("Total input flows %d, output flows: %lu\n", totalrows, outputflows);

	lnf_mem_free(memp);
	lnf_rec_free(recp);
	if (filterp != NULL) {
		lnf_filter_free(filterp);
	}


}


