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
 * \file ipx_filter_data_types.cpp
 * \brief Unit test for conversion of string data to ff_val_t and their evaluation
 * This script provides usage testing of various data types provided by ffilter core
 * this is usefull to chceck overall state of ffilter.
 */

#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

extern "C" {
#include <ffilter.h>
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

enum test_record_field {
	FLD_NONE = 0,
	FLD_SRC_NUMBER,
	FLD_DST_NUMBER,
	FLD_NUMBER64,
	FLD_NUMBER32,
	FLD_NUMBER16,
	FLD_NUMBER8,
	FLD_REAL,
	FLD_MAC_ADDR,
	FLD_MPLS_STACK_LABEL,
	FLD_IP_ADDR,
	FLD_TIMESTAMP,
	FLD_MESSAGE,
	FLD_BINARY_HEAP,
};


struct mock_rec {
	double real;
	int64_t i64;
	int64_t i64_2;
	int32_t i32;
	int16_t i16;
	int8_t i8;
	char message[40];
	char mac[6];
	uint32_t addr[4];
	uint32_t mpls[10];
};

/**
 * \brief Mockup lookup func callback to test all ffilter supported data types
 * Lookup func sets lvalue of field according to first letter of valstr and
 * \param filter
 * \param valstr Name of field
 * \param lvalue
 * \return FF_OK on success
 */
ff_error_t test_lookup_func (struct ff_s *filter, const char *valstr, ff_lvalue_t *lvalue)
{
	ff_type_t type;
	ff_extern_id_t id;

	lvalue->id[0].index = FLD_NONE;
	lvalue->options = FF_OPTS_NONE;

	if (!strcmp(valstr, "uint")) {
		type = FF_TYPE_UNSIGNED;
		lvalue->id[0].index = FLD_SRC_NUMBER;
		lvalue->id[1].index = FLD_DST_NUMBER;

	} else if (!strcmp(valstr, "srcuint")) {
		type = FF_TYPE_UNSIGNED;
		lvalue->id[0].index = FLD_SRC_NUMBER;

	} else if (!strcmp(valstr, "dstuint")) {
		type = FF_TYPE_UNSIGNED;
		lvalue->id[0].index = FLD_DST_NUMBER;

	} else if (!strcmp(valstr, "ui64")) {
		type = FF_TYPE_UINT64;
		lvalue->id[0].index = FLD_NUMBER64;

	} else if (!strcmp(valstr, "ui32")) {
		type = FF_TYPE_UINT32;
		lvalue->id[0].index = FLD_NUMBER32;

	} else if (!strcmp(valstr, "ui16")) {
		type = FF_TYPE_UINT16;
		lvalue->id[0].index = FLD_NUMBER16;

	} else if (!strcmp(valstr, "ui8")) {
		type = FF_TYPE_UINT8;
		lvalue->id[0].index = FLD_NUMBER8;

	} else	if (!strcmp(valstr, "int")) {
		type = FF_TYPE_SIGNED;
		lvalue->id[0].index = FLD_SRC_NUMBER;
		lvalue->id[1].index = FLD_DST_NUMBER;

	} else if (!strcmp(valstr, "srcint")) {
		type = FF_TYPE_SIGNED;
		lvalue->id[0].index = FLD_SRC_NUMBER;

	} else if (!strcmp(valstr, "dstint")) {
		type = FF_TYPE_SIGNED;
		lvalue->id[0].index = FLD_DST_NUMBER;

	} else if (!strcmp(valstr, "i64")) {
		type = FF_TYPE_INT64;
		lvalue->id[0].index = FLD_NUMBER64;

	} else if (!strcmp(valstr, "i32")) {
		type = FF_TYPE_INT32;
		lvalue->id[0].index = FLD_NUMBER32;

	} else if (!strcmp(valstr, "i16")) {
		type = FF_TYPE_INT16;
		lvalue->id[0].index = FLD_NUMBER16;

	} else if (!strcmp(valstr, "i8")) {
        type = FF_TYPE_INT8;
        lvalue->id[0].index = FLD_NUMBER8;

    } else if (!strcmp(valstr, "realeq10")) {
        type = FF_TYPE_DOUBLE;
        lvalue->id[0].index = FLD_REAL;
        lvalue->options = FF_OPTS_CONST;
        lvalue->literal = "10.0";

	} else if (!strcmp(valstr, "constfail")) {
        type = FF_TYPE_DOUBLE;
        lvalue->id[0].index = FLD_REAL;
        lvalue->options = FF_OPTS_CONST;
        lvalue->literal = NULL;

	} else if (!strcmp(valstr, "real")) {
		type = FF_TYPE_DOUBLE;
		lvalue->id[0].index = FLD_REAL;

	} else if (!strcmp(valstr, "mac")) {
		type = FF_TYPE_MAC;
		lvalue->id[0].index = FLD_MAC_ADDR;

	} else if (!strcmp(valstr, "mplsLabel")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS_STACK_LABEL;
		lvalue->n = 1;

	} else if (!strcmp(valstr, "mplsExp")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS_STACK_LABEL;
		lvalue->options |= FF_OPTS_MPLS_EXP;
		lvalue->n = 1;

	} else if (!strcmp(valstr, "mplsEos")) {
		type = FF_TYPE_MPLS;
		lvalue->id[0].index = FLD_MPLS_STACK_LABEL;
		lvalue->options |= FF_OPTS_MPLS_EOS;

	} else if (!strcmp(valstr, "addr")) {
		type = FF_TYPE_ADDR;
		lvalue->id[0].index = FLD_IP_ADDR;

	} else if (!strcmp(valstr, "timestamp")) {
		type = FF_TYPE_TIMESTAMP;
		lvalue->id[0].index = FLD_TIMESTAMP;

	} else if (!strcmp(valstr, "message")) {
		type = FF_TYPE_STRING;
		lvalue->id[0].index = FLD_MESSAGE;

	} else if (!strcmp(valstr, "heap")) {
		type = FF_TYPE_UNSUPPORTED;
		lvalue->id[0].index = FLD_BINARY_HEAP;

	} else if (!strcmp(valstr, "flags")) {
		type = FF_TYPE_UNSIGNED;
        lvalue->options = FF_OPTS_FLAGS;
		lvalue->id[0].index = FLD_NUMBER16;

	} else if (!strcmp(valstr, "none")) {
		type = FF_TYPE_UINT8;
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
 * \param filter
 * \param rec   test_record reference
 * \param extid Ident. of field
 * \param buf   Pointer to data is passed in this variable.
 * \param size  Length of selected data
 * \return FF_OK on data copied
 */
ff_error_t test_data_func (struct ff_s *filter, void *rec, ff_extern_id_t extid, char** buf, size_t *size)
{
	struct mock_rec *trec = (struct mock_rec*)rec;

	char *data;

	switch(extid.index) {
	case FLD_SRC_NUMBER:
		data = (char *)&trec->i64;
		*size = 8;
		break;
	case FLD_DST_NUMBER:
		data = (char *)&trec->i64_2;
		*size = 8;
		break;
	case FLD_NUMBER64:
		data = (char *)&trec->i64;
		*size = 8;
		break;
	case FLD_NUMBER32:
		data = (char *)&trec->i32;
		*size = 4;
		break;
	case FLD_NUMBER16:
		data = (char *)&trec->i16;
		*size = 2;
		break;
	case FLD_NUMBER8:
		data = (char *)&trec->i8;
		*size = 1;
		break;
	case FLD_REAL:
		data = (char *)&trec->real;
		*size = sizeof(double);
		break;
	case FLD_MAC_ADDR:
		data = (char *)&trec->mac;
		*size = sizeof(ff_mac_t);
		break;
	case FLD_MPLS_STACK_LABEL:
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
	case FLD_TIMESTAMP:
		data = (char *)&trec->i64;
		*size = 8;
		break;
	case FLD_MESSAGE:
		data = &trec->message[0];
		*size = sizeof(char*);
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
 * \param filter
 * \param valstr Literal
 * \param test_type Data type of field
 * \param extid	Field identification
 * \param buf Translated data are copied here
 * \param size Length of data copied
 * \return FF_OK on successfull translation
 */
ff_error_t test_rval_map_func (struct ff_s *filter, const char *valstr, ff_type_t type, ff_extern_id_t id, char *buf, size_t *size)
{
	if (!strcmp(valstr, "magic_number")) {
		*(uint64_t *) buf = 6996;
		*size = sizeof(uint64_t);
		return FF_OK;
	} else if (!strcmp(valstr, "kilobyte")) {
		*(uint64_t *) buf = 1000;
		*size = sizeof(uint64_t);
		return FF_OK;
	} else {
		*size = 0;
		return FF_ERR_OTHER;
	}
}


class filter_types_test : public :: testing::Test {
protected:

	char *expr;
	struct mock_rec rec;
	ff_options_t* test_callbacks;
	ff_t *filter = NULL;
	char *buffer;

	virtual void SetUp() {
        // Prepare structure for callbacks
		ff_options_init(&test_callbacks);
        // Alloc extra buffer
		buffer = (char*)malloc(FF_MAX_STRING);

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
	void fillInt16(int16_t val) {
		rec.i16 = val;
	}
	void fillInt8(int8_t val) {
		rec.i8 = val;
	}
	void fillReal(double val) {
		rec.real = val;
	}
	void fillMessage(const char* val) {
		strncpy(&rec.message[0], val, 40);
	}
	void fillIP(const char* val)
	{
		if (inet_pton(AF_INET, val, &rec.addr[3])) {
            rec.addr[0] = 0;
            rec.addr[1] = 0;
            rec.addr[2] = 0;
        }
		else if (inet_pton(AF_INET6, val, &rec.addr[0])) {}
		else {
            rec.addr[0] = 0;
            rec.addr[1] = 0;
            rec.addr[2] = 0;
            rec.addr[3] = 0;
        }
	}

	void fillMAC(int val1, int val2, int val3, int val4, int val5, int val6) {
		rec.mac[0] = (uint8_t)val1;
		rec.mac[1] = (uint8_t)val2;
		rec.mac[2] = (uint8_t)val3;
		rec.mac[3] = (uint8_t)val4;
		rec.mac[4] = (uint8_t)val5;
		rec.mac[5] = (uint8_t)val6;
	}

    void fillMAC(const char* val) {
        memcpy(&rec.mac[0], val, 6);
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


// Left associable, priotrities 1.NEG 2.AND, 3.OR
// Use brackets to modify
TEST_F(filter_types_test, Logic_expressions)
{
	ASSERT_EQ(FF_OK, init("srcint 10"));
	fillInt64(10);
	fillInt64_2(5);
	fillMessage("ahoj");
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("not srcint 10"));
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcint 10 or message ahoj"));
	fillInt64(3);
	EXPECT_TRUE(eval(&rec));

	// And precedence
	ASSERT_EQ(FF_OK, init("srcint 10 or message ahoj and addr 192.168.0.1"));
	fillIP("192.168.0.1");
	EXPECT_TRUE(eval(&rec));
	ASSERT_EQ(FF_OK, init("srcint 10 or message ahoj and not addr 192.168.0.1"));
	EXPECT_FALSE(eval(&rec));
	ASSERT_EQ(FF_OK, init("not srcint 10 or message ahoj and addr 192.168.0.1"));
	EXPECT_TRUE(eval(&rec));
	ASSERT_EQ(FF_OK, init("not (srcint 10 or message ahoj and addr 192.168.0.1)"));
	EXPECT_FALSE(eval(&rec));
	fillIP("192.168.0.2");
	EXPECT_TRUE(eval(&rec));

}

TEST_F(filter_types_test, Multinode_eval)
{
	// Init must be successful otherwise there's no point to continue
	ASSERT_EQ(FF_OK, init("uint 10"));
	// Value in 1. or 2. field should match
	fillInt64(10);
	fillInt64_2(0);
	EXPECT_TRUE(eval(&rec));

	fillInt64(0);
	fillInt64_2(10);
	EXPECT_TRUE(eval(&rec));

	fillInt64(0);
	fillInt64_2(0);
	EXPECT_FALSE(eval(&rec)); //None of both matches - should fail
	// Cleanup is automatic
}

TEST_F(filter_types_test, After_error_reinit)
{
    ASSERT_NE(FF_OK, init("int in [ 10, badinput, 11, 12]"));

    ASSERT_EQ(FF_OK, init("int in [ 10, -10 ]"));
}

TEST_F(filter_types_test, coma_separator_in_list)
{
	ASSERT_EQ(FF_OK, init("uint in [10, 11, 12]"));
	fillInt64(1);
	fillInt64_2(11);
	EXPECT_TRUE(eval(&rec));

	EXPECT_NE(FF_OK, init("uint in [,10, 11,]"));
}

TEST_F(filter_types_test, unsigned_integer) {

	// Range + default operator test (eq)
	ASSERT_EQ(FF_OK, init("srcuint 18446744073709551615")); // Just right
	fillInt64(UINT64_MAX);
	EXPECT_TRUE(eval(&rec)); // Equality test

	ASSERT_EQ(FF_OK, init("srcuint = 0x01020408")); // = test and hexa input
	fillInt64(0x01020408);
	EXPECT_TRUE(eval(&rec));
	fillInt64(0x01020409);
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcuint > 0x01020408")); // > test and hexa input
	fillInt64(0x02000000);
	EXPECT_TRUE(eval(&rec));
	fillInt64(0x01000000);
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcuint < 0x01020408")); // < test and hexa input
	fillInt64(0x02000000);
	EXPECT_FALSE(eval(&rec));
	fillInt64(0x01000000);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcuint & 0x0d")) << &filter->error_str[0]; // flag test
	fillInt64(0x1e);
	EXPECT_FALSE(eval(&rec));
	fillInt64(0x1F);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcuint in [10 52 8]")) << &filter->error_str[0];  // list compare test and input
	fillInt64(52);
	EXPECT_TRUE(eval(&rec));
	fillInt64(10);
	EXPECT_TRUE(eval(&rec));
	fillInt64(8);
	EXPECT_TRUE(eval(&rec));
	fillInt64(100);
	EXPECT_FALSE(eval(&rec));

	// Negative
	EXPECT_NE(FF_OK, init("srcuint 18446744073709551616")); // Over max

	EXPECT_NE(FF_OK, init("srcuint -1")); // Negative

	EXPECT_NE(FF_OK, init("srcuint invalid-input"));
}

TEST_F(filter_types_test, signed_integer)
{
	// Range + default operator test (eq)
	ASSERT_EQ(FF_OK, init("srcint 9223372036854775807")); // Just right
	fillInt64(INT64_MAX);
	EXPECT_TRUE(eval(&rec)); // Equality test

	ASSERT_EQ(FF_OK, init("srcint -9223372036854775808")); // Just right
	fillInt64(INT64_MIN);
	EXPECT_TRUE(eval(&rec)); // Equality test

	ASSERT_EQ(FF_OK, init("srcint = 0x01020408")); // = test and hexa input
	fillInt64(0x01020408);
	EXPECT_TRUE(eval(&rec));
	fillInt64(0x01020409);
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcint > 0x01020408")); // > test and hexa input
	fillInt64(0x02000000);
	EXPECT_TRUE(eval(&rec));
	fillInt64(-65535);
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcint < 0x01020408")); // < test and hexa input
	fillInt64(0x02000000);
	EXPECT_FALSE(eval(&rec));
	fillInt64(-65535);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcint & 0x0d")) << &filter->error_str[0]; // flag test
	fillInt64(0x1e);
	EXPECT_FALSE(eval(&rec));
	fillInt64(0x1F);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("srcint in [10 -52 8]")) << &filter->error_str[0];  // list compare test and input
	fillInt64(-52);
	EXPECT_TRUE(eval(&rec));
	fillInt64(10);
	EXPECT_TRUE(eval(&rec));
	fillInt64(8);
	EXPECT_TRUE(eval(&rec));
	fillInt64(-100);
	EXPECT_FALSE(eval(&rec));

	//Negative
	EXPECT_NE(FF_OK, init("srcint 9223372036854775808")); // Over max

	EXPECT_NE(FF_OK, init("srcint -9223372036854775809")); // Under min

	EXPECT_NE(FF_OK, init("srcint invalid-input"));
}


// Experimental
TEST_F(filter_types_test, flags)
{
    ASSERT_EQ(FF_OK, init("flags 0x82"));
    fillInt16(0x8f);
    EXPECT_TRUE(eval(&rec));
    fillInt16(0x80);
    EXPECT_FALSE(eval(&rec));

    ASSERT_EQ(FF_OK, init("not flags 0x70 and not flags 0x01"));
    fillInt16(0x80);
    EXPECT_TRUE(eval(&rec));
    fillInt16(0x70);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x71);
    EXPECT_FALSE(eval(&rec));

    ASSERT_EQ(FF_OK, init("not flags 0x71"));
    fillInt16(0x8e);
    EXPECT_TRUE(eval(&rec));
    fillInt16(0x170);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x111);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x71);
    EXPECT_FALSE(eval(&rec));

    ASSERT_EQ(FF_OK, init("flags 0x2 and not flags 0x8d"));
    fillInt16(0x8f);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x2);
    EXPECT_TRUE(eval(&rec));

    ASSERT_EQ(FF_OK, init("flags 0x81 and not flags 0x7e"));
    fillInt16(0x7f);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x2);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x83);
    EXPECT_FALSE(eval(&rec));

    ASSERT_EQ(FF_OK, init("flags 0x0182 and not flags 0x8e7d"));
    fillInt16(0xee8f);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x8f);
    EXPECT_FALSE(eval(&rec));
    fillInt16(0x182);
    EXPECT_TRUE(eval(&rec));
    fillInt16(0x183);
    EXPECT_FALSE(eval(&rec));
}

TEST_F(filter_types_test, ip_addr)
{
	ASSERT_EQ(FF_OK, init("addr 192.168.0.1"));
	fillIP("192.168.0.1");
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("addr 192.168.0.1 255.255.255.0"));
	fillIP("192.168.0.230");
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("addr = 194/4"));
	fillIP("192.168.0.230");
	EXPECT_TRUE(eval(&rec));

    ASSERT_EQ(FF_OK, init("addr 195.113/16"));
    fillIP("2001:4118:10:4000:7886:21:0d:bcf9:b32f");
    EXPECT_FALSE(eval(&rec));
    fillIP("195.113.0.230");
    EXPECT_TRUE(eval(&rec));

	//Mixed types in KW - exact and prefix
	ASSERT_EQ(FF_OK, init("addr in [192.168.0.1 10.10/16 172.16.8/24]"));
	fillIP("10.10.10.1");
	EXPECT_TRUE(eval(&rec));
	fillIP("172.16.9.23");
	EXPECT_FALSE(eval(&rec));

	//Unified
	ASSERT_EQ(FF_OK, init("addr in [192.168/16 10.10/16 172.16.8/24]"));
	fillIP("10.10.10.1");
	EXPECT_TRUE(eval(&rec));
	fillIP("172.16.9.23");
	EXPECT_FALSE(eval(&rec));

	EXPECT_EQ(FF_OK, init("addr in [ \"192.168.0.1 255.255.255.0\" ]"));
	fillIP("192.168.0.240");
	EXPECT_TRUE(eval(&rec));
	fillIP("255.255.255.0");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("addr 2a02:26f0:64::170e:5cf7"));
	fillIP("2a02:26f0:64::170e:5cf7");
	EXPECT_TRUE(eval(&rec));
	fillIP("2a02:26f0:64::170e:0");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("addr 2008:608::1"));
	fillIP("2008:608::1");
	EXPECT_TRUE(eval(&rec));
	fillIP("2008:1608::1");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("addr 2008:608::0/ 32"));
	fillIP("2008:608::50:1");
	EXPECT_TRUE(eval(&rec));
	fillIP("2008:609::50:1");
	EXPECT_FALSE(eval(&rec));

    ASSERT_EQ(FF_OK, init("addr 2008:608::0/ 16"));
    fillIP("2008:608::50:1");
    EXPECT_TRUE(eval(&rec));
    fillIP("2008:609::50:1");
    EXPECT_TRUE(eval(&rec));
    fillIP("32.0.0.0");
    EXPECT_FALSE(eval(&rec));

    ASSERT_EQ(FF_OK, init("addr 2008:608::0/ 4"));
    fillIP("32.0.0.0");
    EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("addr 2008:0:0:0:0:0:0:2"));
	fillIP("2008:0:0:0:0:0:0:2");
	EXPECT_TRUE(eval(&rec));
	ASSERT_EQ(FF_OK, init("addr = fe80::e6f8:9cff:fedc:5b77"));
	fillIP("fe80::e6f8:9cff:fedc:5b77");
	EXPECT_TRUE(eval(&rec));

	EXPECT_EQ(FF_OK, init("addr 192.168.0.1 255.255.240.240"));
	fillIP("192.168.16.17");
	EXPECT_FALSE(eval(&rec));

	//Negative
	EXPECT_NE(FF_OK, init("addr 194/11"));
	EXPECT_NE(FF_OK, init("addr 323.123.13.12"));
	EXPECT_NE(FF_OK, init("addr c3.12.a3.FF"));
	EXPECT_NE(FF_OK, init("addr 65535"));
	EXPECT_NE(FF_OK, init("addr 256/4"));
	EXPECT_NE(FF_OK, init("addr www.google.com"));
	EXPECT_NE(FF_OK, init("addr 2008:608::0 /32"));
	EXPECT_NE(FF_OK, init("addr > 192.168.0.1"));
	EXPECT_NE(FF_OK, init("addr < 192.168.0.1"));
	EXPECT_NE(FF_OK, init("addr & 192.168.0.1"));
	EXPECT_NE(FF_OK, init("addr in [ 192.168.0.1 255.255.255.0 ahoj]"));
}

TEST_F(filter_types_test, mac)
{
	ASSERT_EQ(FF_OK, init("mac aa:bb:cc:dd:ee:ff"));
	fillMAC(0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);
    //fillMAC("\xaa\xbb\xcc\xdd\xee\xff\x00");
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mac 01:23:45:56:67:89"));
	fillMAC(0x01, 0x23, 0x45, 0x56, 0x67, 0x89);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mac = 1:23:45:56:67:0x89"));
	fillMAC(0x01, 0x23, 0x45, 0x56, 0x67, 0x89);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mac in [ 1:23:45:56:67:0 0:0:0:0:0:0]"));
	fillMAC(0x01, 0x23, 0x45, 0x56, 0x67, 0x0);
	EXPECT_TRUE(eval(&rec));
	fillMAC(0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);
	EXPECT_FALSE(eval(&rec));

	EXPECT_NE(FF_OK, init("mac > 1:23:45:67:89:0"));
	EXPECT_NE(FF_OK, init("mac < 1:23:45:67:89:0"));
	EXPECT_NE(FF_OK, init("mac & 1:23:45:67:89:0"));

	EXPECT_NE(FF_OK, init("mac 9:0:g:h:33:23223"));
	EXPECT_NE(FF_OK, init("mac 9:0:g:h:33:23:1:1:1"));
	EXPECT_NE(FF_OK, init("mac popokatepetl"));
}


TEST_F(filter_types_test, string)
{
	ASSERT_EQ(FF_OK, init("message Helloworld"));
	fillMessage("Helloworld");
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("message & world"));
	fillMessage("Hello world");
	EXPECT_TRUE(eval(&rec));
	fillMessage("worl");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("message = Helloworld"));
	fillMessage("Helloworld");
	EXPECT_TRUE(eval(&rec));
	fillMessage("world");
	EXPECT_FALSE(eval(&rec));

	ASSERT_NE(FF_OK, init("message < Helloworld"));
//	fillMessage("world");
//	EXPECT_TRUE(eval(&rec));
//	fillMessage("worldworldworld");
//	EXPECT_FALSE(eval(&rec));

	ASSERT_NE(FF_OK, init("message > Helloworld"));
//	fillMessage("world");
//	EXPECT_FALSE(eval(&rec));
//	fillMessage("worldworldworld");
//	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("message in [ Helloworld tlrd etc... ]"));
	fillMessage("tlrd");
	EXPECT_TRUE(eval(&rec));
	fillMessage("etc...");    EXPECT_TRUE(eval(&rec));
	EXPECT_TRUE(eval(&rec));
	fillMessage("world");
	EXPECT_FALSE(eval(&rec));
	fillMessage("foobar");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("message Helloworld"));
	ASSERT_EQ(FF_OK, init("message Hello world"));
	EXPECT_EQ(FF_OK, init("message \"multi word string whiht no quotes\""));

	EXPECT_NE(FF_OK, init("message !@#$%^&*()_+=-0987654321`~"));
	EXPECT_NE(FF_OK, init("message multi word string whiht no quotes"));
}

TEST_F(filter_types_test, real)
{
	ASSERT_EQ(FF_OK, init("real 0.0001")); //Does it makes sense to test for equality of double number ?
	fillReal(1e-4);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("real = 0.0001")); //Does it makes sense to test for equality of double number ?
	fillReal(1e-4);
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("real > 3.13"));
	fillReal(3.14);
	EXPECT_TRUE(eval(&rec));
	fillReal(3.12);
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("real < 3.13e-1"));
	fillReal(0.2);
	EXPECT_TRUE(eval(&rec));
	fillReal(1.2);
	EXPECT_FALSE(eval(&rec));

	// Does this makes sense ?
	ASSERT_EQ(FF_OK, init("real in [1.59 -9.87654321e3 0.1e-12]"));
	fillReal(-9.87654321e3);
	EXPECT_TRUE(eval(&rec));
	fillReal(9.86);
	EXPECT_FALSE(eval(&rec));

	// Does not makes sense but is compilable
	EXPECT_NE(FF_OK, init("real & 5.14"));

	EXPECT_NE(FF_OK, init("real foobar"));
	EXPECT_NE(FF_OK, init("real 0x2f29835.7e-10"));
	EXPECT_NE(FF_OK, init("real #@!$"));
	EXPECT_NE(FF_OK, init("real \x01\x10\x13"));
	EXPECT_NE(FF_OK, init("real in [ 10.1 invalid ]"));
}

TEST_F(filter_types_test, decode_constant)
{
    ASSERT_EQ(FF_OK, init("realeq10"));
    fillReal(10.0);
    EXPECT_TRUE(eval(&rec));
    fillReal(11.0);
    EXPECT_FALSE(eval(&rec));

    ASSERT_NE(FF_OK, init("constfail"));
    EXPECT_STRNE("No Error.", ff_error(filter, buffer, FF_MAX_STRING));
}

// Mpls uses same input routine for all three subtypes
// The trick here is that n variable must be set to 1 to mark desired label to be evaluated
TEST_F(filter_types_test, mpls_Label)
{
	ASSERT_EQ(FF_OK, init("mplsLabel 0"));
	// According to ndfump 1.label 2.exp 3.eos: r->mpls_label[0] >> 4 , (r->mpls_label[0] & 0xF ) >> 1, r->mpls_label[0] & 1,
	// Setting 1st label
	fillMPLS("\x0f\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x0f\x00\xa0\x00");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsLabel = 36"));
	fillMPLS("\x4f\x02\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x60\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsLabel 574373")); // \b 1000 1100 0011 1010 0101 20bits to make sense of ordering
	fillMPLS("\x5f\x3a\x8c\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x57\x3a\x8c\x01");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x67\x3a\x7c\x01");
	EXPECT_FALSE(eval(&rec));

	EXPECT_EQ(FF_OK, init("mplsLabel in [10 11]"));

	EXPECT_NE(FF_OK, init("mplsLabel > 10"));
	EXPECT_NE(FF_OK, init("mplsLabel < 10"));

	EXPECT_NE(FF_OK, init("mplsLabel & 10"));
	EXPECT_NE(FF_OK, init("mplsLabel invalid-input"));
}

// EOS test for position of eos mark counting from 1
TEST_F(filter_types_test, mpls_Eos)
{
	ASSERT_EQ(FF_OK, init("mplsEos 1"));
	fillMPLS("\x57\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x56\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsEos = 2"));
	fillMPLS("\x57\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));
	fillMPLS("\x56\x00\x00\x00"
			 "\x51\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsEos 5"));
	fillMPLS("\x51\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));
	fillMPLS("\x50\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));
	fillMPLS("\x56\x00\x00\x00"
			 "\x66\x00\x00\x00"
			 "\x72\x00\x00\x00"
			 "\x50\x04\x00\x00"
			 "\x51\x02\x00\x00");
	EXPECT_TRUE(eval(&rec));

	EXPECT_EQ(FF_OK, init("mplsEos in [10 11]"));
	EXPECT_EQ(FF_OK, init("mplsEos > 10"));
	EXPECT_EQ(FF_OK, init("mplsEos < 10"));

	//Neg for invalid operators

	EXPECT_NE(FF_OK, init("mplsEos & 10"));
	EXPECT_NE(FF_OK, init("mplsEos invalid-input"));
}

TEST_F(filter_types_test, mpls_Exp)
{
	ASSERT_EQ(FF_OK, init("mplsExp 0"));
	fillMPLS("\x51\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x00\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\xf1\x0f\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x4b\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsExp 3"));
	fillMPLS("\x56\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x4b\x00\x0f\x00");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsExp 1"));
	fillMPLS("\x43\x00\x0f\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x45\x00\x0f\x00");
	EXPECT_FALSE(eval(&rec));

	ASSERT_EQ(FF_OK, init("mplsExp = 7"));
	fillMPLS("\x3e\x00\x00\x00");
	EXPECT_TRUE(eval(&rec));
	fillMPLS("\x3a\x00\x00\x00");
	EXPECT_FALSE(eval(&rec));

	EXPECT_EQ(FF_OK, init("mplsExp in [10 11]"));
	EXPECT_EQ(FF_OK, init("mplsExp > 10"));
	EXPECT_EQ(FF_OK, init("mplsExp < 10"));

	EXPECT_EQ(FF_OK, init("mplsExp & 10"));

	// Negative for invalid operators
	EXPECT_NE(FF_OK, init("mplsExp invalid-input"));
}

