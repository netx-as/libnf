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
 * \file ff_filter_convertors.cpp
 * \brief Unit test of str to ff_val convertors
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <iostream>
#include <arpa/inet.h>
#include <string>

extern "C" {
#include <ffilter.h>
#include <ffilter_internal.h>
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

class str_to_number_test : public :: testing::Test {
protected:
	virtual void SetUp() {
        filter = NULL;

        ff_options_init(&test_callbacks); //Prepare structure for callbacks

        test_callbacks->ff_data_func = NULL;
        test_callbacks->ff_lookup_func = NULL;
        test_callbacks->ff_rval_map_func = NULL;

        ff_init(&filter, "any", test_callbacks);
        ff_options_free(test_callbacks);

		ptr = NULL;
		size = 0;
	}

	virtual void TearDown() {
        ff_free(filter);
	}


    void fail_convert_int(const char* valstr, ff_type_t int_size)
    {
        ASSERT_TRUE(str_to_int(filter, const_cast<char*>(valstr), int_size, &ptr, &size));
    }

    void convert_int(int64_t expect, const char* valstr, ff_type_t int_size)
    {
        ASSERT_FALSE(str_to_int(filter, const_cast<char*>(valstr), int_size, &ptr, &size)) << "Failed to convert \""
                    << valstr << "\" to int";
        // Data has correct length
        // And compare results
        switch(size) {
        case FF_TYPE_INT64:
            EXPECT_EQ(sizeof(int64_t), size);
            EXPECT_EQ(expect, *((int64_t *) ptr));
            break;
        case FF_TYPE_INT32:
            EXPECT_EQ(sizeof(int32_t), size);
            EXPECT_EQ(expect, *((int32_t *) ptr));
            break;
        case FF_TYPE_INT16:
            EXPECT_EQ(sizeof(int16_t), size);
            EXPECT_EQ(expect, *((int16_t *) ptr));
            break;
        case FF_TYPE_INT8:
            EXPECT_EQ(sizeof(int8_t), size);
            EXPECT_EQ(expect, *((int8_t *) ptr));
            break;
        }
        free(ptr);
        return;
    }

    void fail_convert_uint(const char* valstr, ff_type_t int_size)
    {
        ASSERT_TRUE(str_to_uint(filter, const_cast<char*>(valstr), int_size, &ptr, &size));
    }

	void convert_uint(uint64_t expect, const char* valstr, ff_type_t uint_size)
	{
		ASSERT_FALSE(str_to_uint(filter, const_cast<char*>(valstr), uint_size, &ptr, &size)) << "Failed to convert \""
																		<< valstr << "\" to unsigned int";
		// Data has correct length
        // And compare results
        switch(size) {
        case FF_TYPE_UINT64:
            EXPECT_EQ(sizeof(uint64_t), size);
            EXPECT_EQ(expect, *((uint64_t *) ptr));
            break;
        case FF_TYPE_UINT32:
            EXPECT_EQ(sizeof(uint32_t), size);
            EXPECT_EQ(expect, *((uint32_t *) ptr));
            break;
        case FF_TYPE_UINT16:
            EXPECT_EQ(sizeof(uint16_t), size);
            EXPECT_EQ(expect, *((uint16_t *) ptr));
            break;
        case FF_TYPE_UINT8:
            EXPECT_EQ(sizeof(uint8_t), size);
            EXPECT_EQ(expect, *((uint8_t *) ptr));
            break;
        }
		free(ptr);
		return;
	}

	void convert_real(ff_double_t expect, char* valstr)
	{
		ASSERT_FALSE(str_to_real(filter, valstr, &ptr, &size)) << "Failed to convert \""
															 << valstr << "\" to real";
		//Data has correct length
		EXPECT_EQ(sizeof(double), size) << "Size mismatch \""
										<< valstr << "\"";
		//And compare results
		EXPECT_EQ(expect, *((double *) ptr)) << "Unexpected result of conversion";
		free(ptr);
		return;
	}

    ff_options_t *test_callbacks;
    ff_t *filter;

	char *ptr;
	size_t size;
};

class str_to_addr_test : public :: testing::Test {
protected:
	virtual void SetUp() {
        filter = NULL;

        ff_options_init(&test_callbacks); //Prepare structure for callbacks

        test_callbacks->ff_data_func = NULL;
        test_callbacks->ff_lookup_func = NULL;
        test_callbacks->ff_rval_map_func = NULL;

        ff_init(&filter, "any", test_callbacks);
        ff_options_free(test_callbacks);

		ptr = NULL;
		size = 0;
		addrstr[0] = 0;
		maskstr[0] = 0;
	}
	virtual void TearDown() {
        ff_free(filter);
	}

	void addr_to_str(ff_net_t* addr, char *addrstr)
	{
		if(addr->ver == 4 && inet_ntop(AF_INET, &addr->ip.data[3], addrstr, addrstr_len)) {
			inet_ntop(AF_INET, &addr->mask.data[3], maskstr, addrstr_len);
		} else if(inet_ntop(AF_INET6, &addr->ip.data[0], addrstr, addrstr_len)) {
			inet_ntop(AF_INET6, &addr->mask.data[0], maskstr, addrstr_len);
		} else { FAIL() << "Address->string conversion failed";}
	}


	void convert_addr(char* valstr, char* expect_addr, char* expect_mask)
	{
		ASSERT_FALSE(str_to_addr(filter, valstr, &ptr, &size)) << "Conversion failed for \"" << valstr << "\"";
		EXPECT_EQ((sizeof(ff_net_t)), size);
		addr_to_str(((ff_net_t*)ptr), &addrstr[0]);
		ASSERT_STREQ(expect_addr, addrstr) << "Unexpected result of ip conversion";
		ASSERT_STREQ(expect_mask, maskstr) << "Unexpected result of mask conversion";
		free(ptr);

		return;
	}

	void not_convert_addr(char* valstr) {
		int x;
		EXPECT_TRUE(x = str_to_addr(filter, valstr, &ptr, &size)) << "Conversion should not succeed for " << valstr;
		if(!x) free(ptr);
	}

    ff_options_t *test_callbacks;
    ff_t *filter;

	static const int addrstr_len = 40;
	char *ptr;
	size_t size;
	char addrstr[addrstr_len];
	char maskstr[addrstr_len];
};

class str_to_mac_test : public :: testing::Test {
protected:
	virtual void SetUp() {
        filter = NULL;

        ff_options_init(&test_callbacks); //Prepare structure for callbacks

        test_callbacks->ff_data_func = NULL;
        test_callbacks->ff_lookup_func = NULL;
        test_callbacks->ff_rval_map_func = NULL;

        ff_init(&filter, "any", test_callbacks);
        ff_options_free(test_callbacks);

		ptr = NULL;

		size = 0;
		addrstr[0] = 0;
	}

	virtual void TearDown() {
        ff_free(filter);
	}

	void convert_mac(char* mac, char* expect_mac)
	{
		return;
	}

    ff_options_t *test_callbacks;
    ff_t *filter;

	char *ptr;
	size_t size;
	char addrstr[40];
};


/**
 * Check conversion function
 */
TEST_F(str_to_number_test, unsigned_int_valid) {

	convert_uint(0, const_cast<char*>("0"), FF_TYPE_UINT64);
	convert_uint(1000, const_cast<char*>("1k"), FF_TYPE_UINT64);
	convert_uint(1000000, const_cast<char*>("1 M"), FF_TYPE_UINT64);
	convert_uint(UINT16_MAX, const_cast<char*>("65535"), FF_TYPE_UINT64);
	convert_uint(UINT32_MAX, const_cast<char*>("4294967295"), FF_TYPE_UINT64);
	convert_uint(1234567890000000000ULL, const_cast<char*>("1234567890 G"), FF_TYPE_UINT64);
	convert_uint(UINT64_MAX, const_cast<char*>("18446744073709551615"), FF_TYPE_UINT64);
	convert_uint(0xff, const_cast<char*>("0xff"), FF_TYPE_UINT64);
	convert_uint(0x3f, const_cast<char*>("077"), FF_TYPE_UINT64);

}

TEST_F(str_to_addr_test, ipv4_valid)
{
	/*           Tested string - Expected ip - Expected mask  */
	convert_addr(const_cast<char*>("192.168.0.25/4"), const_cast<char*>("192.0.0.0"), const_cast<char*>("240.0.0.0"));
	convert_addr(const_cast<char*>("192.168/10"), const_cast<char*>("192.128.0.0"), const_cast<char*>("255.192.0.0"));
	convert_addr(const_cast<char*>("255/4"), const_cast<char*>("240.0.0.0"), const_cast<char*>("240.0.0.0"));
}

TEST_F(str_to_addr_test, ipv4_invalid) {

	not_convert_addr(const_cast<char*>("22.0 .10"));
	not_convert_addr(const_cast<char*>("192.168.0.0/33"));
	not_convert_addr(const_cast<char*>("929-323-098"));
	not_convert_addr(const_cast<char*>("192.168. 0.0"));
	not_convert_addr(const_cast<char*>("192.168 .0.0"));
}

TEST_F(str_to_addr_test, ipv6_valid) {
	convert_addr(const_cast<char*>("2001:608::/15"), const_cast<char*>("2000::"), const_cast<char*>("fffe::"));

}
TEST_F(str_to_addr_test, invalid_numeric_mask){

	not_convert_addr(const_cast<char*>("192.168.0.0/-1"));
	not_convert_addr(const_cast<char*>("192.168.0.0/33"));
	not_convert_addr(const_cast<char*>("::127/-1"));
	not_convert_addr(const_cast<char*>("::127/129"));
}

/**
 * Conversion should fail given bad input
 */
TEST_F(str_to_number_test, unsigned_invalid_number_conversion) {

	char numbers[][30] = {
		"-+1",
		"1kT",		//Fail if two units
		"10f M",	//Fail if has sign in number
		"1e10",		//Fail if scientific number
		"deadbeef",	//Fail if hexa is without prefix
		"-922337203685477580 M8 ", //Fail if unit is not last sign in number
		""
	};

	for (int x = 0; numbers[x][0] ; x++) {
		//Conversion should fail
		EXPECT_TRUE(str_to_uint(filter, numbers[x], FF_TYPE_UINT64, &ptr, &size )) <<
			"Conversion to number " << x <<  ". \"" << numbers[x] << "\" should fail";
	}

}

/**
 * Large numbers are trimmed to max range if string contains too bg number
 *
 * NOT relevant any more, see unsigned_range tests
 */
/*TEST_F(str_to_number_test, unsigned_large_number_trimm) {

	char number[] = "18446744073709551615";

	EXPECT_EQ(0, str_to_uint(filter, number, FF_TYPE_UINT32, &ptr, &size )) <<
		"Failed to convert test number" <<"\"" << number << "\"";

	EXPECT_EQ(size, sizeof(uint32_t));

	EXPECT_EQ(UINT32_MAX, *((uint32_t *)ptr));

}*/

/**
 * Max range check for signed integers
 */
 //
TEST_F(str_to_number_test, signed_valid_max_range){

	//Check max range numbers
	ASSERT_EQ(0, str_to_int(filter, const_cast<char*>("-9223372036854775808"), FF_TYPE_INT64, &ptr, &size));
	EXPECT_EQ((sizeof(int64_t)), size);
	ASSERT_EQ((INT64_MIN), *((uint64_t *)ptr));

	ASSERT_EQ(0, str_to_int(filter, const_cast<char*>("9223372036854775807"), FF_TYPE_INT64, &ptr, &size));
	EXPECT_EQ((sizeof(int64_t)), size);
	ASSERT_EQ((INT64_MAX), *((uint64_t *)ptr));

}

/**
 * Range check on out of range numbers, rasults are to be trimmed
 */
TEST_F(str_to_number_test, int_invalid_range) {

	//Try convert over max uint64 range numbers
	ASSERT_NE(0, str_to_int(filter, const_cast<char*>("-9223372036854775809"), FF_TYPE_INT64, &ptr, &size));
	ASSERT_NE(0, str_to_int(filter, const_cast<char*>("9223372036854775808"), FF_TYPE_INT64, &ptr, &size));
	//Max range exceeded error
}


/**
 * Check address conversion on regular ipv6 without mask
 */
TEST_F(str_to_addr_test, addrV6_full_valid) {

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("2001:608::0"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);
	//Address to be marked as ipv6

	inet_ntop(AF_INET6, &(((ff_net_t*)ptr)->ip.data[0]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("2001:608::"), addrstr);
	free(ptr);

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("2001:608:0:f:f:f:f:1"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);

	inet_ntop(AF_INET6, &(((ff_net_t*)ptr)->ip.data[0]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("2001:608:0:f:f:f:f:1"), addrstr);
	free(ptr);
}

/**
 * Mask generation mechanism int_to_netmask
 */
TEST_F(str_to_addr_test, addrV4_full_valid_numeric_mask) {

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("192.168.0.1/10"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);

	inet_ntop(AF_INET, &(((ff_net_t*)ptr)->ip.data[3]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("192.128.0.0"), addrstr);
	free(ptr);

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("255.255.255.255/17"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);

	inet_ntop(AF_INET, &(((ff_net_t*)ptr)->ip.data[3]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("255.255.128.0"), addrstr);
	free(ptr);

}

/**
 * Ip autocompletion test
 */
TEST_F(str_to_addr_test, addrV4_short_valid_numeric_mask) {

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("192.168/10"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);

	inet_ntop(AF_INET, &(((ff_net_t*)ptr)->ip.data[3]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("192.128.0.0"), addrstr);
	free(ptr);

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("255.255.255/17"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);

	inet_ntop(AF_INET, &(((ff_net_t*)ptr)->ip.data[3]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("255.255.128.0"), addrstr);

	free(ptr);
}

/**
 * Mask ip with full mask conversion test
 */
TEST_F(str_to_addr_test, addrV4_valid_mask) {

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("192.168.0.0 255.0.0.0"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);
	inet_ntop(AF_INET, &(((ff_net_t*)ptr)->ip.data[3]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("192.0.0.0"), addrstr);
	free(ptr);

	ASSERT_EQ(0, str_to_addr(filter, const_cast<char*>("255.255.0.0 255.128.0.0"), &ptr, &size));
	EXPECT_EQ((sizeof(ff_net_t)), size);

	inet_ntop(AF_INET, &(((ff_net_t*)ptr)->ip.data[3]), addrstr, 40);

	ASSERT_STREQ(const_cast<char*>("255.128.0.0"), addrstr);
}

/**
 * Conversion of valid mac
 */
TEST_F(str_to_mac_test, valid_mac) {

	char mac[] = "02:ff:de:ad:be:ef";

	ASSERT_EQ(0, str_to_mac(filter, mac, &ptr, &size));
	EXPECT_EQ(sizeof(ff_mac_t), size);
	snprintf(addrstr, 40,"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

	ASSERT_STRCASEEQ(mac, addrstr);
}

/**
 * Conversion of invalid mac
 */
TEST_F(str_to_mac_test, invalid_mac) {

	char mac[] = "02:fff:de:ad:be:ef";

	EXPECT_EQ(1, str_to_mac(filter, mac, &ptr, &size));

	char mac2[] = "02:ff:de:ad:be:ef:ed";

	EXPECT_EQ(1, str_to_mac(filter, mac2, &ptr, &size));
}

TEST_F(str_to_number_test, real_number) {

	char number[] = "-10.0e-3";

	ASSERT_EQ(0, str_to_real(filter, number, &ptr, &size));
	ASSERT_EQ(-10.0e-3,  *((double *)ptr));
}

TEST_F(str_to_number_test, invalid_real) {
	int x;
	x = str_to_real(filter, const_cast<char*>("092238.39ffs.e"), &ptr, &size);
	EXPECT_TRUE(x);
	if (!x) free(ptr);
}

TEST_F(str_to_number_test, signed_ranges) {
    convert_int(INT8_MIN, "-128", FF_TYPE_INT8);
    convert_int(INT8_MAX, "127", FF_TYPE_INT8);
    convert_int(0, "0", FF_TYPE_INT8);

    fail_convert_int("-129", FF_TYPE_INT8);
    fail_convert_int("128", FF_TYPE_INT8);

    convert_int(INT16_MIN, "-32768", FF_TYPE_INT16);
    convert_int(INT16_MAX, "32767", FF_TYPE_INT16);
    convert_int(0, "0", FF_TYPE_INT16);

    fail_convert_int("-32769", FF_TYPE_INT16);
    fail_convert_int("32768", FF_TYPE_INT16);

    convert_int(INT32_MIN, "-2147483648", FF_TYPE_INT32);
    convert_int(INT32_MAX, "2147483647", FF_TYPE_INT32);
    convert_int(0, "0", FF_TYPE_INT32);

    fail_convert_int("-2147483649", FF_TYPE_INT32);
    fail_convert_int("2147483648", FF_TYPE_INT32);

    convert_int(INT64_MIN, "-9223372036854775808", FF_TYPE_INT64);
    convert_int(INT64_MAX, "9223372036854775807", FF_TYPE_INT64);
    convert_int(0, "0", FF_TYPE_INT64);

    fail_convert_int("-9223372036854775809", FF_TYPE_INT64);
    fail_convert_int("9223372036854775808", FF_TYPE_INT64);
}

TEST_F(str_to_number_test, unsigned_ranges) {

    convert_uint(UINT8_MAX, "255", FF_TYPE_UINT8);
    fail_convert_uint("-1", FF_TYPE_UINT8);
    fail_convert_uint("256", FF_TYPE_UINT8);

    convert_uint(UINT16_MAX, "65535", FF_TYPE_UINT16);
    fail_convert_uint("-1", FF_TYPE_UINT16);
    fail_convert_uint("-65336", FF_TYPE_UINT16);
    fail_convert_uint("65536", FF_TYPE_UINT16);

    convert_uint(UINT32_MAX, "4294967295", FF_TYPE_UINT32);
    fail_convert_uint("-1", FF_TYPE_UINT32);
    fail_convert_uint("4294967296", FF_TYPE_UINT32);

    convert_uint(UINT64_MAX, "18446744073709551615", FF_TYPE_UINT64);
    fail_convert_uint("-1", FF_TYPE_UINT64);
    fail_convert_uint("-9223372036854775809", FF_TYPE_UINT64);
    fail_convert_uint("18446744073709551616", FF_TYPE_UINT64);
}

TEST_F(str_to_number_test, timestamps)
{
    ASSERT_FALSE(str_to_timestamp(filter, "2017-9-1 12:24:44", &ptr, &size));
    EXPECT_EQ(1504265084000, *((ff_timestamp_t*)ptr));

    ASSERT_FALSE(str_to_timestamp(filter, "2017-9-01  12:25:44", &ptr, &size));
    //60000 more
    EXPECT_EQ(1504265144000, *((ff_timestamp_t*)ptr));


    ASSERT_FALSE(str_to_timestamp(filter, "1970-1-1 1:0:0", &ptr, &size));
    EXPECT_EQ(0, *((ff_timestamp_t*)ptr));

    // Will cause underflow which is bad but cant do anything about it
    ASSERT_FALSE(str_to_timestamp(filter, "1970-1-1 0:0:0", &ptr, &size));
    EXPECT_EQ((uint64_t)(-3600000), *((ff_timestamp_t*)ptr));

    ASSERT_TRUE(str_to_timestamp(filter, "2017-9-01  12:25:144", &ptr, &size));
    ASSERT_TRUE(str_to_timestamp(filter, "12:25:144 random text", &ptr, &size));
}
