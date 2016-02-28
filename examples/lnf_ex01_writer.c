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
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define FILENAME "./test-file.tmp"


int main (int argc, char **argv) {

	int i;	
	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_brec1_t	brec;
	uint32_t input, output;

	int nrecs = 1;
	int compress = LNF_COMP_LZO;
	int aggip = 1;
	int append = 0;
	char *filename = FILENAME;
	char c;
	char errbuf[1024];

	while ((c = getopt (argc, argv, "acbzn:f:r:?")) != -1) {
		switch (c) {
			case 'n':
				nrecs = atoi(optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				compress = 0; 
				break;
			case 'b':
				compress = LNF_COMP_BZ2; 
				break;
			case 'z':
				compress = LNF_COMP_LZO; 
				break;
			case 'a':
				append = 1; 
				break;
			case 'r':
				aggip = atoi(optarg); 
				break;
			case '?': 
				printf("Usage: %s [ -ac ] [ -r <step > ] [ -n <number of records to write> ] [ -f <output file name> ]\n", argv[0]);	
				printf(" -r  <n> : rotate src IP addess (for aggregation testing)\n\n");
				printf(" -a      : open in append mode\n");
				printf(" -z      : compress with LZO (default)\n");
				printf(" -b      : compress with BZ2\n");
				printf(" -c      : do not compressed file\n");
				exit(1);
		}
	}

	/* open lnf file desriptor */
	if (lnf_open(&filep, filename, LNF_WRITE | compress  | (append ? LNF_APPEND : 0), "myfile") != LNF_OK) { 
		lnf_error(errbuf, sizeof(errbuf));
		fprintf(stderr, "Can not open file (%s).\n", errbuf);
		exit(1);
	}

	srand(time(NULL));

	/* initialise empty record */
	lnf_rec_init(&recp);

	/* prepare data in asic record1 (lnf_brec1_t) */
	brec.first = 11220;
	brec.last = 11229;
	inet_pton(AF_INET6, "2001:67c:1220::aa:bb", &brec.srcaddr);
	inet_pton(AF_INET6, "2001:67c:1220::11:22", &brec.dstaddr);
	brec.prot = 6; /* TCP */
	brec.srcport = 1123;
	brec.dstport = 80;	
	brec.bytes = 12345;	
	brec.pkts = 20;	
	brec.flows = 1;	


	/* write records to file */
	for (i = 0; i < nrecs; i++) {

		input = i % 5; /* make input index interface 0 - 5 */
		output = i % 10; /* make output index interface 0 - 5 */

		if (aggip) {
			brec.bytes = i;
			brec.srcaddr.data[1] = 1000 + (i % aggip);
		}
		/* prepare record */
		lnf_rec_fset(recp, LNF_FLD_BREC1, &brec);
	
		/* set input and output interface */
		lnf_rec_fset(recp, LNF_FLD_INPUT, &input);
		lnf_rec_fset(recp, LNF_FLD_OUTPUT, &output);

		/* write record to file */
		if (lnf_write(filep, recp) != LNF_OK) {
			fprintf(stderr, "Can not write record no %d\n", i);
		}
	}

	/* return memory */
	lnf_rec_free(recp);
	lnf_close(filep);

	printf("%d records was written to %s\n", i, filename);
	printf("You can read it via cmd 'nfdump -r %s -o raw'\n", filename);

	return 0;
}


