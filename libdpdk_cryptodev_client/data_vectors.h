
#ifndef _CPERF_TEST_VECTRORS_
#define _CPERF_TEST_VECTRORS_

#include "options.h"

enum Sess_operation {
	SESS_OP_ATTACH = 0,
	SESS_OP_CREATE = 1,
	SESS_OP_CREATE_AND_ATTACH = 2,
	SESS_OP_CLOSE = 3,
};

struct Dpdk_cryptodev_data_vector {
	struct {
		uint32_t _sess_id;
		enum Sess_operation _sess_op;
		enum rte_crypto_cipher_algorithm _cipher_algo;
		enum rte_crypto_cipher_operation _cipher_op;
	} sess;

	struct {
		uint8_t *data;
		uint32_t length;
	} pckhdr; // pdcpinfo

	struct {
		uint8_t *data;
		uint32_t length;
	} ciphertext;

	struct {
		uint8_t *data;
		uint16_t length;
	} cipher_key;

	struct {
		uint8_t *data;
		uint16_t length;
	} cipher_iv;
};

#endif // _CPERF_TEST_VECTRORS_
