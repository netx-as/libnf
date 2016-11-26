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

#define FILENAME_IN "./test-file.tmp"
#define FILENAME_OUT "./test-file-out.tmp"

#define LLUI long long unsigned int

int main(int argc, char **argv) {

	lnf_file_t *filep_in;
	lnf_file_t *filep_out;
	lnf_rec_t *recp_in;
	lnf_rec_t *recp_out;
	char buf[LNF_REC_RAW_TLV_BUFSIZE];
	char errbuf[LNF_MAX_STRING];
	size_t size;

    char *filename_in = FILENAME_IN;
    char *filename_out = FILENAME_OUT;
	int i = 0;
	int j = 0;
	int c;
	int ret;
	

	while ((c = getopt (argc, argv, "fF:?")) != -1) {
		switch (c) {
			case 'f':
				filename_in = optarg;
				break;
			case 'F':
				filename_out = optarg;
				break;
			case '?':
				printf("Usage: %s [ -p ] [ -f <output file name> ] [ -F <output file name> ]\n", argv[0]);
				exit(1);
		}
	}

	
	if (lnf_open(&filep_in, filename_in, LNF_READ, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename_in);
		exit(1);
	}

	if (lnf_open(&filep_out, filename_out, LNF_WRITE, NULL) != LNF_OK) {
		fprintf(stderr, "Can not open file %s\n", filename_out);
		exit(1);
	}

	lnf_rec_init(&recp_in);
	lnf_rec_init(&recp_out);

	while (lnf_read(filep_in, recp_in) != LNF_EOF) {

		i++;

		if (lnf_rec_get_raw(recp_in, LNF_REC_RAW_TLV, buf, sizeof(buf), &size) == LNF_OK) {
			printf("Raw record size: %zu bytes\n", size);

			/* now, the data in buf cen be transfered somwhere else */

			if ( (ret = lnf_rec_set_raw(recp_out, buf, size)) == LNF_OK) {
				if ( lnf_write(filep_out, recp_out) == LNF_OK) {
					j++;
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

	}

	printf("Total records: %d, transfered records: %d\n", i, j);

	lnf_rec_free(recp_in);
	lnf_rec_free(recp_out);
	lnf_close(filep_in);
	lnf_close(filep_out);

	return 0;
}


