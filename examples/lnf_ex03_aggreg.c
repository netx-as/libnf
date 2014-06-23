
/* Simple reader of nfdump files.  To demostrate functionality */ 
/* records are matched against two filters */

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define FILENAME "./test-file.tmp"
#define FILTER1 "src port > 80"
#define FILTER2 "in if 2"

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_mem_t *memp;

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
    int fget = 1;
    char *filename = FILENAME;
    char c;

	while ((c = getopt (argc, argv, "pPFGf:1:2:")) != -1) {
		switch (c) {
			case 'p':
				print = 0;
				break;
			case 'P':
				print = 0;
				break;
			case 'G':
				fget = 0;
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


	lnf_rec_init(&recp);
	lnf_mem_init(&memp);

	/* set rules for aggregation srcip/24,srcport,dstas */
	lnf_mem_add_aggr(memp, LNF_FLD_SRCADDR, 24, 64);
	lnf_mem_add_aggr(memp, LNF_FLD_SRCPORT, 0, 0);
	lnf_mem_add_aggr(memp, LNF_FLD_DSTAS, 0, 0);

	while (lnf_read(filep, recp) != LNF_EOF) {

		i++;

		/* add to memory heap */
		lnf_mem_write(memp,recp);
		printf("********************\n");

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

	printf("Total input records: %d\n", i);

	lnf_mem_free(memp);
	lnf_rec_free(recp);
	lnf_close(filep);
}


