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
#include <inttypes.h>

#define FILENAME_IN "./ringbuf-out.tmp"
#define SHM "libnf-shm"

#define LLUI long long unsigned int

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_ring_t *ringp;
	lnf_rec_t *recp;
	char buf[LNF_MAX_STRING];
	char errbuf[LNF_MAX_STRING];

    char *filename_in = FILENAME_IN;
    char *shm = SHM;
	int i = 0;
	int j = 0;
	int c;
	int ret;
	int print = 1;
	int statistics = 0;
	int ts1,ts2;
	uint64_t lost1 = 0;
	uint64_t lost2 = 0;
	

	while ((c = getopt (argc, argv, "Psf:S:?")) != -1) {
		switch (c) {
			case 'f':
				filename_in = optarg;
				break;
			case 'S':
				shm = optarg;
				break;
			case 'P':
				print = 0;
				break;
			case 's':
				statistics = 1;
				break;
			case '?':
				printf("Usage: %s [ -P ] [ -s ] [ -f <output file name> ] [ -S <shared memory name> ]\n", argv[0]);
				printf("   -P do not print information about received records\n");
				printf("   -s do statistics only\n");
				exit(1);
		}
	}

	
	if (!statistics && lnf_open(&filep, filename_in, LNF_WRITE, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename_in);
		exit(1);
	}

	lnf_rec_init(&recp);
	if (lnf_ring_init(&ringp, shm, 0) != LNF_OK) {
		fprintf(stderr, "Can not initialise ring buffer %s\n", shm);
		lnf_error(buf, LNF_MAX_STRING);
		fprintf(stderr, "%s\n", buf);
		exit(1);
	}

	printf("waiting for data...\n");
	ts1 = time(NULL);
	for (;;) {

		if ((ret = lnf_ring_read(ringp, recp)) == LNF_OK) {
			i++;
			j++;
			if (print) {
				printf("Received record #%d\n", i);
			}
			if (!statistics) {
				if (lnf_write(filep, recp) != LNF_OK) {
					printf("Can't write record to file\n");
				}
			} else {
				ts2 = time(NULL);
				if (ts2 > ts1) {
					lnf_ring_info(ringp, LNF_RING_LOST, &lost1, sizeof(lost1));	
					printf("Received %d recs in %d secs (lost: %" PRId64 " recs)\n", j, ts2 - ts1, lost1 - lost2);
					j = 0;
					lost2 = lost1;
				}

				ts1 = ts2;
			}
		} else {
			if (ret == LNF_ERR_OTHER_MSG) {
				lnf_error((char *)&errbuf, LNF_MAX_STRING);
				printf("ERROR: %s\n", errbuf);
			} else {
				printf("ERROR: other\n");
			}
		}
	}

	printf("Total records read from ringbuf: %d\n", i);

	lnf_rec_free(recp);
	lnf_ring_free(ringp);
	lnf_close(filep);

	return 0;
}


