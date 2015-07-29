#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../src/rlite.h"
#include "../src/page_multi_string.h"
#include "../src/util.h"
#include "test_util.h"

int basic_set_get(int UNUSED(_))
{
	int retval, i, j;
	long size, size2, number;
	unsigned char *data, *data2;

	rlite *db = NULL;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);

	for (i = 0; i < 2; i++) {
		srand(1);
		size = i == 0 ? 20 : 1020;
		fprintf(stderr, "Start page_multi_string-test %ld\n", size);
		data = malloc(sizeof(unsigned char) * size);
		for (j = 0; j < size; j++) {
			data[j] = (unsigned char)(rand() / CHAR_MAX);
		}
		RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &number, data, size);
		RL_CALL_VERBOSE(rl_multi_string_get, RL_OK, db, number, &data2, &size2);
		EXPECT_BYTES(data, size, data2, size2);
		rl_free(data);
		rl_free(data2);
		fprintf(stderr, "End page_multi_string-test %ld\n", size);
	}

	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int empty_set_get(int UNUSED(_))
{
	int retval;
	long size, number;
	unsigned char *data;

	rlite *db = NULL;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);

	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &number, NULL, 0);
	RL_CALL_VERBOSE(rl_multi_string_get, RL_OK, db, number, &data, &size);
	EXPECT_INT(size, 0);
	EXPECT_PTR(data, NULL);
	fprintf(stderr, "End page_multi_string-test %ld\n", size);

	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

static int test_sha(long size)
{
	fprintf(stderr, "Start test_sha %ld\n", size);
	unsigned char *data = malloc(sizeof(unsigned char) * size);
	rlite *db = NULL;
	int retval;
	unsigned char digest1[20], digest2[20];
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);

	long page, i;
	for (i = 0; i < size; i++) {
		data[i] = i % 123;
	}

	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &page, data, size);
	RL_CALL_VERBOSE(rl_multi_string_sha1, RL_OK, db, digest1, page);
	RL_CALL_VERBOSE(sha1, RL_OK, data, size, digest2);
	EXPECT_BYTES(digest1, 20, digest2, 20);

	fprintf(stderr, "End test_sha %ld\n", size);
	retval = RL_OK;
cleanup:
	rl_free(data);
	rl_close(db);
	return retval;
}

static int assert_cmp(rlite *db, long p1, unsigned char *data, long size, int expected_cmp)
{
	long p2;
	int cmp;
	int retval;
	RL_CALL_VERBOSE(rl_multi_string_cmp_str, RL_OK, db, p1, data, size, &cmp);
	EXPECT_INT(cmp, expected_cmp);
	if (data) {
		RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &p2, data, size);
		RL_CALL_VERBOSE(rl_multi_string_cmp, RL_OK, db, p1, p2, &cmp);
		EXPECT_INT(cmp, expected_cmp);
	}
	retval = 0;
cleanup:
	return retval;
}
static int test_cmp_different_length(int UNUSED(_))
{
	unsigned char data[3], data2[3];
	long size = 2, i;
	long p1;
	rlite *db = NULL;
	fprintf(stderr, "Start test_cmp_different_length\n");
	int retval;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);
	for (i = 0; i < 3; i++) {
		data[i] = data2[i] = 100 + i;
	}
	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &p1, data, size);
	RL_CALL_VERBOSE(assert_cmp, 0, db, p1, data2, 0, 1);
	RL_CALL_VERBOSE(assert_cmp, 0, db, p1, data2, size, 0);
	RL_CALL_VERBOSE(assert_cmp, 0, db, p1, data2, size - 1, 1);
	data2[0]++;
	RL_CALL_VERBOSE(assert_cmp, 0, db, p1, data2, size - 1, -1);
	RL_CALL_VERBOSE(assert_cmp, 0, db, p1, data2, size, -1);
	RL_CALL_VERBOSE(assert_cmp, 0, db, p1, NULL, 0, 1);

	rl_close(db);
	fprintf(stderr, "End test_cmp_different_length\n");
	retval = RL_OK;
cleanup:
	return retval;
}

#define CMP_SIZE 2000
static int test_cmp_internal(int expected_cmp, long position)
{
	unsigned char data[CMP_SIZE], data2[CMP_SIZE];
	long size = CMP_SIZE, i;
	int cmp;
	long p1, p2;
	rlite *db = NULL;
	if (position % 500 == 0) {
	}
	int retval;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);
	for (i = 0; i < CMP_SIZE; i++) {
		data[i] = data2[i] = i % CHAR_MAX;
	}
	if (expected_cmp != 0) {
		if (data[position] == CHAR_MAX && expected_cmp > 0) {
			data2[position]--;
		} else if (data[position] == 0 && expected_cmp < 0) {
			data2[position]++;
		} else if (expected_cmp > 0) {
			data[position]++;
		} else {
			data[position]--;
		}
	}
	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &p1, data, size);
	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &p2, data2, size);
	RL_CALL_VERBOSE(rl_multi_string_cmp, RL_OK, db, p1, p2, &cmp);
	EXPECT_INT(cmp, expected_cmp);
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

static int test_cmp(int expected_cmp, long position_from, long position_to)
{
	int retval = RL_OK;
	long i;
	fprintf(stderr, "Start page_multi_string-test test_cmp %d %ld %ld\n", expected_cmp, position_from, position_to);
	for (i = position_from; i < position_to; i++) {
		RL_CALL(test_cmp_internal, 0, expected_cmp, i);
	}
	fprintf(stderr, "End page_multi_string-test basic_cmp\n");
cleanup:
	return retval;
}

static int test_cmp2_internal(int expected_cmp, long position)
{
	unsigned char data[CMP_SIZE], data2[CMP_SIZE];
	long size = CMP_SIZE, i;
	int cmp;
	long p1;
	rlite *db = NULL;
	int retval;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);
	for (i = 0; i < CMP_SIZE; i++) {
		data[i] = data2[i] = i % CHAR_MAX;
	}
	if (expected_cmp != 0) {
		if (data[position] == CHAR_MAX && expected_cmp > 0) {
			data2[position]--;
		} else if (data[position] == 0 && expected_cmp < 0) {
			data2[position]++;
		} else if (expected_cmp > 0) {
			data[position]++;
		} else {
			data[position]--;
		}
	}
	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &p1, data, size);
	RL_CALL_VERBOSE(rl_multi_string_cmp_str, RL_OK, db, p1, data2, size, &cmp);
	EXPECT_INT(cmp, expected_cmp);
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

static int test_cmp2(int expected_cmp, long position_from, long position_to)
{
	int retval = RL_OK;
	long i;
	fprintf(stderr, "Start page_multi_string-test test_cmp2 %d %ld %ld\n", expected_cmp, position_from, position_to);
	for (i = position_from; i < position_to; i++) {
		RL_CALL(test_cmp2_internal, 0, expected_cmp, i);
	}
	fprintf(stderr, "End page_multi_string-test basic_cmp2\n");
cleanup:
	return retval;
}

static int test_append(long size, long append_size)
{
	fprintf(stderr, "Start test_append %ld %ld\n", size, append_size);
	unsigned char *data = malloc(sizeof(unsigned char) * size);
	unsigned char *append_data = malloc(sizeof(unsigned char) * append_size);
	unsigned char *testdata;
	long testdatalen;
	rlite *db = NULL;
	int retval;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);

	long page, i;
	for (i = 0; i < size; i++) {
		data[i] = i % 123;
	}
	for (i = 0; i < append_size; i++) {
		append_data[i] = i % 123;
	}

	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &page, data, size);
	RL_CALL_VERBOSE(rl_multi_string_append, RL_OK, db, page, append_data, append_size, &testdatalen);
	EXPECT_LONG(testdatalen, size + append_size);

	RL_CALL_VERBOSE(rl_multi_string_get, RL_OK, db, page, &testdata, &testdatalen);
	EXPECT_LONG(testdatalen, size + append_size);
	EXPECT_BYTES(testdata, size, data, size);
	EXPECT_BYTES(&testdata[size], append_size, append_data, append_size);
	rl_free(testdata);

	fprintf(stderr, "End test_append %ld\n", size);
	retval = 0;
cleanup:
	rl_free(data);
	rl_free(append_data);
	rl_close(db);
	return retval;
}

static int test_substr(long strsize, long start, long stop, long startindex, long expectedsize)
{
	fprintf(stderr, "Start test_substr %ld %ld %ld %ld %ld\n", strsize, start, stop, startindex, expectedsize);
	unsigned char *data = malloc(sizeof(unsigned char) * strsize), *data2;
	rlite *db = NULL;
	int retval;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);

	long page, i, size2;
	for (i = 0; i < strsize; i++) {
		data[i] = i % 123;
	}

	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &page, data, strsize);
	RL_CALL_VERBOSE(rl_multi_string_getrange, RL_OK, db, page, &data2, &size2, start, stop);
	EXPECT_BYTES(&data[startindex], size2, data2, expectedsize);
	rl_free(data2);
	fprintf(stderr, "End test_substr\n");
	retval = 0;
cleanup:
	free(data);
	rl_close(db);
	return retval;
}

static int test_setrange(long initialsize, long index, long updatesize)
{
	fprintf(stderr, "Start test_setrange %ld %ld %ld\n", initialsize, index, updatesize);
	long finalsize = index + updatesize > initialsize ? index + updatesize : initialsize;
	long newlength, testdatalen;
	unsigned char *testdata;
	unsigned char *finaldata = calloc(finalsize, sizeof(unsigned char));
	unsigned char *initialdata = malloc(sizeof(unsigned char) * initialsize);
	unsigned char *updatedata = malloc(sizeof(unsigned char) * updatesize);
	rlite *db = NULL;
	int retval;
	RL_CALL_VERBOSE(rl_open, RL_OK, ":memory:", &db, RLITE_OPEN_READWRITE | RLITE_OPEN_CREATE);

	long page, i;
	for (i = 0; i < initialsize; i++) {
		finaldata[i] = initialdata[i] = i % 123;
	}
	for (i = 0; i < updatesize; i++) {
		finaldata[index + i] = updatedata[i] = i % 151;
	}

	RL_CALL_VERBOSE(rl_multi_string_set, RL_OK, db, &page, initialdata, initialsize);
	RL_CALL_VERBOSE(rl_multi_string_setrange, RL_OK, db, page, updatedata, updatesize, index, &newlength);
	EXPECT_LONG(finalsize, newlength);
	RL_CALL_VERBOSE(rl_multi_string_get, RL_OK, db, page, &testdata, &testdatalen);
	EXPECT_BYTES(finaldata, finalsize, testdata, testdatalen);
	rl_free(testdata);

	fprintf(stderr, "End test_setrange\n");
	retval = 0;
cleanup:
	free(finaldata);
	free(initialdata);
	free(updatedata);
	rl_close(db);
	return retval;
}

RL_TEST_MAIN_START(multi_string_test)
{
	RL_TEST(basic_set_get, 0);
	RL_TEST(empty_set_get, 0);
	RL_TEST(test_cmp_different_length, 0);
	RL_TEST(test_cmp, 0, 0, 1);
	RL_TEST(test_sha, 100);
	RL_TEST(test_sha, 1000);
	RL_TEST(test_append, 10, 20);
	RL_TEST(test_append, 10, 1200);
	RL_TEST(test_append, 1200, 10);
	RL_TEST(test_append, 1000, 2000);
	long i, j;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < CMP_SIZE / 500; j++) {
			RL_TEST(test_cmp, i * 2 - 1, j * 500, (j + 1) * 500);
			RL_TEST(test_cmp2, i * 2 - 1, j * 500, (j + 1) * 500);
		}
	}
	RL_TEST(test_substr, 20, 0, 10, 0, 11);
	RL_TEST(test_substr, 2000, 1, -1, 1, 1999);
	RL_TEST(test_substr, 2000, -10, -1, 1990, 10);
	RL_TEST(test_setrange, 10, 5, 3);
	RL_TEST(test_setrange, 10, 5, 20);
	RL_TEST(test_setrange, 10, 20, 5);
	RL_TEST(test_setrange, 1024, 1024, 1024);
	RL_TEST(test_setrange, 1024, 100, 1024);
	RL_TEST(test_setrange, 1024, 1024, 100);
}
RL_TEST_MAIN_END
