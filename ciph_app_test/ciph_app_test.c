///////////////////////////////////
// ciph_app_test

#include <math.h>
#include <unistd.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ciph_agent_c.h"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>

enum { MAX_BUFF_SIZE = 9000 };
enum { TEST_BUFF_SIZE = 9000 };

uint8_t plaintext[MAX_BUFF_SIZE] = {};
uint8_t plaintext_quant[64] = {
0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,
};

uint8_t ciphertext[MAX_BUFF_SIZE] = {};
uint8_t ciphertext_quant[64] = {
0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,
};

uint8_t iv[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

uint8_t cipher_key[] = {
	0xE4, 0x23, 0x33, 0x8A, 0x35, 0x64, 0x61, 0xE2, 0x49, 0x03, 0xDD, 0xC6, 0xB8, 0xCA, 0x55, 0x7A
};

const uint16_t QID_USER = 0, QID_CTRL = 1;

enum { THR_CNT = 6 };
enum { MAX_ACTIVE_SESS_NUM = 300 };
enum { MAX_OUTBUFF_LEN = TEST_BUFF_SIZE }; 

uint16_t USE_RND = 0;

typedef struct
{
	uint64_t setup_sess_pck_seq;
	int32_t setup_sess_id;
	uint32_t is_recv_resp;
	uint32_t resp_status;
}setup_sess_ctx_t;

typedef struct
{
	int32_t setup_sess_id;
	uint32_t is_recv_resp;
}setup_sess_ctx_test_t;

typedef struct
{
  	long index;
	on_ops_complete_CallBk_t cb;
  	uint32_t packet_size;
  	uint32_t num_pck;
  	uint32_t num_pck_per_batch;

	uint32_t target_pps;

	struct timespec start;
	struct timespec end;

	uint32_t total_size;
	uint32_t data_failed;
	uint32_t op_failed;

	setup_sess_ctx_t setup_sess_ctx;

	enum Crypto_cipher_algorithm cipher_algo;
    enum Crypto_cipher_operation cipher_op;

	uint32_t active_sess_ids[MAX_ACTIVE_SESS_NUM];

	uint8_t outbuff[MAX_OUTBUFF_LEN];

	uint32_t setup_sess_id_enc;
	uint32_t setup_sess_id_dec;
	uint64_t seq;
} thread_data_t;

thread_data_t thread_data[THR_CNT];
pthread_t thread[THR_CNT];

const uint16_t SLEEP_TO_FACTOR = 1;

const uint32_t MAX_CONN_CLIENT_BURST = 64;

const uint32_t PCK_NUM = 10000000; //10000000;

const uint32_t PCK_PER_BATCH_NUM = 32; //10000000;

const uint32_t MAX_POLL_RETRIES = 1000000;

void print_buff(uint8_t* data, uint32_t len)
{
  	printf("test app length %d\n", len);
	
  	//uint32_t c = 0;
	uint32_t i;

  	//printf("%03d ", c++);

  	for (i = 0; i < len; ++i)
  	{
    	printf("0x%02X%s", data[i], ( i + 1 ) % 16 == 0 ? ",\r\n" : ", " );

    	//if (( i + 1 ) % 16 == 0)
      	//	printf("%03d ", c++);
  	}

  	printf("\r\n");
}
  
static double get_delta_usec(struct timespec start, struct timespec end)
{
    uint64_t t1 = end.tv_sec - start.tv_sec;
    uint64_t t2;
    if (end.tv_nsec > start.tv_nsec)
    {
        t2 = end.tv_nsec - start.tv_nsec;
    }
    else
    {
        t2 = start.tv_nsec - end.tv_nsec;
        t1--;
    }
        
	double tmp = t1 * 1000 * 1000;
    tmp += t2 / 1000.0;

	return tmp;
}

void on_ops_complete_cb_0 (uint32_t cid, uint16_t qid, Crypto_operation* vec, uint32_t size)
{	
	uint32_t j;

//printf ("cid %d qid %d\n", cid, qid);

    for (j = 0; j < size; ++j)
    {
		//printf ("seq %d\n", vec[j].op.seq);
		//printf ("ctx %x\n", vec[j].op.op_ctx_ptr);

      	// control plain
      	if (vec[j].op.op_type == CRYPTO_OP_TYPE_SESS_CREATE)
      	{
        	printf ("session created id: %u seq %" PRIu64 " == %" PRIu64 "\n", vec[j].op.sess_id, vec[j].op.seq, thread_data[cid].setup_sess_ctx.setup_sess_pck_seq);
      		if (thread_data[cid].setup_sess_ctx.setup_sess_pck_seq == vec[j].op.seq)
			{
			  thread_data[cid].setup_sess_ctx.setup_sess_id = vec[j].op.sess_id;
			  thread_data[cid].setup_sess_ctx.is_recv_resp = 1;
			  thread_data[cid].setup_sess_ctx.resp_status = vec[j].op.op_status;
			}
      	}
      	if (vec[j].op.op_type == CRYPTO_OP_TYPE_SESS_CLOSE)
      	{
        	printf ("session closed %d\n", vec[j].op.sess_id);
      	}

      	if (vec[j].op.op_status != CRYPTO_OP_STATUS_SUCC)
      	{
			//printf ("op failed %d\n", vec[j].op.op_status);
        	thread_data[cid].op_failed++;
      	}

      	// data plain
      	if (vec[j].op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING && vec[j].op.op_status == CRYPTO_OP_STATUS_SUCC)
	  	{
			//print_buff(vec[j].cipher_buff_list.buffs[0].data, vec[j].cipher_buff_list.buffs[0].length);

			if (thread_data[cid].cipher_algo == CRYPTO_CIPHER_AES_CTR && vec[j].op.outbuff_len == 0)
			{
				if (thread_data[cid].cipher_op == CRYPTO_CIPHER_OP_DECRYPT)
				{
					//print_buff(vec[j].op.outbuff_ptr, vec[j].op.outbuff_len);

					if ( 0 != memcmp(plaintext,
							vec[j].op.outbuff_ptr,
							vec[j].op.outbuff_len))
					{
          				thread_data[cid].data_failed++;
						printf ("data_failed %d %d\n", cid, thread_data[cid].data_failed);
					}
				}
				else  if (thread_data[cid].cipher_op == CRYPTO_CIPHER_OP_ENCRYPT)
				{
					//print_buff(vec[j].op.outbuff_ptr, vec[j].op.outbuff_len);

					if ( 0 != memcmp(ciphertext,
							vec[j].op.outbuff_ptr,
							vec[j].op.outbuff_len))
					{
        				thread_data[cid].data_failed++;
						//print_buff(vec[j].op.outbuff_ptr, vec[j].op.outbuff_len);
						printf ("data_failed %d %d\n", cid, thread_data[cid].data_failed);
					}
				}
		  	}

			/*
			if (thread_data[cid].cipher_algo == CRYPTO_CIPHER_NULL && vec[j].op.outbuff_len == 0)
			{
				if (thread_data[cid].cipher_op == CRYPTO_CIPHER_OP_DECRYPT)
				{
					//print_buff(vec[j].op.outbuff_ptr, vec[j].op.outbuff_len);

					if ( 0 != memcmp(ciphertext,
							vec[j].op.outbuff_ptr,
							vec[j].op.outbuff_len))
					{
          				thread_data[cid].data_failed++;
						printf ("data_failed %d %d\n", cid, thread_data[cid].data_failed);
					}
				}
				else  if (thread_data[cid].cipher_op == CRYPTO_CIPHER_OP_ENCRYPT)
				{
					//print_buff(vec[j].op.outbuff_ptr, vec[j].op.outbuff_len);

					if ( 0 != memcmp(plaintext,
							vec[j].op.outbuff_ptr,
							vec[j].op.outbuff_len))
					{
        				thread_data[cid].data_failed++;
						//print_buff(vec[j].op.outbuff_ptr, vec[j].op.outbuff_len);
						printf ("data_failed %d %d\n", cid, thread_data[cid].data_failed);
					}
				}
		  	}
			*/
	  	}
    }

    thread_data[cid].total_size += size;
    //printf ("cid %d Seq len: %u\n", cid, thread_data[cid].total_size);
}

void on_ops_complete_cb_0_v (uint32_t cid, uint16_t qid, Crypto_operation* vec, uint32_t size)
{	
	uint32_t j;

//printf ("cid %d qid %d\n", cid, qid);

    for (j = 0; j < size; ++j)
    {
		//printf ("seq %d\n", vec[j].op.seq);
		//printf ("ctx %x\n", vec[j].op.op_ctx_ptr);

      	// control plain
      	if (vec[j].op.op_type == CRYPTO_OP_TYPE_SESS_CREATE)
      	{
        	printf ("session created id: %u seq %" PRIu64 " == %" PRIu64 "\n", vec[j].op.sess_id, vec[j].op.seq, thread_data[cid].setup_sess_ctx.setup_sess_pck_seq);
      		if (thread_data[cid].setup_sess_ctx.setup_sess_pck_seq == vec[j].op.seq)
			{
			  thread_data[cid].setup_sess_ctx.setup_sess_id = vec[j].op.sess_id;
			  thread_data[cid].setup_sess_ctx.is_recv_resp = 1;
			  thread_data[cid].setup_sess_ctx.resp_status = vec[j].op.op_status;
			}
      	}
      	if (vec[j].op.op_type == CRYPTO_OP_TYPE_SESS_CLOSE)
      	{
        	printf ("session closed %d\n", vec[j].op.sess_id);
      	}

      	if (vec[j].op.op_status != CRYPTO_OP_STATUS_SUCC)
      	{
        	thread_data[cid].op_failed++;
      	}

      	// data plain
      	if (vec[j].op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING && vec[j].op.op_status == CRYPTO_OP_STATUS_SUCC)
	  	{
			//print_buff(vec[j].cipher_buff_list.buffs[0].data, vec[j].cipher_buff_list.buffs[0].length);

			if (thread_data[cid].setup_sess_id_enc == vec[j].op.sess_id)
			{
				//printf("rollback %d\n", vec[j].op.seq);
				
				int32_t res;

			    Crypto_operation op;
    			memset(&op, 0, sizeof(Crypto_operation));
				op.op.seq = ++thread_data[cid].seq;
    			// sess
    			op.op.sess_id = thread_data[cid].setup_sess_id_dec;
    			op.op.op_type = CRYPTO_OP_TYPE_SESS_CIPHERING;
				op.op.cipher_op = CRYPTO_CIPHER_OP_DECRYPT;

				op.op.op_ctx_ptr = NULL;

				op.cipher_buff_list.buff_list_length = 1;
				op.cipher_buff_list.buffs[0].data = vec[j].cipher_buff_list.buffs[0].data;
				op.cipher_buff_list.buffs[0].length = vec[j].cipher_buff_list.buffs[0].length;

    			op.cipher_iv.data = iv;
    			op.cipher_iv.length = 16;
				// outbuf
				op.op.outbuff_ptr = thread_data[cid].outbuff;
				op.op.outbuff_len = MAX_OUTBUFF_LEN;

				res = ciph_agent_send(cid, qid, &op, 1);
				if (res != 0) 
					printf ("ciph_agent_send ERROR\n");
			}
			else
			{
				//print_buff(vec[j].cipher_buff_list.buffs[0].data, vec[j].cipher_buff_list.buffs[0].length);
				if ( 0 == vec[j].op.seq % 1000000 )
					printf("check %d %" PRIu64 "\n", cid, vec[j].op.seq);

				if ( 0 != memcmp(plaintext,
						vec[j].cipher_buff_list.buffs[0].data,
						vec[j].cipher_buff_list.buffs[0].length))
				{
        			thread_data[cid].data_failed++;
					print_buff(vec[j].cipher_buff_list.buffs[0].data, vec[j].cipher_buff_list.buffs[0].length);
					printf ("data_failed %d %d\n", cid, thread_data[cid].data_failed);
				}
		  	}
	  	}
    }

    thread_data[cid].total_size += size;
    //printf ("cid %d Seq len: %u\n", cid, thread_data[cid].total_size);
}

uint8_t dummy_ctx[1024];

int32_t create_session(long cid, uint16_t qid, uint64_t seq, uint32_t algo, uint32_t op)
{
	int32_t res;

    Crypto_operation op_sess;
    memset(&op_sess, 0, sizeof(Crypto_operation));
	op_sess.op.seq = seq;
	// sess
    op_sess.op.op_type = CRYPTO_OP_TYPE_SESS_CREATE;
    op_sess.op.cipher_algo = algo;
    
	if (USE_RND)
	{
		op_sess.op.cipher_op = rand() % 2;
	}
	else
	{
		op_sess.op.cipher_op = op;
	}

//	if (thread_data[cid].cipher_algo != CRYPTO_CIPHER_NULL)
	{
		op_sess.cipher_key.length = 16;
    
		if (USE_RND)
		{
			// fill with random data
			for(size_t i = 0; i < op_sess.cipher_key.length; i++)
    			op_sess.cipher_key.data[i] = rand() % 256;
		}
		else
		{
			op_sess.cipher_key.data = cipher_key;
		}
	}
/*
	else
	{
		op_sess.cipher_key.data = NULL;
    	op_sess.cipher_key.length = 0;
	}
*/	

    memset(&thread_data[cid].setup_sess_ctx, 0, sizeof(setup_sess_ctx_t));
	thread_data[cid].setup_sess_ctx.setup_sess_id = -1;
	thread_data[cid].setup_sess_ctx.setup_sess_pck_seq = seq;

    ciph_agent_send(cid, qid, &op_sess, 1);

    // poll and recv created session id
	uint32_t retries = 0;

	while (!thread_data[cid].setup_sess_ctx.is_recv_resp)
	{
    	res = ciph_agent_poll(cid, qid, MAX_CONN_CLIENT_BURST);
		if (res == -2) 
		{
			printf ("ciph_agent_poll ERROR\n");
			return res;
		}

		if (++retries > MAX_POLL_RETRIES)
		{
			printf ("ciph_agent_poll ERROR retries > MAX_RETRIES - server is down\n");
			return -1;
		}
	}

	if (thread_data[cid].setup_sess_ctx.resp_status != CRYPTO_OP_STATUS_SUCC)
	{
		printf ("FAILED resp status %d\n",  thread_data[cid].setup_sess_ctx.resp_status);
		return -1;
	}

	return 0;
}

int32_t close_session(long cid, uint16_t qid, uint64_t seq, uint64_t sess_id)
{
    Crypto_operation op_sess_del;
    memset(&op_sess_del, 0, sizeof(Crypto_operation));
	op_sess_del.op.seq = seq;
    // sess
    op_sess_del.op.sess_id = sess_id;
    op_sess_del.op.op_type = CRYPTO_OP_TYPE_SESS_CLOSE;

	ciph_agent_send(cid, qid, &op_sess_del, 1);

	return 0;
}

int32_t cipher(long cid, uint16_t qid, uint64_t seq, uint64_t sess_id, uint32_t cipher_op)
{
	int32_t res;

    Crypto_operation op;
    memset(&op, 0, sizeof(Crypto_operation));
	op.op.seq = seq;
    // sess
    op.op.sess_id = sess_id;
    op.op.op_type = CRYPTO_OP_TYPE_SESS_CIPHERING;
	op.op.cipher_op = cipher_op;

	if (USE_RND)
	{
    	op.op.cipher_op = rand() % 2;
	}

	op.op.op_ctx_ptr = dummy_ctx;

	static int cnt = 0;
	// 1% 300 - 1500 bytes
	++cnt;

	//uint32_t BUFFER_TOTAL_LEN = TEST_BUFF_SIZE;
	//printf("len %d\n", BUFFER_TOTAL_LEN);

	uint32_t BUFFER_TOTAL_LEN = 1 + rand() % 500;

//	if (cnt % 100 == 0) 
//		BUFFER_TOTAL_LEN = 300 + rand() % 8500;
	// TODO kills server
	if (cnt % 1000000 == 0) 
		BUFFER_TOTAL_LEN = 10000; // too large

	//uint32_t BUFFER_TOTAL_LEN = 1 + rand() % 15;

	uint32_t BUFFER_SEGMENT_NUM = 1 + rand() % 15;
	BUFFER_SEGMENT_NUM = (BUFFER_SEGMENT_NUM > BUFFER_TOTAL_LEN) ? 
												BUFFER_TOTAL_LEN : 
												BUFFER_SEGMENT_NUM;
	//BUFFER_SEGMENT_NUM = 1;

	op.cipher_buff_list.buff_list_length = BUFFER_SEGMENT_NUM;
	
	uint32_t total_len = 0;
	uint32_t step = BUFFER_TOTAL_LEN / BUFFER_SEGMENT_NUM;
	for (uint32_t i = 0; i < op.cipher_buff_list.buff_list_length; ++i)
	{
		if (USE_RND)
		{
			// fill with random data
			for(size_t i = 0; i < op.cipher_buff_list.buffs[i].length; i++)
    			op.cipher_buff_list.buffs[i].data[i] = rand() % 256;
		}
		else
		{
	    	op.cipher_buff_list.buffs[i].data = ((cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) ? plaintext : ciphertext) + i*step;
			op.cipher_buff_list.buffs[i].length = step;
		}

		total_len += step;
		// last segment
		if (i == op.cipher_buff_list.buff_list_length - 1)
			op.cipher_buff_list.buffs[i].length += (BUFFER_TOTAL_LEN - total_len);
	}
	
	//op.cipher_buff_list.buffs[0].data = ((cipher_op == CRYPTO_CIPHER_OP_ENCRYPT) ? plaintext : ciphertext);
	//op.cipher_buff_list.buffs[0].length = BUFFER_TOTAL_LEN;

	//printf ("BUFFER_TOTAL_LEN %d BUFFER_SEGMENT_NUM %d\n", BUFFER_TOTAL_LEN, BUFFER_SEGMENT_NUM);

    op.cipher_iv.data = iv;
    op.cipher_iv.length = 16;
	
	if (USE_RND)
	{
		// fill with random data
		for(size_t i = 0; i < op.cipher_iv.length; i++)
    		op.cipher_iv.data[i] = rand() % 256;
	}

	// outbuf
	op.op.outbuff_ptr = thread_data[cid].outbuff;
	op.op.outbuff_len = MAX_OUTBUFF_LEN;

	res = ciph_agent_send(cid, qid, &op, 1);
	//if (res != 0) 
	//	printf ("ciph_agent_send ERROR\n");

	return 0;
}

enum TEST_TYPE
{
	TT_MAX_TP,
	TT_TARGET_TP,
	TT_VERIFY
};

enum TEST_TYPE test_type = TT_VERIFY;
//enum TEST_TYPE test_type = TT_MAX_TP;
//enum TEST_TYPE test_type = TT_TARGET_TP;

// TODO negative cases
// bad sessid, too big buffer
void* send_proc(void* data)
{
	uint32_t cc = 0;
	uint32_t cc_1 = 0;
	uint8_t res;
	uint32_t i;

	uint32_t retries;

	long cid = *((long*) data);
  	uint32_t num_pck = thread_data[cid].num_pck;
  	uint32_t num_pck_per_batch = thread_data[cid].num_pck_per_batch;

	/*
   	for( i = 0 ; i < 100 ; i++ ) 
	{
    	printf("%d\n", rand() % 50);
   	}
*/

	thread_data[cid].seq = 0;

    ciph_agent_conn_alloc(cid, thread_data[cid].cb);

    printf ("start %ld ...\n", cid);

	uint32_t setup_sess_id;
	uint32_t cipher_op;

	if (test_type == TT_VERIFY)
	{
		res = create_session(cid, QID_CTRL, thread_data[cid].seq, thread_data[cid].cipher_algo, CRYPTO_CIPHER_OP_ENCRYPT);
		if (0 != res) 
			printf ("create_session ERROR\n");
		else
			thread_data[cid].setup_sess_id_enc = thread_data[cid].setup_sess_ctx.setup_sess_id;

		res = create_session(cid, QID_CTRL, thread_data[cid].seq, thread_data[cid].cipher_algo, CRYPTO_CIPHER_OP_DECRYPT);
		if (0 != res) 
			printf ("create_session ERROR\n");
		else
			thread_data[cid].setup_sess_id_dec = thread_data[cid].setup_sess_ctx.setup_sess_id;

		setup_sess_id = thread_data[cid].setup_sess_id_enc;
		cipher_op = CRYPTO_CIPHER_OP_ENCRYPT;
	}
	else
	{
		// create 3 sessions
		res = create_session(cid, QID_CTRL, thread_data[cid].seq, thread_data[cid].cipher_algo, thread_data[cid].cipher_op);
		if (0 != res) printf ("create_session ERROR\n");

		res = create_session(cid, QID_CTRL, thread_data[cid].seq, thread_data[cid].cipher_algo, thread_data[cid].cipher_op);
		if (0 != res) printf ("create_session ERROR\n");

		res = create_session(cid, QID_CTRL, thread_data[cid].seq, thread_data[cid].cipher_algo, thread_data[cid].cipher_op);
		if (0 != res) printf ("create_session ERROR\n");

		setup_sess_id = thread_data[cid].setup_sess_ctx.setup_sess_id;
		cipher_op = thread_data[cid].cipher_op;
	}

	usleep(1000 * 1000); 

	// warmup
    for (i = 0; i < num_pck_per_batch; ++i)
    {
		cipher(cid, QID_USER, ++thread_data[cid].seq, setup_sess_id, cipher_op);
    }
    
	res = ciph_agent_poll(cid, QID_USER, MAX_CONN_CLIENT_BURST);
	if (res == -2) printf ("ciph_agent_poll ERROR\n");
	
    timespec_get (&thread_data[cid].start, TIME_UTC);

    uint32_t num_batch = num_pck / num_pck_per_batch;
    
	uint32_t pps = thread_data[cid].target_pps;
	int32_t send_to_usec = 2 * (1000.0 * 1000.0 * num_pck_per_batch) / pps;
	struct timespec start_send, end_send;
	struct timespec start_sleep, end_sleep;

    printf ("run ... %d\n", send_to_usec);
	int32_t to_sleep = 0;
	uint32_t c;
    for(c = 0; c < num_batch; ++c)
    {
		memset (&start_send, 0, sizeof (start_send));
		timespec_get (&start_send, TIME_UTC);

      	for (i = 0; i < num_pck_per_batch; ++i)
      	{

			// create session onece per 10 pck
			// and close any of active sessions if > threshold
			++cc_1;
			if (cc_1 > 1000000)
			{
				cc_1 = 0;

				res = close_session(cid, QID_USER, ++thread_data[cid].seq, thread_data[cid].setup_sess_ctx.setup_sess_id);
				if (0 != res) printf ("close_session ERROR\n");

				res = create_session(cid, QID_CTRL, ++thread_data[cid].seq, thread_data[cid].cipher_algo, thread_data[cid].cipher_op);
				if (0 != res) printf ("create_session ERROR\n");
			}
	
			cipher(cid, QID_USER, ++thread_data[cid].seq, setup_sess_id, cipher_op);
      	}

    	res = ciph_agent_poll(cid, QID_USER, MAX_CONN_CLIENT_BURST);
		if (res == -2) printf ("ciph_agent_poll ERROR\n");

		if (TT_TARGET_TP == test_type)
		{
			memset (&end_send, 0, sizeof (end_send));
        	timespec_get (&end_send, TIME_UTC);

			int32_t send_took = (int32_t) get_delta_usec(start_send, end_send);
			to_sleep += send_to_usec;
			to_sleep -= send_took;
			//printf ("Curr pps %ld %ld\n", cid, to_sleep);
			while (to_sleep > send_to_usec * SLEEP_TO_FACTOR)
			{
				memset (&start_sleep, 0, sizeof (start_sleep));
				timespec_get (&start_sleep, TIME_UTC);

				usleep(send_to_usec * SLEEP_TO_FACTOR);
			
	  			cc++;
				// control pps
				if (cc > 1000) 
				{
					cc = 0;
      				timespec_get (&thread_data[cid].end, TIME_UTC);

      				double tmp = 1000.0 * thread_data[cid].seq / get_delta_usec(thread_data[cid].start, thread_data[cid].end);
	  				printf ("Curr Kpps %ld %f\n", cid, tmp);
				}
			
				memset (&end_sleep, 0, sizeof (end_sleep));
        		timespec_get (&end_sleep, TIME_UTC);

				int32_t slept = (int32_t) get_delta_usec(start_sleep, end_sleep);
				to_sleep -= slept;
			}
		}
		else
		{
/*
			cc++;
			// control pps
			if (cc > 1000) 
			{
				cc = 0;
      			timespec_get (&thread_data[cid].end, TIME_UTC);

      			double tmp = 1000.0 * thread_data[cid].seq / get_delta_usec(thread_data[cid].start, thread_data[cid].end);
	  			printf ("Curr pps %ld %f\n", cid, tmp);
			}
*/
		}
    	//printf ("num %ld %ld ...\n", cid, send_to_usec);
    }
/*
// SIGV
int *a;
*a = 0;
*/
	printf ("done send\n");

    timespec_get (&thread_data[cid].end, TIME_UTC);
    printf ("\n\n");
    printf ("Pakcet sequence finished!\n");
    printf ("Seq len: %lu\n", thread_data[cid].seq + 1);

    // flush
	printf ("flush ...\n");
	retries = 0;

	uint32_t all_pck_count = thread_data[cid].seq + 1;  // num_pck + num_pck_per_batch + sess cr

	// NOTE. for test - close session before flush
	// in verify test we rollback packets in poll - so last packets (batch) will be skipped at server (NO SESSION)
	/*
	res = close_session(cid, QID_USER, ++thread_data[cid].seq, thread_data[cid].setup_sess_ctx.setup_sess_id);
	if (0 != res) printf ("close_session ERROR\n");
*/
	// disconnect while send (client crash simulation)
    //while (thread_data[cid].total_size < all_pck_count / 2)
    while (thread_data[cid].total_size < all_pck_count)
	{
      	res = ciph_agent_poll(cid, QID_USER, MAX_CONN_CLIENT_BURST);
		if (res == -2) printf ("ciph_agent_poll ERROR\n");;

		if (++retries > MAX_POLL_RETRIES)
		{
			printf ("ciph_agent_poll ERROR retries > MAX_RETRIES - server is down\n");;
			break;
		}
	}

	// NOTE. for test - close session twice
	res = close_session(cid, QID_USER, ++thread_data[cid].seq, thread_data[cid].setup_sess_ctx.setup_sess_id);
	if (0 != res) printf ("close_session ERROR\n");

	// leave hanged sessions to check cleanup
/*
	res = close_session(cid, QID_USER, ++thread_data[cid].seq, (thread_data[cid].setup_sess_ctx.setup_sess_id - 1) );
	if (0 != res) printf ("close_session ERROR\n");

	res = close_session(cid, QID_USER, ++thread_data[cid].seq, (thread_data[cid].setup_sess_ctx.setup_sess_id - 2) );
	if (0 != res) printf ("close_session ERROR\n");
*/

    double tmp = 1000.0 * thread_data[cid].seq / get_delta_usec(thread_data[cid].start, thread_data[cid].end);
    printf ("Average Kpps: %f\n", tmp);
    printf ("Average TP: %f Gb/s\n", (tmp * thread_data[cid].packet_size) * 8.0 / 1000000.0);

	printf ("Seq len: %u\n", thread_data[cid].total_size);
	
	printf ("data_failed %d\n", thread_data[cid].data_failed);
    printf ("op_failed %d\n", thread_data[cid].op_failed);

    ciph_agent_conn_free(cid);

	pthread_exit (NULL);
}

/*
#include<signal.h>
#include<unistd.h>

void del_sigs()
{
   struct sigaction sa;

   memset(&sa, 0, sizeof(struct sigaction));
   sigemptyset(&sa.sa_mask);
   sa.sa_handler = SIG_DFL;

   sigaction(SIGILL, &sa, NULL);
   sigaction(SIGSEGV, &sa, NULL);
   sigaction(SIGABRT, &sa, NULL);
   sigaction(SIGTERM, &sa, NULL);
   sigaction(SIGHUP, &sa, NULL);
}

static void signal_segv(int signum, struct siginfo_t * info, void *ptr)
{
	printf("\nALL Error handler\n");

	del_sigs();
	raise(signum);
}
*/

#include <unistd.h>

pid_t get_thread_id_by_name(const char* th_name)
{
	pid_t pid = getpid();

	execlp(	"ps","-T", "-p", pid, "|", "grep",  th_name, "|", "awk", "'{print $2}'");
}

int main(int argc, char* argv[])
{
/*
	struct sigaction act;
   	sigset_t set;
   	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = signal_segv;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
*/
	long instance_id = 12;
/*
	const char* s = getenv("INSTANCE_ID");
	if (NULL == s)
	{
		printf("WARN!!! no  INSTANCE_ID var\n");
	}
	else
	{
		instance_id = strtol(s, NULL, 10);
	}
*/

	// init data
	for (uint32_t i = 0; i < 4; i++)
		memcpy(plaintext + i*64, plaintext_quant, 64);

	for (uint32_t i = 0; i < 4; i++)
		memcpy(ciphertext + i*64, ciphertext_quant, 64);

    long cid_0 = 0;
    long cid_1 = 1;

	time_t t;
	srand((unsigned) time(&t));

    ciph_agent_init(instance_id);

	memset(thread_data, 0, THR_CNT * sizeof(thread_data_t));

	while(1)
	{
	thread_data[cid_0].index = cid_0;
	thread_data[cid_0].packet_size = TEST_BUFF_SIZE;
  	thread_data[cid_0].num_pck = PCK_NUM;
	thread_data[cid_0].target_pps = 160000;
  	thread_data[cid_0].num_pck_per_batch = PCK_PER_BATCH_NUM;
    memset (&thread_data[cid_0].start, 0, sizeof (thread_data[cid_0].start));
    memset (&thread_data[cid_0].end, 0, sizeof (thread_data[cid_0].end));
	thread_data[cid_0].total_size = 0;
	thread_data[cid_0].cb = (TT_VERIFY == test_type) ? on_ops_complete_cb_0_v : on_ops_complete_cb_0;
	//thread_data[cid_0].cipher_algo = CRYPTO_CIPHER_SNOW3G_UEA2;
	thread_data[cid_0].cipher_algo = CRYPTO_CIPHER_AES_CTR;
	//thread_data[cid_0].cipher_algo = CRYPTO_CIPHER_NULL;
    thread_data[cid_0].cipher_op = CRYPTO_CIPHER_OP_DECRYPT;
	
	thread_data[cid_1].index = cid_1;
	thread_data[cid_1].packet_size = TEST_BUFF_SIZE;
  	thread_data[cid_1].num_pck = PCK_NUM / 2;
	thread_data[cid_1].target_pps = 160000 / 2;
  	thread_data[cid_1].num_pck_per_batch = PCK_PER_BATCH_NUM;
    memset (&thread_data[cid_1].start, 0, sizeof (thread_data[cid_1].start));
    memset (&thread_data[cid_1].end, 0, sizeof (thread_data[cid_1].end));
	thread_data[cid_1].total_size = 0;
	thread_data[cid_1].cb = (TT_VERIFY == test_type) ? on_ops_complete_cb_0_v : on_ops_complete_cb_0;
	//thread_data[cid_1].cipher_algo = CRYPTO_CIPHER_SNOW3G_UEA2;
	thread_data[cid_1].cipher_algo = CRYPTO_CIPHER_AES_CTR;
	//thread_data[cid_1].cipher_algo = CRYPTO_CIPHER_NULL;
	if (TT_VERIFY == test_type)
	{
		thread_data[cid_1].cipher_algo = ( rand() % CRYPTO_CIPHER_ALGO_LAST );
	}

    thread_data[cid_1].cipher_op = CRYPTO_CIPHER_OP_DECRYPT;

	//int s;
	void *res;

	usleep(1000 * 1000); 

	if (TT_TARGET_TP == test_type)
	{
  		pthread_create (&thread[cid_0], NULL, send_proc, (void *)&cid_0);
		printf("cr 0\n");
	}

	pthread_create (&thread[cid_1], NULL, send_proc, (void *)&cid_1);
	printf("cr 1\n");

	if (TT_TARGET_TP == test_type)
	{
		pthread_join(thread[cid_0], &res);
		printf("Joined with thread %ld; returned value was %s\n", cid_0, (char *) res);
	}

	pthread_join(thread[cid_1], &res);
	printf("Joined with thread %ld; returned value was %s\n", cid_1, (char *) res);

	}

	ciph_agent_cleanup();

    return 0;
}


