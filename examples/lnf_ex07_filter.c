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

/* Simple reader of nfdump files.  To demostrate functionality */ 
/* records are matched against two filters */

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define FILENAME "./test-file.tmp"
#define FILTER "src net 2001:67c:1220::/64"

#define LLUI long long unsigned int

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_filter_t *filterp1, *filterp2;
	lnf_brec1_t brec;
	char *filter = FILTER;
	char buf[LNF_MAX_STRING];
	int res;
	int match1, match2;

	int i = 0;
	int if1 = 0;
	int if2 = 0;

    char *filename = FILENAME;
    int c;

	while ((c = getopt (argc, argv, "f:1:")) != -1) {
		switch (c) {
			case 'f':
				filename = optarg;
				break;
			case '1':
				filter = optarg;
				break;
			case '?':
				printf("Usage: %s [ -f <input file name> ] [ -1 <filter> ] \n", argv[0]);
				printf(" -P : do not print records to stdout\n");
				printf(" -F : do not use filters\n");
				printf(" -G : do not use lng_rec_fget\n");
				exit(1);
		}
	}

	
	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename);
		exit(1);
	}


	/* initialise old (nfdump) and new (libnf) filter */
	if ((res = lnf_filter_init_v1(&filterp1, filter)) != LNF_OK) {
		fprintf(stderr, "Can not init old (nfdump) filter '%s'\n", filter);
		if (res == LNF_ERR_OTHER_MSG) {
			lnf_error(buf, LNF_MAX_STRING);
			fprintf(stderr, "RES: %s\n", buf);
			filterp1 = NULL;
		}
	}

	if ((res = lnf_filter_init_v2(&filterp2, filter)) != LNF_OK) {
		fprintf(stderr, "Can not init new (libnf) filter '%s'\n", filter);
		lnf_error(buf, LNF_MAX_STRING);
		if (res == LNF_ERR_OTHER_MSG) {
			lnf_error(buf, LNF_MAX_STRING);
			fprintf(stderr, "RES: %s\n", buf);
			filterp2 = NULL;
		}
	}

	lnf_rec_init(&recp);

	printf("Filter expr: %s\n", filter);

	while (lnf_read(filep, recp) != LNF_EOF) {
		i++;
		match1 = 0;
		match2 = 0;
		if (filter) {
			if (filterp1 != NULL && lnf_filter_match(filterp1, recp)) {
				if1++;
				match1 = 1;
			}
			if (filterp2 != NULL && lnf_filter_match(filterp2, recp)) {
				if2++;
				match2 = 1;
			}
		}

		if (match1 != match2) {
			char sbuf[INET6_ADDRSTRLEN];
			char dbuf[INET6_ADDRSTRLEN];

			lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
	
			inet_ntop(AF_INET6, &brec.srcaddr, sbuf, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &brec.dstaddr, dbuf, INET6_ADDRSTRLEN);

			printf(" %s :%d -> %s :%d %llu %llu %llu [%d %d]\n", 
					sbuf, brec.srcport, 
					dbuf, brec.dstport,  
					(LLUI)brec.pkts, (LLUI)brec.bytes, (LLUI)brec.flows, 
					match1, match2);
		}
	}

	printf("Total records: %d\n", i);
	printf("%d records matched by filter1 (old - nfdump)\n", if1);
	printf("%d records matched by filter2 (new - libnf)\n", if2);

	lnf_rec_free(recp);
	lnf_filter_free(filterp1);
	lnf_filter_free(filterp2);
	lnf_close(filep);

	return 0;
}


