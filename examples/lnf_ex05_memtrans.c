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

/* Demonstration of lnf_mem_read_raw, lnf_mem_write_raw */ 
/*  */

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define FILENAME "./test-file.tmp"
#define FILTER1 "src port > 80"
#define FILTER2 "in if 2"

#define LLUI long long unsigned int

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_mem_t *memp1;
	lnf_mem_t *memp2;
	lnf_mem_cursor_t *cursor;

	lnf_brec1_t brec;

	int i = 0;

    int print = 1;
    int printa = 1;
    char *filename = FILENAME;
    int c;

	char buff[LNF_MAX_RAW_LEN];
	int datasize;

	while ((c = getopt (argc, argv, "pPAf:")) != -1) {
		switch (c) {
			case 'p':
				print = 0;
				break;
			case 'P':
				print = 0;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'A':
				printa = 0;
				break;
			case '?':
				printf("Usage: %s [ -P ] [ -A ] [ -f <input file name> ] \n", argv[0]);
				printf(" -P : do not print input records to stdout\n");
				printf(" -A : do not aggregated records to stdout\n");
				exit(1);
		}
	}

	
	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename);
		exit(1);
	}


	lnf_rec_init(&recp);
	lnf_mem_init(&memp1);
	lnf_mem_init(&memp2);

	lnf_mem_fastaggr(memp1, LNF_FAST_AGGR_BASIC);
	lnf_mem_fastaggr(memp2, LNF_FAST_AGGR_BASIC);

	/* set rules for aggregation srcip/24,srcport,dstas */
	lnf_mem_fadd(memp1, LNF_FLD_SRCADDR, LNF_AGGR_KEY|LNF_SORT_DESC, 24, 64);
	lnf_mem_fadd(memp2, LNF_FLD_SRCADDR, LNF_AGGR_KEY|LNF_SORT_DESC, 24, 64);

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;

		/* add to memory heap */
		lnf_mem_write(memp1, recp);

		if (print) {
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
	}

	printf("Total input records: %d\n", i);

	/* transfer data from memp1 to memp2 */
	lnf_mem_first_c(memp1, &cursor);
	while (cursor != NULL) {
		lnf_mem_read_raw_c(memp1, cursor, buff, &datasize, LNF_MAX_RAW_LEN);
		lnf_mem_write_raw(memp2, buff, datasize);
		lnf_mem_next_c(memp1, &cursor);
	}


	/* all data are now in memp2) */


	i = 0;
	lnf_mem_first_c(memp2, &cursor);
	while (cursor != NULL) {

		i++;
		lnf_mem_read_c(memp2, cursor, recp);

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
		lnf_mem_next_c(memp2, &cursor);
	}

	printf("Total aggregated records: %d\n", i);

	lnf_mem_free(memp1);
	lnf_mem_free(memp2);
	lnf_rec_free(recp);
	lnf_close(filep);

	return 0;
}


