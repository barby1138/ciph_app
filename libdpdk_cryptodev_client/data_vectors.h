#ifndef _CRYPTO_OPERATION_
#define _CRYPTO_OPERATION_

enum Crypto_operation_type
{
	CRYPTO_OP_TYPE_SESS_CIPHERING = 0,
	CRYPTO_OP_TYPE_SESS_CREATE,
	CRYPTO_OP_TYPE_SESS_CLOSE,
};

enum Crypto_operation_status 
{
	CRYPTO_OP_STATUS_NOT_PROCESSED = 0,
	CRYPTO_OP_STATUS_SUCC,
	CRYPTO_OP_STATUS_FAILED
};

enum Crypto_cipher_algorithm
{
//	CRYPTO_CIPHER_AES_CBC = 0,
	CRYPTO_CIPHER_AES_CTR = 0,
	CRYPTO_CIPHER_SNOW3G_UEA2,
	CRYPTO_CIPHER_NULL,
	CRYPTO_CIPHER_ALGO_LAST
};

enum Crypto_cipher_operation
{
	CRYPTO_CIPHER_OP_ENCRYPT = 0,
	CRYPTO_CIPHER_OP_DECRYPT,
	CRYPTO_CIPHER_OP_LAST
};

enum { 
	MAX_CYPTO_BUFF_TOTAL_LEN_BYTES = 9000,
	MAX_CYPTO_BUFF_LIST_SIZE = 100 
};

typedef struct Crypto_operation_context {
	uint32_t op_status;

	uint8_t* op_ctx_ptr;

	uint8_t* outbuff_ptr;
	uint32_t outbuff_len;

	uint64_t seq;

	enum Crypto_operation_type op_type;
	// [in] for CRYPTO_OP_TYPE_SESS_CIPHERING / CRYPTO_OP_TYPE_SESS_CLOSE [out] for CRYPTO_OP_TYPE_SESS_CREATE
	uint32_t sess_id;
	// used only for CRYPTO_OP_TYPE_SESS_CREATE
	enum Crypto_cipher_algorithm cipher_algo;
	enum Crypto_cipher_operation cipher_op;

	uint32_t reserved_1;
}Crypto_operation_context;

typedef struct Crypto_buff {
	uint32_t length;
	uint8_t* data;
}Crypto_buff;

typedef struct Crypto_buff_list {
	uint32_t buff_list_length;
	struct Crypto_buff buffs[MAX_CYPTO_BUFF_LIST_SIZE];
}Crypto_buff_list;

typedef struct Crypto_operation {
	Crypto_operation_context op;

	Crypto_buff_list cipher_buff_list;
	Crypto_buff cipher_key;
	Crypto_buff cipher_iv;
}Crypto_operation;

#endif // _CRYPTO_OPERATION_
