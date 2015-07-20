
/* Simple reader of nfdump files.  To demostrate functionality */ 
/* records are matched against two filters */

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define FILENAME "./test-file.tmp"
#define FILTER "src port 80"

#define LLUI long long unsigned int

int main(int argc, char **argv) {

	lnf_file_t *filep;
	lnf_rec_t *recp, *recp2;
	lnf_mem_t *memp;
	lnf_mem_cursor_t *cursor;

	lnf_brec1_t brec;
	uint16_t port;

	int i = 0;

    int print = 1;
    int printa = 1;
    char *filename = FILENAME;
    char c;

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
	lnf_mem_init(&memp);

	/* set rules for aggregation srcip/24,srcport,dstas */
	lnf_mem_fadd(memp, LNF_FLD_SRCPORT, LNF_AGGR_KEY, 0, 0); 

	lnf_mem_fadd(memp, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_DOCTETS, LNF_AGGR_SUM|LNF_SORT_DESC, 0, 0);
	lnf_mem_fadd(memp, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);



	while (lnf_read(filep, recp) != LNF_EOF) {
		i++;
		/* add to memory heap */
		lnf_mem_write(memp,recp);
	}

	printf("Total input records: %d\n", i);


	i = 0;
	while (lnf_mem_read(memp, recp) != LNF_EOF) {

		i++;

		if (printa) {
			lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);
			printf(" %d - %llu %llu %llu\n", 
					brec.srcport, 
					(LLUI)brec.first, (LLUI)brec.bytes, (LLUI)brec.pkts);
		}
	}

	printf("Total aggregated records: %d\n", i);
	printf("Lookup for src port 1123\n");

	lnf_rec_init(&recp2);

	/* set key field in record */
	port = 1124;
	lnf_rec_fset(recp2, LNF_FLD_SRCPORT, &port);
	
	if (lnf_mem_lookup_c(memp, recp2, &cursor) == LNF_OK) {
		printf("xxxx1\n");
		lnf_mem_read_c(memp, cursor, recp);

		lnf_rec_fget(recp, LNF_FLD_BREC1, &brec);	
		printf(" %d - %llu %llu %llu\n", 
			brec.srcport, 
			(LLUI)brec.first, (LLUI)brec.bytes, (LLUI)brec.pkts);
	} else {
		printf("Record not found\n");
	}
	

	lnf_mem_free(memp);
	lnf_rec_free(recp);
	lnf_close(filep);

	return 0;
}


