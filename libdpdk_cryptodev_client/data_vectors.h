
#ifndef _CPERF_TEST_VECTRORS_
#define _CPERF_TEST_VECTRORS_

/*
*/
enum Sess_operation 
{
	SESS_OP_ATTACH = 0,
	SESS_OP_CREATE = 1,
	SESS_OP_CREATE_AND_ATTACH = 2,
	SESS_OP_CLOSE = 3,
};

/*
*/
enum Operation_status 
{
	OP_STATUS_NOT_PROCESSED = 0,
	OP_STATUS_SUCC = 1,
	OP_STATUS_FAILED = 2
};

/*
*/
enum Crypto_cipher_algorithm
{
	CRYPTO_CIPHER_AES_CBC = 0,
	CRYPTO_CIPHER_SNOW3G_UEA2,
	CRYPTO_CIPHER_ALGO_LAST
};

/*
*/
enum Crypto_cipher_operation
{
	CRYPTO_CIPHER_OP_ENCRYPT = 0,
	CRYPTO_CIPHER_OP_DECRYPT,
	CRYPTO_CIPHER_OP_LAST
};

enum { MAX_BUFF_LIST = 100 };

struct Operation_t {
		uint32_t _op_status;
		uint8_t* _op_ctx_ptr;
		uint8_t* _op_outbuff_ptr;
		uint32_t _op_outbuff_len;
		uint32_t _op_in_buff_list_len;
		uint64_t _seq;

		enum Sess_operation _sess_op;
		// [in] for SESS_OP_ATTACH / SESS_OP_CLOSE [out] for SESS_OP_CREATE
		uint32_t _sess_id;
		// used only for SESS_OP_CREATE
		enum Crypto_cipher_algorithm _cipher_algo;
		enum Crypto_cipher_operation _cipher_op;
};

struct Buff_t {
		uint8_t *data;
		uint32_t length;
};

struct Dpdk_cryptodev_data_vector {
	struct Operation_t op;

	struct Buff_t cipher_buff_list[MAX_BUFF_LIST];
	struct Buff_t cipher_key;
	struct Buff_t cipher_iv;
};

#endif // _CPERF_TEST_VECTRORS_
