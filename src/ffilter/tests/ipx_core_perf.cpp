/*

 Copyright (c) 2015-2017, Imrich Stoffa

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

/**
 * \file ipx_core_perf.cpp
 * \brief Performance test of ffilter evaluation
 */

#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>

extern "C" {
#include <ffilter.h>
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

enum test_record_field {
	FLD_NONE = 0,
	FLD_SPORT,
	FLD_DPORT,
	FLD_TSTART,
	FLD_TEND,
	FLD_DURATION,
	FLD_PACKETS,
	FLD_BYTES,
	FLD_FLAGS,
	FLD_PROTO,
	FLD_REAL,
	FLD_MAC_ADDR,
	FLD_MPLS,
	FLD_IP_ADDR,
	FLD_IP_ADDR2,
	FLD_TIMESTAMP,
	FLD_MESSAGE,
	FLD_BPP,
	FLD_BINARY_HEAP,
};


struct mock_rec {
	double real;
	int64_t i64;
	int64_t i64_2;
	int32_t i32;
	int32_t i32_2;
	int16_t i16;
	int16_t i16_2;
	int8_t i8;
	int8_t i8_2;
	char message[40];
	char mac[6];
	uint32_t addr[4];
	uint32_t addr_2[4];
	uint32_t mpls[10];
};

/**
 * \brief Mockup lookup func callback to test all ffilter supported data types
 * Lookup func sets lvalue of field according to first letter of valstr and
 * @param filter
 * @param valstr Name of field
 * @param lvalue
 * @return FF_OK on success
 */
ff_error_t test_lookup_func (struct ff_s *filter, const char *valstr, ff_lvalue_t *lvalue)
{
	ff_type_t type;
	ff_extern_id_t id;

	lvalue->id[0].index = FLD_NONE;
	lvalue->options = FF_OPTS_NONE;

	if (!strcmp(valstr, "port")) {
		type = FF_TYPE_UINT16;
		lvalue->id[0].index = FLD_SPORT;
		lvalue->id[0].index = FLD_DPORT;

	} else if (!strcmp(valstr, "srcport")) {
		type = FF_TYPE_UINT16;
		lvalue->id[0].index = FLD_SPORT;

	} else if (!strcmp(valstr, "dstport")) {
		type = FF_TYPE_UINT16;
		lvalue->id[0].index = FLD_DPORT;

	} else if (!strcmp(valstr, "tstart")) {
		type = FF_TYPE_UINT64;
		lvalue->id[0].index = FLD_TSTART;

	} else if (!strcmp(valstr, "tend")) {
		type = FF_TYPE_UINT64;
		lvalue->id[0].index = FLD_TEND;

	} else if (!strcmp(valstr, "duration")) {
		type = FF_TYPE_UINT64;
		lvalue->id[0].index = FLD_DURATION;

	} else if (!strcmp(valstr, "bpp")) {
		type = FF_TYPE_UINT32;
		lvalue->id[0].index = FLD_BPP;

	} else if (!strcmp(valstr, "flags")) {
		type = FF_TYPE_UINT8;
		lvalue->id[0].index = FLD_FLAGS;

	} else if (!strcmp(valstr, "proto")) {
		type = FF_TYPE_UINT8;
		lvalue->id[0].index = FLD_PROTO;

	} else	if (!strcmp(valstr, "bytes")) {
		type = FF_TYPE_UINT32;
		lvalue->id[0].index = FLD_BYTES;

	} else if (!strcmp(valstr, "packets")) {
		type = FF_TYPE_UINT32;
		lvalue->id[0].index = FLD_PACKETS;

	} else if (!strcmp(valstr, "real")) {
		type = FF_TYPE_DOUBLE;
		lvalue->id[0].index = FLD_REAL;

	} else if (!strcmp(valstr, "mac")) {
		type = FF_TYPE_MAC;
		lvalue->id[0].index = FLD_MAC_ADDR;

	} else if (!strcmp(valstr, "mplsLabel1")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS;
		lvalue->n = 0;

	} else if (!strcmp(valstr, "mplsLabel2")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS;
		lvalue->n = 1;

	} else if (!strcmp(valstr, "mplsExp1")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS;
		lvalue->options |= FF_OPTS_MPLS_EXP;
		lvalue->n = 0;

	} else if (!strcmp(valstr, "mplsExp2")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS;
		lvalue->options |= FF_OPTS_MPLS_EXP;
		lvalue->n = 1;

	} else if (!strcmp(valstr, "mplsEos")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS;
		lvalue->options |= FF_OPTS_MPLS_EOS;

	} else if (!strcmp(valstr, "ip")) {
		type = FF_TYPE_ADDR;
		lvalue->id[0].index = FLD_IP_ADDR;
		lvalue->id[1].index = FLD_IP_ADDR2;

    } else if (!strcmp(valstr, "net")) {
        type = FF_TYPE_ADDR;
        lvalue->id[0].index = FLD_IP_ADDR;
        lvalue->id[1].index = FLD_IP_ADDR2;

	} else if (!strcmp(valstr, "srcip")) {
		type = FF_TYPE_ADDR;
		lvalue->id[0].index = FLD_IP_ADDR;

    } else if (!strcmp(valstr, "srcnet")) {
        type = FF_TYPE_ADDR;
        lvalue->id[0].index = FLD_IP_ADDR;

	} else if (!strcmp(valstr, "dstip")) {
		type = FF_TYPE_ADDR;
		lvalue->id[0].index = FLD_IP_ADDR2;

    } else if (!strcmp(valstr, "dstnet")) {
        type = FF_TYPE_ADDR;
        lvalue->id[0].index = FLD_IP_ADDR;

	} else if (!strcmp(valstr, "timestamp")) {
		type = FF_TYPE_TIMESTAMP;
		lvalue->id[0].index = FLD_TIMESTAMP;

	} else if (!strcmp(valstr, "ident")) {
		type = FF_TYPE_STRING;
		lvalue->id[0].index = FLD_MESSAGE;

	} else if (!strcmp(valstr, "heap")) {
		type = FF_TYPE_UNSUPPORTED;
		lvalue->id[0].index = FLD_BINARY_HEAP;

	} else if (!strcmp(valstr, "none")) {
		type = FF_TYPE_UNSUPPORTED;
		lvalue->id[0].index = FLD_NONE;

	} else {
		return FF_ERR_OTHER;
	}

	lvalue->type = type;

	return FF_OK;
}

/**
 * \breif Mockup data func callback to test all ffilter supported data types
 * Test data func selects data from record based on external identification,
 * which was set by lookup callback \see test_lookup_func
 * @param filter
 * @param rec test_record reference
 * @param extid Ident. of field
 * @param buf Selected data are copied here
 * @param size Length of selected data
 * @return FF_OK on data copied
 */
ff_error_t test_data_func (struct ff_s *filter, void *rec, ff_extern_id_t extid, char** buf, size_t *size)
{
	struct mock_rec *trec = (struct mock_rec*)rec;

	uint64_t tmp;
	char *data = *buf;

	switch(extid.index) {
	case FLD_SPORT:
		data = (char*)&trec->i16;
		*size = sizeof(trec->i16);
		break;
	case FLD_DPORT:
		data = (char*)&trec->i16_2;
		*size = sizeof(trec->i16_2);
		break;
	case FLD_TSTART:
		data = (char*)&trec->i64;
		*size = sizeof(trec->i64);
		break;
	case FLD_TEND:
		data = (char*)&trec->i64_2;
		*size = sizeof(trec->i64_2);
		break;
	case FLD_PACKETS:
		data = (char*)&trec->i32_2;
		*size = sizeof(trec->i32_2);
		break;
	case FLD_BYTES:
		data = (char*)&trec->i32;
		*size = sizeof(trec->i32);
		break;
	case FLD_FLAGS:
		data = (char*)&trec->i8;
		*size = sizeof(trec->i8);
		break;
	case FLD_PROTO:
		data = (char*)&trec->i8_2;
		*size = sizeof(trec->i8_2);
		break;

	case FLD_DURATION:
		tmp = trec->i64_2 - trec->i64;
		memcpy(data, &tmp, sizeof(tmp));
		*size = sizeof(tmp);
		break;
	case FLD_BPP:
		tmp = trec->i32_2/(!trec->i32?1:trec->i32);
		memcpy(data, &tmp, sizeof(tmp));
		*size = sizeof(tmp);
		break;
	case FLD_REAL:
		data = (char *)&trec->real;
		*size = sizeof(tmp);
		break;
	case FLD_MAC_ADDR:
		data = (char *)&trec->mac;
		*size = sizeof(ff_mpls_stack_t);
		break;
	case FLD_MPLS:
		data = (char *)&trec->mpls;
		*size = sizeof(ff_mpls_stack_t);
		break;
	case FLD_IP_ADDR:
		if (trec->addr[0] == 0 && trec->addr[1] == 0 &&
		    trec->addr[2] == 0 && trec->addr[3] != 0) {
			data = (char *) &trec->addr[3];
			*size = 4;
		} else {
			data = (char *) &trec->addr[0];
			*size = 16;
		}
		break;
	case FLD_IP_ADDR2:
		if (trec->addr_2[0] == 0 && trec->addr_2[1] == 0 &&
		    trec->addr_2[2] == 0 && trec->addr_2[3] != 0) {
			data = (char *) &trec->addr_2[3];
			*size = 4;
		} else {
			data = (char *) &trec->addr_2[0];
			*size = 16;
		}
		break;
	case FLD_MESSAGE:
		trec->message[39]=0;
		data = &trec->message[0];
		*size = strlen(&trec->message[0]);
		break;
	case FLD_BINARY_HEAP:
	case FLD_NONE:
	default : *size = 0; return FF_ERR_OTHER;
	}

	*buf = data;

	return FF_OK;
}

/**
 * \brief Mockup map callback func to test constants translation
 * Test constants translatoin function converts literals for signed and unsigned field type
 * For testing purposes only two constants are available for now
 * tenBelow -> -10
 * megabyte -> 1024*1024
 *
 * @param filter
 * @param valstr Literal
 * @param test_type Data type of field
 * @param extid	Field identification
 * @param buf Translated data are copied here
 * @param size Length of data copied
 * @return FF_OK on successfull translation
 */
ff_error_t test_rval_map_func (struct ff_s * filter, const char *valstr, ff_type_t test_type, ff_extern_id_t extid, char* buf, size_t *size)
{
	if (test_type == FF_TYPE_SIGNED || test_type == FF_TYPE_UNSIGNED || test_type == FF_TYPE_INT16) {
		if (!strcmp(valstr, "magic_number")) {
			*(uint64_t *) buf = 6996;
			*size = sizeof(uint64_t);
		} else if (!strcmp(valstr, "kilobyte")) {
			*(uint64_t *) buf = 1000;
			*size = sizeof(uint64_t);
		} else {
			*size = 0;
			return FF_ERR_OTHER;
		}
		return FF_OK;
	}
	return FF_ERR_OTHER;
}


class filter_perf_test : public :: testing::Test {
protected:

	char *expr;
	struct mock_rec rec;
	ff_options_t* test_callbacks;
	ff_t *filter = NULL;
	char *buffer;

	virtual void SetUp() {
		ff_options_init(&test_callbacks); //Prepare structure for callbacks
		buffer = (char*)malloc(FF_MAX_STRING);	//Alloc extra buffer

		test_callbacks->ff_data_func = test_data_func;
		test_callbacks->ff_lookup_func = test_lookup_func;
		test_callbacks->ff_rval_map_func = test_rval_map_func;

		memset(&rec, 0, sizeof(struct mock_rec));
		filter = NULL;
		expr = NULL;
	}

	virtual void TearDown() {
		ff_options_free(test_callbacks);
		free(buffer);
		ff_free(filter);
	}

	/**
	 * \brief Helper functions to work with mocked record
	 * Purpose is just to fill right fields in record, types are
	 * dynamic according to FF_TYPE of filter identifier
	 */
	void fillInt64_2(int64_t val)
	{
		rec.i64_2 = val;
	}
	void fillInt64(int64_t val) {
		rec.i64 = val;
	}
	void fillInt32(int32_t val) {
		rec.i32 = val;
	}
	void fillInt32_2(int32_t val) {
		rec.i32_2 = val;
	}
	void fillInt16(int16_t val) {
		rec.i16 = val;
	}
	void fillInt16_2(int16_t val) {
		rec.i16_2 = val;
	}
	void fillInt8(int8_t val) {
		rec.i8 = val;
	}
	void fillInt8_2(int8_t val) {
		rec.i8_2 = val;
	}
	void fillReal(double val) {
		rec.real = val;
	}
	void fillMessage(const char* val) {
		strncpy(&rec.message[0], val, 40);
	}
	void fillIP(const char* val)
	{
		if (inet_pton(AF_INET, val, &rec.addr[3])) {}
		else if (inet_pton(AF_INET6, val, &rec.addr[0])) {}
		else {rec.addr[0] = 0;}
	}
	void fillIP_2(const char* val)
	{
		if (inet_pton(AF_INET, val, &rec.addr_2[3])) {}
		else if (inet_pton(AF_INET6, val, &rec.addr_2[0])) {}
		else {rec.addr[0] = 0;}
	}

	void fillMAC(char val1, char val2, char val3, char val4, char val5, char val6) {
		rec.mac[0]=val1;
		rec.mac[1]=val2;
		rec.mac[2]=val3;
		rec.mac[3]=val4;
		rec.mac[4]=val5;
		rec.mac[5]=val6;
	}
	void fillMPLS(const char* val) {
		memcpy(rec.mpls, val, 40);
	}

/**
 *  \brief Helper functions to shorten parameter list and better readability
 */
	ff_error_t init(const char* str)
	{
		ff_free(filter);
		return ff_init(&filter, str, test_callbacks);
	}

	int eval(struct mock_rec* rec) {
		return ff_eval(filter, (char *)rec);
	}
};

TEST_F(filter_perf_test, ipv4_perf)
{
	srand(time(0));
	struct mock_rec * rec_list = (struct mock_rec*)malloc(4096*sizeof(struct mock_rec));

	init("ip 192.168.2.64");
	fillIP("192.168.2.64");
	rec_list[0].addr[3] = rec.addr[3];
	for (int x = 1; x < 4096; x++) {
		rec_list[x].addr[3] = rand();
	}

	clock_t t;
	t = clock();
	int32_t sum = 0;
	for (int64_t x = 0; x < 10000000LL; x++) {
		sum += ff_eval(filter, &rec_list[x & 0x0fff]);
	}
	t = clock() - t;
	printf("Evaluation took %lf seconds\nPerformance: %.0lf ip per second\nMatches: %d\n",
	       ((double)t/CLOCKS_PER_SEC), 1/(((double)t)/(CLOCKS_PER_SEC))*10000000, sum);

	free(rec_list);
}

TEST_F(filter_perf_test, leaves_perf)
{
	srand(time(0));
	struct mock_rec * rec_list = (struct mock_rec*)malloc(4096*sizeof(struct mock_rec));
	char * x = (char *)(rec_list+1);

#define PLEN 200
#define REC_N 1000000
	char program[PLEN];
	int nodes;
	int set=0;

	FILE* filters;
	filters = fopen("../tests/perf-test.in","r");

	filter = NULL;


	fillIP("192.168.2.15");
	rec_list[0].addr[3] = rec.addr[3];

	//Init the record somehow
	for (; x < (char *) (rec_list + 4096); x++)
		*x = rand();

	clock_t t;
	printf("\n Set, Nodes, duration sec, performance records/sec, matches");
	while(!feof(filters)) {
		set++;

		if(fscanf(filters, "%d", &nodes) < 1) {
			break;
		}
		fgets(&program[0], sizeof(program), filters);

		if(init(&program[0])!=FF_OK) {
			fprintf(stderr, "Can not init filter '%s'\n", &program[0]);
			ff_error(filter, &program[0], PLEN);
			fprintf(stderr, "%s\n", &program[0]);
			continue;
		}

		t = clock();
		int32_t sum = 0;
		for (int64_t x = 0; x < REC_N; x++) {
			sum += ff_eval(filter, &rec_list[x & 0x0fff]);
		}
		t = clock() - t;
		printf("\n%d,\t%d,\t%lf,\t%.0lf,\t%d",
		       set, nodes, ((double) t / CLOCKS_PER_SEC),
		       ((double) REC_N)*((CLOCKS_PER_SEC)/((double)t)),
		       sum);
	}
	printf("\n");
	if (filters)
		fclose(filters);
	free(rec_list);
}
