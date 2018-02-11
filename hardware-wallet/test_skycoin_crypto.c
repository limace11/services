#include "check.h"

#include "skycoin_crypto.h"


#define FROMHEX_MAXLEN 512

const uint8_t *fromhex(const char *str)
{
	static uint8_t buf[FROMHEX_MAXLEN];
	size_t len = strlen(str) / 2;
	if (len > FROMHEX_MAXLEN) len = FROMHEX_MAXLEN;
	for (size_t i = 0; i < len; i++) {
		uint8_t c = 0;
		if (str[i * 2] >= '0' && str[i*2] <= '9') c += (str[i * 2] - '0') << 4;
		if ((str[i * 2] & ~0x20) >= 'A' && (str[i*2] & ~0x20) <= 'F') c += (10 + (str[i * 2] & ~0x20) - 'A') << 4;
		if (str[i * 2 + 1] >= '0' && str[i * 2 + 1] <= '9') c += (str[i * 2 + 1] - '0');
		if ((str[i * 2 + 1] & ~0x20) >= 'A' && (str[i * 2 + 1] & ~0x20) <= 'F') c += (10 + (str[i * 2 + 1] & ~0x20) - 'A');
		buf[i] = c;
	}
	return buf;
}

START_TEST(test_generate_deterministic_key_pair_seckey)
{
    char seed[256] = "seed";
    uint8_t seckey_digest[SHA256_DIGEST_LENGTH] = {0};
    genereate_deterministic_key_pair_seckey(seed, seckey_digest);
	ck_assert_mem_eq(seckey_digest, fromhex("a7e130694166cdb95b1e1bbce3f21e4dbd63f46df42b48c5a1f8295033d57d04"), 32);
}
END_TEST

START_TEST(test_compute_sha256sum)
{
    char seed[256] = "seed";
    uint8_t digest[SHA256_DIGEST_LENGTH] = {0};
    compute_sha256sum(seed, digest, strlen(seed));

	ck_assert_mem_eq(digest, fromhex("19b25856e1c150ca834cffc8b59b23adbd0ec0389e58eb22b3b64768098d002b"), 32);

    strcpy(seed, "random_seed");
    memset(digest, 0, SHA256_DIGEST_LENGTH);
    compute_sha256sum(seed, digest, strlen(seed));

	ck_assert_mem_eq(digest, fromhex("7b491face15c5be43df3affe42e6e4aab48522a3b564043de464e8de50184a5d"), 32);
}
END_TEST

START_TEST(test_compute_ecdh)
{
    char seed_str[256] = "seed";
	HDNode alice;
    int key_size;
    uint8_t session_key1[65] = {0};

    create_node(seed_str, &alice);
	ck_assert_mem_eq(alice.public_key, fromhex("03008fa0a5668a567cb28ab45e4b6747f5592690c1d519c860f748f6762fa13103"), 33);
	ck_assert_mem_eq(alice.private_key, fromhex("8f609a12bdfc8572590c66763bb05ce609cc0fdcd0c563067e91c06bfd5f1027"), 32);


    hdnode_get_shared_key(&alice, alice.public_key, session_key1, &key_size);


	ck_assert_mem_eq(session_key1, fromhex("907d3c524abb561a80644cdb0cf48e6c71ce33ed6a2d5eed40a771bcf86bd081"), 32);

    ck_assert_int_eq(1, 1);
}
END_TEST

// define test suite and cases
Suite *test_suite(void)
{
	Suite *s = suite_create("skycoin_crypto");
	TCase *tc;

	tc = tcase_create("checksums");
	tcase_add_test(tc, test_generate_deterministic_key_pair_seckey);
	tcase_add_test(tc, test_compute_sha256sum);
	tcase_add_test(tc, test_compute_ecdh);
	suite_add_tcase(s, tc);

	return s;
}


// run suite
int main(void)
{
	int number_failed;
	Suite *s = test_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	if (number_failed == 0) {
		printf("PASSED ALL TESTS\n");
	}
	return number_failed;
}
