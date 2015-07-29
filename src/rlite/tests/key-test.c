#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "test_util.h"
#include "../src/page_key.h"
#include "../src/rlite.h"
#include "../src/type_zset.h"

static int expect_key(rlite *db, unsigned char *key, long keylen, char type, long page)
{
	unsigned char type2;
	long page2;
	int retval;
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type2, NULL, &page2, NULL, NULL);
	EXPECT_INT(type, type2);
	EXPECT_LONG(page, page2);
	retval = 0;
cleanup:
	return retval;
}

int basic_test_set_get(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_set_get %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 23;
	RL_CALL_VERBOSE(rl_key_set, RL_OK, db, key, keylen, type, page, 0, 0);
	RL_COMMIT();
	RL_CALL_VERBOSE(expect_key, RL_OK, db, key, keylen, type, page);
	fprintf(stderr, "End basic_test_set_get\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_get_unexisting(int UNUSED(_))
{
	int retval;
	fprintf(stderr, "Start basic_test_get_unexisting\n");

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, 0, 1);

	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	RL_CALL_VERBOSE(rl_key_get, RL_NOT_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);
	fprintf(stderr, "End basic_test_get_unexisting\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_set_delete(int UNUSED(_))
{
	int retval;
	fprintf(stderr, "Start basic_test_set_delete\n");

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, 0, 1);

	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 23;
	RL_CALL_VERBOSE(rl_key_set, RL_OK, db, key, keylen, type, page, 0, 0);
	RL_CALL_VERBOSE(rl_key_delete, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_key_get, RL_NOT_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);

	fprintf(stderr, "End basic_test_set_delete\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_get_or_create(int _commit)
{
	int retval;
	fprintf(stderr, "Start basic_test_get_or_create %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 100, page2 = 200; // set dummy values for != assert
	long version;
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_NOT_FOUND, db, key, keylen, type, &page, &version);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_key_get_or_create, RL_FOUND, db, key, keylen, type, &page2, &version);
	RL_COMMIT();
	EXPECT_LONG(page, page2);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_WRONG_TYPE, db, key, keylen, type + 1, &page2, &version);

	fprintf(stderr, "End basic_test_get_or_create\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_multidb(int _commit)
{
	int retval;
	fprintf(stderr, "Start basic_test_multidb %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 100, page2 = 200, pagetest; // set dummy values for != assert

	RL_CALL_VERBOSE(rl_key_get_or_create, RL_NOT_FOUND, db, key, keylen, type, &page, NULL);
	RL_COMMIT();
	RL_CALL_VERBOSE(rl_select, RL_OK, db, 1);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_NOT_FOUND, db, key, keylen, type, &page2, NULL);
	RL_COMMIT();
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_FOUND, db, key, keylen, type, &pagetest, NULL);
	EXPECT_LONG(pagetest, page2);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_FOUND, db, key, keylen, type, &pagetest, NULL);
	if (pagetest == page) {
		fprintf(stderr, "Expected pagetest %ld to mismatch page %ld\n", pagetest, page);
		retval = RL_UNEXPECTED;
		goto cleanup;
	}

	RL_CALL_VERBOSE(rl_select, RL_OK, db, 0);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_FOUND, db, key, keylen, type, &pagetest, NULL);
	EXPECT_LONG(pagetest, page);

	fprintf(stderr, "End basic_test_multidb\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_move(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_move %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 100, pagetest; // set dummy values for != assert

	RL_CALL_VERBOSE(rl_key_get_or_create, RL_NOT_FOUND, db, key, keylen, type, &page, NULL);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_move, RL_OK, db, key, keylen, 1);
	RL_CALL_VERBOSE(rl_key_get, RL_NOT_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);
	RL_CALL_VERBOSE(rl_select, RL_OK, db, 1);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_FOUND, db, key, keylen, type, &pagetest, NULL);
	EXPECT_LONG(pagetest, page);
	fprintf(stderr, "End basic_test_move\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int existing_test_move(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start existing_test_move %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 100, page2 = 101, pagetest; // set dummy values for != assert

	RL_CALL_VERBOSE(rl_select, RL_OK, db, 1);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_NOT_FOUND, db, key, keylen, type, &page, NULL);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_select, RL_OK, db, 0);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_NOT_FOUND, db, key, keylen, type, &page2, NULL);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_move, RL_FOUND, db, key, keylen, 1);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, NULL, NULL, &pagetest, NULL, NULL);
	EXPECT_LONG(pagetest, page2)

	RL_CALL_VERBOSE(rl_select, RL_OK, db, 1);
	RL_CALL_VERBOSE(rl_key_get_or_create, RL_FOUND, db, key, keylen, type, &pagetest, NULL);
	EXPECT_LONG(pagetest, page);
	fprintf(stderr, "End existing_test_move\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_expires(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_expires %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 100;

	RL_CALL_VERBOSE(rl_key_set, RL_OK, db, key, keylen, type, page, rl_mstime() - 1, 0);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_key_get, RL_NOT_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);
	RL_CALL_VERBOSE(rl_key_set, RL_OK, db, key, keylen, type, page, 10000 + rl_mstime(), 0);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);

	fprintf(stderr, "End basic_test_expires\n");
	retval = 0;
cleanup:
	rl_close(db);
	return retval;
}

int basic_test_change_expiration(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start basic_test_change_expiration %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char type = 'A';
	long page = 100;
	unsigned long long expiration = rl_mstime() + 1000, expirationtest;

	RL_CALL_VERBOSE(rl_key_set, RL_OK, db, key, keylen, type, page, expiration, 0);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, NULL, NULL, NULL, &expirationtest, NULL);
	EXPECT_LLU(expirationtest, expiration);
	RL_CALL_VERBOSE(rl_key_expires, RL_OK, db, key, keylen, expiration + 1000);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);
	RL_CALL_VERBOSE(rl_key_expires, RL_OK, db, key, keylen, expiration - 10000);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_key_get, RL_NOT_FOUND, db, key, keylen, NULL, NULL, NULL, NULL, NULL);

	fprintf(stderr, "End basic_test_change_expiration\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

int test_delete_with_value(int _commit)
{
	int retval;
	fprintf(stderr, "Start test_delete_with_value %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, 100, (unsigned char *)"asd", 3);
	RL_COMMIT();
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_is_balanced, RL_OK, db);

	fprintf(stderr, "End test_delete_with_value\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

static int test_rename_ok(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_rename_ok %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, 100, (unsigned char *)"asd", 3);
	RL_COMMIT();
	RL_CALL_VERBOSE(rl_rename, RL_OK, db, key, keylen, key2, key2len, 0);
	RL_CALL_VERBOSE(rl_is_balanced, RL_OK, db);

	fprintf(stderr, "End test_rename_ok\n");
	retval = 0;
cleanup:
	rl_close(db);
	return retval;
}

static int test_rename_overwrite(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_rename_overwrite %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	double score = 100, score2 = 200, scoretest;

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, score, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key2, key2len, score2, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_rename, RL_OK, db, key, keylen, key2, key2len, 1);
	RL_COMMIT();
	RL_CALL_VERBOSE(rl_zscore, RL_FOUND, db, key2, key2len, data, datalen, &scoretest);
	EXPECT_DOUBLE(scoretest, score);

	fprintf(stderr, "End test_rename_overwrite\n");
	retval = 0;
cleanup:
	rl_close(db);
	return retval;
}

static int test_rename_no_overwrite(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_rename_no_overwrite %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	double score = 100, score2 = 200, scoretest;

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, score, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key2, key2len, score2, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_rename, RL_FOUND, db, key, keylen, key2, key2len, 0);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_zscore, RL_FOUND, db, key2, key2len, data, datalen, &scoretest);
	EXPECT_DOUBLE(scoretest, score2);

	fprintf(stderr, "End test_rename_no_overwrite\n");
	retval = 0;
cleanup:
	rl_close(db);
	return retval;
}

static int test_dbsize(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_dbsize %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	double score = 100, score2 = 200;
	long size;

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, score, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_dbsize, RL_OK, db, &size);

	EXPECT_LONG(size, 1);

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key2, key2len, score2, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_dbsize, RL_OK, db, &size);
	EXPECT_LONG(size, 2);

	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_dbsize, RL_OK, db, &size);
	EXPECT_LONG(size, 1);

	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key2, key2len);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_dbsize, RL_OK, db, &size);
	EXPECT_LONG(size, 0);

	RL_CALL_VERBOSE(rl_is_balanced, RL_OK, db);

	fprintf(stderr, "End test_dbsize\n");
	retval = RL_OK;
cleanup:
	rl_close(db);
	return retval;
}

static int test_keys(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_keys %d\n", _commit);

	rlite *db;
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	double score = 100, score2 = 200;
	long len = 0, *keyslen = NULL;
	unsigned char **keys = NULL;
	long i;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);

#define FREE_KEYS()\
	for (i = 0; i < len; i++) {\
		rl_free(keys[i]);\
	}\
	rl_free(keyslen);\
	rl_free(keys);\
	keys = NULL;\
	keyslen = NULL;\
	len = 0;

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, score, data, datalen);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_keys, RL_OK, db, (unsigned char*)"", 0, &len, &keys, &keyslen);
	EXPECT_LONG(len, 0);

	FREE_KEYS();

	RL_CALL_VERBOSE(rl_keys, RL_OK, db, (unsigned char*)"*", 1, &len, &keys, &keyslen);
	EXPECT_LONG(len, 1);

	EXPECT_BYTES(keys[0], keyslen[0], key, keylen);

	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key2, key2len, score2, data, datalen);
	RL_COMMIT();

	FREE_KEYS();
	RL_CALL_VERBOSE(rl_keys, RL_OK, db, (unsigned char*)"*", 1, &len, &keys, &keyslen);
	EXPECT_LONG(len, 2);

	if ((keyslen[0] == keylen && memcmp(keys[0], key, keylen) != 0) || (keyslen[1] == keylen && memcmp(keys[1], key, keylen) != 0)) {
		fprintf(stderr, "Expected keys to contain key on line %d\n", __LINE__);
		retval = RL_UNEXPECTED;
		goto cleanup;
	}

	if ((keyslen[0] == key2len && memcmp(keys[0], key2, key2len) != 0) || (keyslen[1] == key2len && memcmp(keys[1], key2, key2len) != 0)) {
		fprintf(stderr, "Expected keys to contain key2 on line %d\n", __LINE__);
		retval = RL_UNEXPECTED;
		goto cleanup;
	}

	FREE_KEYS();
	RL_CALL_VERBOSE(rl_keys, RL_OK, db, (unsigned char*)"*2", 2, &len, &keys, &keyslen);

	EXPECT_LONG(len, 1);
	EXPECT_BYTES(keys[0], keyslen[0], key2, key2len);
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_COMMIT();

	FREE_KEYS();
	RL_CALL_VERBOSE(rl_keys, RL_OK, db, (unsigned char*)"*", 1, &len, &keys, &keyslen);

	EXPECT_LONG(len, 1);
	EXPECT_BYTES(keys[0], keyslen[0], key2, key2len);

	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key2, key2len);
	RL_COMMIT();

	FREE_KEYS();
	RL_CALL_VERBOSE(rl_keys, RL_OK, db, (unsigned char*)"*", 1, &len, &keys, &keyslen);
	EXPECT_LONG(len, 0);
	RL_CALL_VERBOSE(rl_is_balanced, RL_OK, db);

	fprintf(stderr, "End test_keys\n");
	retval = 0;
cleanup:
	FREE_KEYS();
	rl_close(db);
	return retval;
}

static int test_randomkey(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_randomkey %d\n", _commit);

	rlite *db;
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	unsigned char *testkey;
	long testkeylen, i;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);

	RL_CALL_VERBOSE(rl_set, RL_OK, db, key, keylen, data, datalen, 0, 0);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_randomkey, RL_OK, db, &testkey, &testkeylen);
	EXPECT_BYTES(testkey, testkeylen, key, keylen);
	rl_free(testkey);

	RL_CALL_VERBOSE(rl_set, RL_OK, db, key2, key2len, data, datalen, 0, 0);
	RL_COMMIT();

	for (i = 0; i < 20; i++) {
		RL_CALL_VERBOSE(rl_randomkey, RL_OK, db, &testkey, &testkeylen);

		if ((keylen == testkeylen && memcmp(key, testkey, testkeylen) != 0) || (key2len == testkeylen && memcmp(key2, testkey, testkeylen) != 0)) {
			fprintf(stderr, "Expected keys to contain key on line %d\n", __LINE__);
			retval = RL_UNEXPECTED;
			goto cleanup;
		}
		rl_free(testkey);
	}

	fprintf(stderr, "End test_randomkey\n");
	retval = 0;
cleanup:
	rl_close(db);
	return retval;
}

static int test_flushdb(int _commit)
{
	int retval = 0;
	fprintf(stderr, "Start test_flushdb %d\n", _commit);

	rlite *db;
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *key2 = (unsigned char *)"my key 2";
	long key2len = strlen((char *)key2);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	long len;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);

	RL_CALL_VERBOSE(rl_set, RL_OK, db, key, keylen, data, datalen, 0, 0);
	RL_CALL_VERBOSE(rl_set, RL_OK, db, key2, key2len, data, datalen, 0, 0);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_flushdb, RL_OK, db);
	RL_COMMIT();

	RL_CALL_VERBOSE(rl_dbsize, RL_OK, db, &len);
	EXPECT_LONG(len, 0);

	fprintf(stderr, "End test_flushdb\n");
	retval = 0;
cleanup:
	rl_close(db);
	return retval;
}

static int string_version_test(int _commit) {
	int retval = 0;
	fprintf(stderr, "Start string_version_test %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	unsigned char type = 'A';
	long page = 23, version = -1, version2 = -2, version3 = -3;
	RL_CALL_VERBOSE(rl_set, RL_OK, db, key, keylen, data, datalen, 0, 0);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version);
	RL_CALL_VERBOSE(rl_set, RL_OK, db, key, keylen, data, datalen, 0, 0);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version2);
	EXPECT_LONG(version + 1, version2);
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_set, RL_OK, db, key, keylen, data, datalen, 0, 0);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version3);
	if (version3 == version || version2 == version) {
		fprintf(stderr, "Expected version after delete not to match with previous ones\n");
		retval = 1;
		goto cleanup;
	}
	retval = 0;
	fprintf(stderr, "End string_version_test %d\n", _commit);
cleanup:
	rl_close(db);
	return retval;
}

static int list_version_test(int _commit) {
	int retval = 0;
	fprintf(stderr, "Start list_version_test %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	unsigned char type = 'A';
	long page = 23, version = -1, version2 = -2, version3 = -3;
	RL_CALL_VERBOSE(rl_push, RL_OK, db, key, keylen, 1, 1, 1, &data, &datalen, NULL);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version);
	RL_CALL_VERBOSE(rl_push, RL_OK, db, key, keylen, 1, 1, 1, &data, &datalen, NULL);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version2);
	EXPECT_LONG(version + 1, version2);
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_push, RL_OK, db, key, keylen, 1, 1, 1, &data, &datalen, NULL);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version3);
	if (version3 == version || version2 == version) {
		fprintf(stderr, "Expected version after delete not to match with previous ones\n");
		retval = 1;
		goto cleanup;
	}
	retval = 0;
	fprintf(stderr, "End list_version_test %d\n", _commit);
cleanup:
	rl_close(db);
	return retval;
}

static int set_version_test(int _commit) {
	int retval = 0;
	fprintf(stderr, "Start set_version_test %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	unsigned char *data2 = (unsigned char *)"asd2";
	long data2len = strlen((char *)data2);
	unsigned char type = 'A';
	long page = 23, version = -1, version2 = -2, version3 = -3;
	RL_CALL_VERBOSE(rl_sadd, RL_OK, db, key, keylen, 1, &data, &datalen, NULL);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version);
	RL_CALL_VERBOSE(rl_sadd, RL_OK, db, key, keylen, 1, &data2, &data2len, NULL);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version2);
	EXPECT_LONG(version + 1, version2);
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_sadd, RL_OK, db, key, keylen, 1, &data, &datalen, NULL);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version3);
	if (version3 == version || version2 == version) {
		fprintf(stderr, "Expected version after delete not to match with previous ones\n");
		retval = 1;
		goto cleanup;
	}
	retval = 0;
	fprintf(stderr, "End set_version_test %d\n", _commit);
cleanup:
	rl_close(db);
	return retval;
}

static int zset_version_test(int _commit) {
	int retval = 0;
	fprintf(stderr, "Start zset_version_test %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	unsigned char *data2 = (unsigned char *)"asd2";
	long data2len = strlen((char *)data2);
	unsigned char type = 'A';
	long page = 23, version = -1, version2 = -2, version3 = -3;
	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, 0.0, data, datalen);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version);
	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, 0.0, data2, data2len);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version2);
	EXPECT_LONG(version + 1, version2);
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_zadd, RL_OK, db, key, keylen, 0.0, data, datalen);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version3);
	if (version3 == version || version2 == version) {
		fprintf(stderr, "Expected version after delete not to match with previous ones\n");
		retval = 1;
		goto cleanup;
	}
	retval = 0;
	fprintf(stderr, "End zset_version_test %d\n", _commit);
cleanup:
	rl_close(db);
	return retval;
}

static int hash_version_test(int _commit) {
	int retval = 0;
	fprintf(stderr, "Start hash_version_test %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *field = (unsigned char *)"field";
	long fieldlen = strlen((char *)field);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);
	unsigned char *data2 = (unsigned char *)"asd2";
	long data2len = strlen((char *)data2);
	unsigned char type = 'A';
	long page = 23, version = -1, version2 = -2, version3 = -3;
	RL_CALL_VERBOSE(rl_hset, RL_OK, db, key, keylen, field, fieldlen, data, datalen, NULL, 1);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version);
	RL_CALL_VERBOSE(rl_hset, RL_OK, db, key, keylen, field, fieldlen, data2, data2len, NULL, 1);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version2);
	EXPECT_LONG(version + 1, version2);
	RL_CALL_VERBOSE(rl_key_delete_with_value, RL_OK, db, key, keylen);
	RL_CALL_VERBOSE(rl_hset, RL_OK, db, key, keylen, field, fieldlen, data, datalen, NULL, 1);
	RL_CALL_VERBOSE(rl_key_get, RL_FOUND, db, key, keylen, &type, NULL, &page, NULL, &version3);
	if (version3 == version || version2 == version) {
		fprintf(stderr, "Expected version after delete not to match with previous ones\n");
		retval = 1;
		goto cleanup;
	}
	retval = 0;
	fprintf(stderr, "End hash_version_test %d\n", _commit);
cleanup:
	rl_close(db);
	return retval;
}


static int watch_test(int _commit) {
	int retval = 0;
	fprintf(stderr, "Start watch_test %d\n", _commit);

	rlite *db;
	RL_CALL_VERBOSE(setup_db, RL_OK, &db, _commit, 1);
	unsigned char *key = (unsigned char *)"my key";
	long keylen = strlen((char *)key);
	unsigned char *data = (unsigned char *)"asd";
	long datalen = strlen((char *)data);

	struct watched_key *wkey;

	RL_CALL_VERBOSE(rl_watch, RL_OK, db, &wkey, key, keylen);
	RL_CALL_VERBOSE(rl_set, RL_OK, db, key, keylen, data, datalen, 0, 0);
	RL_CALL_VERBOSE(rl_check_watched_keys, RL_OUTDATED, db, 1, &wkey);
	rl_free(wkey);

	RL_CALL_VERBOSE(rl_watch, RL_OK, db, &wkey, key, keylen);
	RL_CALL_VERBOSE(rl_check_watched_keys, RL_OK, db, 1, &wkey);
	rl_free(wkey);

	retval = 0;
	fprintf(stderr, "End watch_test %d\n", _commit);
cleanup:
	rl_close(db);
	return retval;
}
RL_TEST_MAIN_START(key_test)
{
	long i;
	for (i = 0; i < 3; i++) {
		RL_TEST(basic_test_set_get, i);
		RL_TEST(basic_test_get_or_create, i);
		RL_TEST(basic_test_multidb, i);
		RL_TEST(basic_test_move, i);
		RL_TEST(existing_test_move, i);
		RL_TEST(basic_test_expires, i);
		RL_TEST(basic_test_change_expiration, i);
		RL_TEST(test_delete_with_value, i);
		RL_TEST(test_rename_ok, i);
		RL_TEST(test_rename_overwrite, i);
		RL_TEST(test_rename_no_overwrite, i);
		RL_TEST(test_dbsize, i);
		RL_TEST(test_keys, i);
		RL_TEST(test_randomkey, i);
		RL_TEST(test_flushdb, i);
		RL_TEST(string_version_test, i);
		RL_TEST(list_version_test, i);
		RL_TEST(set_version_test, i);
		RL_TEST(zset_version_test, i);
		RL_TEST(hash_version_test, i);
		RL_TEST(watch_test, i);
	}
	RL_TEST(basic_test_get_unexisting, 0);
	RL_TEST(basic_test_set_delete, 0);
	RL_TEST(basic_test_get_or_create, 0);
}
RL_TEST_MAIN_END
