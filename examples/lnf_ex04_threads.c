

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAX_THREADS 50
#define FILENAME "./test-file.tmp"

/* global variable */
lnf_mem_t *memp;
int print = 1;

/* thread loop */
void *process_file(void *p);
void *process_file(void *p) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_brec1_t brec;
	int i = 0;
	int tid;

	tid = (int)pthread_self();

	char *filename = p;

	printf("[#%x] Processing %s\n", tid, filename);

	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "[#%x] Can not open file %s\n", tid, filename);
		return NULL;
	}

	lnf_rec_init(&recp);

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;

		/* add to memory heap */
		lnf_mem_write(memp, recp);

		if (print) {
			char sbuf[INET6_ADDRSTRLEN];
			char dbuf[INET6_ADDRSTRLEN];

			lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
	
			inet_ntop(AF_INET6, &brec.srcaddr, sbuf, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &brec.dstaddr, dbuf, INET6_ADDRSTRLEN);

			printf(" [#%x] %s :%d -> %s :%d %llu %llu %llu\n",  tid,
					sbuf, brec.srcport, 
					dbuf, brec.dstport,  
					brec.first, brec.bytes, brec.pkts);
		}
	}

	lnf_close(filep);

	printf("[#%x] Total input records: %d\n", tid, i);

	return NULL;
}



int main(int argc, char **argv) {

	lnf_rec_t *recp;
	lnf_brec1_t brec;

	pthread_t th[MAX_THREADS];

	int i = 0;

    int printa = 1;
	int numthreads = 0;
    char c;

	while ((c = getopt (argc, argv, "pPA:")) != -1) {
		switch (c) {
			case 'p':
			case 'P':
				print = 0;
				break;
			case 'A':
				printa = 0;
				break;
			case '?':
				printf("Usage: %s [ -P ] [ -A ] [ <file1> <file2> ... ] \n", argv[0]);
				printf(" -P : do not print input records to stdout\n");
				printf(" -A : do not aggregated records to stdout\n");
				exit(1);
		}
	}


	/* initalise one instance of memory heap (share by all threads) */
	lnf_mem_init(&memp);

	/* set rules for aggregation srcip/24,srcport,dstas */
	lnf_mem_fadd(memp, LNF_FLD_SRCADDR, LNF_AGGR_KEY|LNF_SORT_DESC, 24, 64);

	lnf_mem_fadd(memp, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_LAST, LNF_AGGR_MAX, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_DOCTETS, LNF_AGGR_SUM, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);

	/* read data from files in the separate threads */

	for ( i = optind; i < argc && i < MAX_THREADS; i++ ) {

		numthreads = i - optind;
		if ( pthread_create(&th[numthreads], NULL, process_file, argv[i]) < 0) {
			fprintf(stderr, "Can not create thread for %s\n", argv[i]);
			break;
		}

	}


	/* wait for threads */
	for ( i = 0; i <= numthreads; i++ ) {
		if( pthread_join(th[i], NULL) ) {
			fprintf(stderr, "Error joining thread\n");
			break;
		}
	}

	printf("threads ended \n");

	/* print the records out */
	i = 0;
	lnf_rec_init(&recp);
	while (lnf_mem_read(memp, recp) != LNF_EOF) {

		i++;

		if (printa) {
			char sbuf[INET6_ADDRSTRLEN];
			char dbuf[INET6_ADDRSTRLEN];

			lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
	
			inet_ntop(AF_INET6, &brec.srcaddr, sbuf, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &brec.dstaddr, dbuf, INET6_ADDRSTRLEN);

			printf(" %s :%d -> %s :%d %llu %llu %llu\n", 
					sbuf, brec.srcport, 
					dbuf, brec.dstport,  
					brec.first, brec.bytes, brec.pkts);
		}
	}

	printf("Total aggregated records: %d\n", i);

	lnf_mem_free(memp);
	lnf_rec_free(recp);


}


