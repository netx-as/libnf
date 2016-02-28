

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
#include "output.h"
#include "progress.h"

#define MAX_THREADS 50				/* maximum number of threads */
#define MAX_OUTPUTS 50				/* maximum number of outpus */
#define NUM_THREADS_FACTOR 0.7		/* defalt number of threads = real number of thread * THREADS_FACTOR */
#define LLUI long long unsigned int
#define MAX_FILTER_LEN 1024

/* global variable */
//lnf_mem_t *memp;
int totalrows = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
flist_t *flist;
int fileidx = 0;
unsigned long outputflows = 0;
progress_t *progressp;
lnf_filter_t *filterp;
output_t output[MAX_OUTPUTS];
int numoutputs;
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
			for (int o = 0; o < numoutputs; o++) {
				output_write(&output[o], recp);
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

	tid = (int)pthread_self();

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

	for (int o = 0; o < numoutputs; o++) {
		output_merge_threads(&output[o]);
	}

	return NULL;
}


int main(int argc, char **argv) {

//	lnf_rec_t *recp;
	pthread_t th[MAX_THREADS];
	int i = 0;
	int numthreads = 1;
	int sortfield = 0;
	int sortbits4 = 0;
	int sortbits6 = 0;
	int c;
	int numaflags = 0;
	output_t *outputp;
//	lnf_filter_t *filterp;
	

	flist_init(&flist);

	numthreads = get_cpu_cores() * NUM_THREADS_FACTOR;

	/* initalise one instance of memory heap (share by all threads) */
//	memp = NULL;
	filterp = NULL;
	progressp = NULL;
//	recp = NULL;
	filter[0] = '\0';

	/* fields in all outpusts  - initalised in all outputs */
	for (int i = 0; i < MAX_OUTPUTS; i++) {
		output_init(&output[i]);
		output_set_fmt(&output[i], OFMT_LINE, NULL);			
		output_field_add(&output[i], LNF_FLD_FIRST);
		output_field_add(&output[i], LNF_FLD_CALC_DURATION);
	}

	numoutputs = 1;
	outputp = &output[0];

	while ((c = getopt_long(argc, argv, "w:o:A:O:r:R:T:W;", longopts, NULL)) != -1) {
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
			case 'w': 
				output_set_fmt(outputp, OFMT_BIN_NFDUMP, optarg);
				break;
			case 'o': 
				if (strcmp(optarg, "raw") == 0) {
					output_set_fmt(outputp, OFMT_RAW, NULL);			
				} else if (strcmp(optarg, "line") == 0) {
					output_set_fmt(outputp, OFMT_LINE, NULL);			
				} else {
					fprintf(stderr, "Unknown output format \"%s\".\n", optarg);
					exit(1);
				}
				break;
			case 'A':
				/* if it is the second A flag we initialise second screen */
				if (numaflags >= 1) {
					if (numoutputs >= MAX_OUTPUTS - 1) {
						fprintf(stderr, "Reached max outputs \"%d\".\n", MAX_OUTPUTS);
					}
					outputp = &output[numoutputs];
					outputp->sortfield = output[numoutputs - 1].sortfield;
					numoutputs++;
				}

				if ( ! output_parse_aggreg(outputp, optarg) ) { 
					exit(1);
				}
				numaflags++;
				break;
			case 'O':
				sortfield = lnf_fld_parse(optarg, &sortbits4, &sortbits6);
				if (sortfield == 0) {
					fprintf(stderr, "Unknow or unsupported sort field: %s\n", optarg);
					exit(1);
				}
				output_set_sort(outputp, sortfield, sortbits4, sortbits6);
				break;
			case '?':
				printf("Usage: %s [ -A ] [ -R -r ] [ <filter> ] \n", argv[0]);
				printf(" -r : \n");
				printf(" -R : Input file or directory  (multiple -r -R options is allowed)\n");
				printf(" -A : aggregation\n");
				printf(" -O : sort order\n");
				printf(" -o : output format: line, raw, nfdump (default: line or nfdump with -w option)\n");
				printf(" -w : output file\n");
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

	output_start(&output[0]);

	/* init progress bar */
	if  (progress_init(&progressp, 0, NULL) != NULL) {
		progress_steps(progressp,  flist_count(&flist));
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

	for (int o = 0; o < numoutputs; o++) {
		if (o > 0 ) { 
			output_start(&output[o]);
		}

		/* print the records out */
		output_output_rows(&output[o]);


		/* finish output */
		output_finish(&output[o]);

		/* header */
		printf("Total input flows %d, output flows: %lu\n\n", totalrows, output[o].outputflows);

	}

	if (filterp != NULL) {
		lnf_filter_free(filterp);
	}

}


