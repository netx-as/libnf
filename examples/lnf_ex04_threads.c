/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAX_THREADS 50
#define MAX_FILES 10000
#define LLUI long long unsigned int

/* global variable */
lnf_mem_t *memp;
int print = 1;
int aggregate = 1;
int totalrows = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char *filelist[MAX_FILES];
int fileidx = 0;



/* process one file */
int process_file(char *filename);
int process_file(char *filename) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_brec1_t brec;
	int i = 0;
	int tid;

	tid = (int)pthread_self();

	if (print) {
		printf("[#%x] Processing %s\n", tid, filename);
	}

	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "[#%x] Can not open file %s\n", tid, filename);
		return 0;
	}

	lnf_rec_init(&recp);

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;

		/* add to memory heap */
		if (aggregate) {
			lnf_mem_write(memp, recp);
		}

		if (print) {
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
	}

	lnf_close(filep);

	if (print) {
		printf("[#%x] Total input records in file %s : %d\n", tid, filename, i);
	}

	return i; 
}


/* thread loop */
void *process_thread(void *p);
void *process_thread(void *p) {

	int rows;
	char *filename;

	for (;;) {

		/* get next file */
		pthread_mutex_lock(&mutex);
		if (filelist[fileidx] == NULL) {
			pthread_mutex_unlock(&mutex);
			goto DONE;
		} else {
			filename = filelist[fileidx];
			fileidx++;
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
	lnf_brec1_t brec;
	lnf_mem_cursor_t *cursor;

	pthread_t th[MAX_THREADS];

	int i = 0;

    int printa = 1;
	int numthreads = 1;
	int listmode = 0;
	int c;

	while ((c = getopt (argc, argv, "lnpPAt:")) != -1) {
		switch (c) {
			case 'p':
			case 'P':
				print = 0;
				break;
			case 'A':
				printa = 0;
				break;
			case 'n':
				aggregate = 0;
				break;
			case 'l':
				listmode = 1;
				break;
			case 't': 
				numthreads = atoi(optarg);
				if (numthreads > MAX_THREADS) {
					numthreads = MAX_THREADS - 1;
				}
				break;
			case '?':
				printf("Usage: %s [ -P ] [ -A ] [ <file1> <file2> ... ] \n", argv[0]);
				printf(" -P : do not print input records to stdout\n");
				printf(" -A : do not print aggregated records to stdout\n");
				printf(" -n : do not aggregate records (just read and throw out)\n");
				printf(" -l : switch to list mode (dot not aggregate)\n");
				printf(" -t : num threads\n");
				exit(1);
		}
	}

	/* initalise one instance of memory heap (share by all threads) */
	lnf_mem_init(&memp);

	if (listmode) {
		if (lnf_mem_setopt(memp, LNF_OPT_LISTMODE, NULL, 0) != LNF_OK) {
			printf("Can't switch to list mode\n");
			exit(1);
		}
	}

	/* set rules for aggregation srcip/24,srcport,dstas */
	lnf_mem_fadd(memp, LNF_FLD_SRCADDR, LNF_AGGR_KEY|LNF_SORT_DESC, 24, 64);

	lnf_mem_fadd(memp, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_LAST, LNF_AGGR_MAX, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_DOCTETS, LNF_AGGR_SUM, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);


	/* prepare file list */
	for (i = optind; i < argc; i++) {
		filelist[i -  optind] = argv[i];
	}
	filelist[i -  optind] = NULL;


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

	printf("Threads ended, total input records %d \n", totalrows);

	/* print the records out */
	i = 0;
	lnf_rec_init(&recp);
	lnf_mem_first_c(memp, &cursor);
	while (cursor != NULL) {

		lnf_mem_read_c(memp, cursor, recp);

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
					(LLUI)brec.first, (LLUI)brec.bytes, (LLUI)brec.pkts);
		}
		lnf_mem_next_c(memp, &cursor);
	} 

	printf("Total aggregated records: %d\n", i);

	lnf_mem_free(memp);
	lnf_rec_free(recp);


	return 0;
}


