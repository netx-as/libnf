
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

	int i = 0;

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
	lnf_mem_addf(memp, LNF_FLD_SRCADDR, LNF_AGGR_KEY, 24, 64);
	lnf_mem_addf(memp, LNF_FLD_SRCPORT, LNF_AGGR_KEY, 0, 0); 
	lnf_mem_addf(memp, LNF_FLD_DSTAS, LNF_AGGR_KEY, 0, 0);

	lnf_mem_addf(memp, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
	lnf_mem_addf(memp, LNF_FLD_DOCTETS, LNF_AGGR_SUM|LNF_SORT_DESC, 0, 0);
	lnf_mem_addf(memp, LNF_FLD_LAST, LNF_AGGR_MAX, 0, 0);
	lnf_mem_addf(memp, LNF_FLD_TCP_FLAGS, LNF_AGGR_OR, 0, 0);
	lnf_mem_addf(memp, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);

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

			printf(" %s :%d -> %s :%d %llu %llu %llu\n", 
					sbuf, brec.srcport, 
					dbuf, brec.dstport,  
					brec.pkts, brec.bytes, brec.flows);
		}
	}

	printf("Total input records: %d\n", i);

	lnf_mem_free(memp);
	lnf_rec_free(recp);
	lnf_close(filep);
}


