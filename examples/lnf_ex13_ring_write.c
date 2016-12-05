/* 

 Copyright (c) 2013-2016, Tomas Podermanski
    
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

/* Simple reader of nfdump files.  To demostrate functionality */ 
/* records are matched against two filters */

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define FILENAME_IN "./test-file.tmp"
#define SHM "libnf-shm"

#define LLUI long long unsigned int

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_ring_t *ringp;
	lnf_rec_t *recp;
	char buf[LNF_MAX_STRING];

    char *filename_in = FILENAME_IN;
    char *shm = SHM;
	int i = 0;
	int j = 0;
	int flags = 0;
	int c;
	struct timespec ts;
	int recs_per_sec = 100000;
	

	while ((c = getopt (argc, argv, "f:S:?Fp:")) != -1) {
		switch (c) {
			case 'f':
				filename_in = optarg;
				break;
			case 'S':
				shm = optarg;
				break;
			case 'F':
				flags = LNF_RING_FORCE_RELEASE|LNF_RING_FORCE_INIT;
				break;
			case 'p':
				recs_per_sec = atoi(optarg);
				break;
			case '?':
				printf("Usage: %s [ -f <input file name> ] [ -S <shared memory file> ] [ -p <rec per second> ]\n", argv[0]);
				printf(" -F : use LNF_RING_FORCE_RELEASE|LNF_RING_FORCE_INIT \n");
				printf(" -p : rec per second (default: 100000) \n");
				exit(1);
		}
	}


	/* compute delays between sends */
	if (recs_per_sec > 0) {
		ts.tv_nsec = 1000000 / recs_per_sec;
	}
	
	if (lnf_open(&filep, filename_in, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename_in);
		exit(1);
	}

	lnf_rec_init(&recp);
	if (lnf_ring_init(&ringp, shm, flags) != LNF_OK) {
		fprintf(stderr, "Can not initialise ring buffer %s\n", shm);
		lnf_error(buf, LNF_MAX_STRING);
		fprintf(stderr, "%s\n", buf);
		exit(1);
	}

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;
		if (lnf_ring_write(ringp, recp) != LNF_OK) {
			printf("Can't write record to ring buffer\n");
		} else {
			j++;
		}

		if (recs_per_sec > 0) {
			nanosleep(&ts, NULL);
		}

	}

	printf("Total records: %d, records written into ringbuf: %d\n", i, j);

	lnf_rec_free(recp);
	lnf_ring_free(ringp);
	lnf_close(filep);

	return 0;
}


