
#ifndef _CPERF_TEST_VECTRORS_
#define _CPERF_TEST_VECTRORS_

enum Sess_operation 
{
	SESS_OP_ATTACH = 0,
	SESS_OP_CREATE = 1,
	SESS_OP_CREATE_AND_ATTACH = 2,
	SESS_OP_CLOSE = 3,
};

enum Operation_status 
{
	OP_STATUS_NOT_PROCESSED = 0,
	OP_STATUS_SUCC = 1,
	OP_STATUS_FAILED = 2
};

enum Crypto_cipher_algorithm
{
	CRYPTO_CIPHER_AES_CBC = 0,
	CRYPTO_CIPHER_ALGO_LAST
};

enum Crypto_cipher_operation
{
	CRYPTO_CIPHER_OP_ENCRYPT = 0,
	CRYPTO_CIPHER_OP_DECRYPT = 1,
	CRYPTO_CIPHER_OP_LAST
};

enum { MAX_BUFF_LIST = 10 };

struct Dpdk_cryptodev_data_vector {
	struct {
		uint32_t _op_status;
		uint8_t* _op_ctx_ptr;
		uint8_t* _op_outbuff_ptr;
		uint32_t _op_outbuff_len;
		uint32_t _op_in_buff_list_len;

		enum Sess_operation _sess_op;
		// [in] get / remove [out] create
		uint32_t _sess_id;
		// [in] create
		enum Crypto_cipher_algorithm _cipher_algo;
		enum Crypto_cipher_operation _cipher_op;
	} op; // TODO rename

	struct {
		uint8_t *data;
		uint32_t length;
	} cipher_buff_list[MAX_BUFF_LIST];

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
