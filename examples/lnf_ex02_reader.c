
/* Simple reader of nfdump files.  To demostrate functionality */ 
/* records are matched against two filters */

#include <libnf.h>
#include <arpa/inet.h>
#include <unistd.h>

#define FILENAME "./test-file.tmp"
#define FILTER1 "src port > 80"
#define FILTER2 "in if 2"

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_filter_t *filterp1, *filterp2;
	lnf_brec1_t brec;
	char *filter1 = FILTER1;
	char *filter2 = FILTER2;
	uint32_t input, output;

	int i = 0;
	int match1 = 0;
	int match2 = 0;
	int if1 = 0;
	int if2 = 0;

    int print = 1;
    int filter = 1;
    char *filename = FILENAME;
    char c;

	while ((c = getopt (argc, argv, "pFf:1:2:")) != -1) {
		switch (c) {
			case 'p':
				print = 0;
				break;
			case 'F':
				filter = 0;
				break;
			case 'f':
				filename = optarg;
				break;
			case '1':
				filter1 = optarg;
				break;
			case '2':
				filter2 = optarg;
				break;
			case '?':
				printf("Usage: %s [ -p ] [ -f <output file name> ] [ -1 <filter1> ] [ -2 <filter2> ]\n", argv[0]);
				printf(" -p : do not print records to stdout\n");
				printf(" -F : do not use filters\n");
				exit(1);
		}
	}

	
	if (lnf_open(&filep, filename, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename);
		exit(1);
	}


	if (lnf_filter_init(&filterp1, filter1) != LNF_OK) {
		fprintf(stderr, "Can not init filter1 '%s'\n", filter1);
		exit(1);
	}

	if (lnf_filter_init(&filterp2, filter2) != LNF_OK) {
		fprintf(stderr, "Can not init filter2 '%s'\n", filter2);
		exit(1);
	}

	lnf_rec_init(&recp);

	while (lnf_read(filep, recp) != LNF_EOF) {

		lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
		lnf_rec_fget(recp, LNF_FLD_INPUT, &input);
		lnf_rec_fget(recp, LNF_FLD_OUTPUT, &output);
		i++;

		match1 = 0;
		match2 = 0;
		if (filter) {
			if (lnf_filter_match(filterp1, recp)) {
				if1++;
				match1 = 1;
			}
			if (lnf_filter_match(filterp2, recp)) {
				if2++;
				match2 = 1;
			}
		}

		if (print) {
			char sbuf[INET6_ADDRSTRLEN];
			char dbuf[INET6_ADDRSTRLEN];
	
			inet_ntop(AF_INET6, &brec.srcaddr, sbuf, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &brec.dstaddr, dbuf, INET6_ADDRSTRLEN);

			printf(" %s :%d -> %s :%d %d -> %d %llu %llu %llu [%d %d]\n", 
					sbuf, brec.srcport, 
					dbuf, brec.dstport,  
					input, output, 
					brec.pkts, brec.bytes, brec.flows, 
					match1, match2);
		}
	}

	printf("Total records: %d\n", i);
	printf("%d records matched by filter1 '%s'\n", if1, filter1);
	printf("%d records matched by filter2 '%s'\n", if2, filter2);

	lnf_rec_free(recp);
	lnf_filter_free(filterp1);
	lnf_filter_free(filterp2);
	lnf_close(filep);
}


