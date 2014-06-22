
#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define FILENAME "./test-file.tmp"


int main (int argc, char **argv) {

	int i;	
	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_brec1_t	brec;
	uint32_t input, output;

	int nrecs = 1;
	int compress = 1;
	char *filename = FILENAME;
	char c;

	while ((c = getopt (argc, argv, "n:f:?")) != -1) {
		switch (c) {
			case 'n':
				nrecs = atoi(optarg);
			break;
			case 'f':
				filename = optarg;
			case 'C':
				compress = 0; 
			break;
			case '?': 
				printf("Usage: %s [ -n <number of records to write> ] [ -f <output file name> ]\n", argv[0]);	
				exit(1);
		}
	}

	/* open lnf file desriptor */
	if (lnf_open(&filep, filename, LNF_WRITE | ( compress ? LNF_COMP : 0 ), "myfile") != LNF_OK) {
		fprintf(stderr, "Can not open file.\n");
		exit(1);
	}

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
}


