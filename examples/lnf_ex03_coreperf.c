//
// Created by istoffa on 5/16/17.
//
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
#include <time.h>
#include <stdlib.h>


#define FILENAME "./test-file.tmp"
#define FILTER1 "src port > 80"

#define LLUI long long unsigned int

int main(int argc, char **argv)
{

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_filter_t *filterp1;
	lnf_brec1_t brec;
	char *filter1 = FILTER1;
	uint32_t input, output;
	char buf[LNF_MAX_STRING];
	int res;
	char use_v2 = 1;

	int i = 0;
	int match1 = 0;

	int print = 1;
	int filter = 1;
	int fget = 1;
	char *filename = FILENAME;
	char *fifname = NULL;
	char program[200];
	int c;

	FILE *filters = NULL;

	while ((c = getopt(argc, argv, "pnPF:Gf:1:")) != -1) {
		switch (c) {
		case 'p':
			print = 0;
			break;
		case 'n':
			use_v2 = 0;
			break;
		case 'P':
			print = 0;
			break;
		case 'F':
			fifname = optarg;
			break;
		case 'f':
			filename = optarg;
			break;
		case '1':
			filter1 = optarg;
			break;
		case '?':
			printf("Usage: %s [ -p ] [ -f <output file name> ] [ -1 <filter1> ]\n", argv[0]);
			printf(" -P : do not print records to stdout\n");
			printf(" -F : supply file with filters to test, each line is in form '%%d %%s' - group number and filter\n");
			printf(" -n : use stock nfdump core\n");
			exit(1);
		}
	}



	if ((filters = fopen(fifname, "r")) == NULL) {
		fprintf(stderr, "Can not open file with test filters %s\n", fifname);
		exit(1);
	}

	printf("Set, Nodes, duration sec, performance records/sec, matches\n");

	int set = 0;
	int nodes;

	while (!feof(filters)) {

		if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
			fprintf(stderr, "Can not open file %s\n", filename);
			exit(1);
		}

		set++;

		if (fscanf(filters, "%d", &nodes) < 1) {
			break;
		}
		fgets(&program[0], sizeof(program), filters);

		if ((!use_v2 || ((res = lnf_filter_init_v2(&filterp1, &program[0])) != LNF_OK)))
				if(use_v2 || (res = lnf_filter_init(&filterp1, &program[0])) != LNF_OK)
				{
					fprintf(stderr, "Can not init filter '%s'\n", &program[0]);
					if (res == LNF_ERR_OTHER_MSG) {
						lnf_error(buf, LNF_MAX_STRING);
						fprintf(stderr, "%s\n", buf);
					}
					continue;
				}

		lnf_rec_init(&recp);

		clock_t t;
		clock_t total = 0;

		int i = 0;
		long int count = 0;
		int match;

		while (lnf_read(filep, recp) != LNF_EOF) {

			if (fget) {
				lnf_rec_fget(recp, LNF_FLD_INPUT, &input);

				lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
				lnf_rec_fget(recp, LNF_FLD_INPUT, &input);
				lnf_rec_fget(recp, LNF_FLD_OUTPUT, &output);
			}
			count++;

			t = clock();
			match = ((lnf_filter_match(filterp1, recp)) == 1);
			total += clock() - t;

			i += match;
			if (print && match) {
				char sbuf[INET6_ADDRSTRLEN];
				char dbuf[INET6_ADDRSTRLEN];

				inet_ntop(AF_INET6, &brec.srcaddr, sbuf, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, &brec.dstaddr, dbuf, INET6_ADDRSTRLEN);

				printf("%s :%d -> %s :%d %d -> %d %llu %llu %llu\n",
				       sbuf, brec.srcport,
				       dbuf, brec.dstport,
				       input, output,
				       (LLUI) brec.pkts, (LLUI) brec.bytes, (LLUI) brec.flows);
			}
		}
		printf("%d,\t%d,\t%lf,\t%.0lf,\t%d\n",
		       set, nodes, ((double) total) / CLOCKS_PER_SEC,
		       count*((CLOCKS_PER_SEC)/((double)total)),
		       i);

		lnf_rec_free(recp);
		lnf_filter_free(filterp1);
		lnf_close(filep);
	}

	if(filters)
		fclose(filters);

	return 0;
}

