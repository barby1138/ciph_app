///////////////////////////////////
// ciph_app_test

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <math.h>
#include <fstream>
#include <stdexcept>

#include <cstdint> // uint64_t
#include <memory>
 #include <unistd.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// libcommon
#include "exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ciph_agent_c.h"
#include <thread>

#include "memcpy_fast.h"

uint8_t plaintext[128] = {
0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b

};

uint8_t ciphertext[128] = {
0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55

};

uint8_t iv[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

uint8_t cipher_key[] = {
	0xE4, 0x23, 0x33, 0x8A, 0x35, 0x64, 0x61, 0xE2, 0x49, 0x03, 0xDD, 0xC6, 0xB8, 0xCA, 0x55, 0x7A
};

enum { MAX_OUTBUFF_LEN = 1500 };
uint8_t outbuff[MAX_OUTBUFF_LEN];

enum { THR_CNT = 2 };
typedef struct
{
  	long index;
	on_job_complete_cb_t cb;
  	uint32_t packet_size;
  	uint32_t num_pck;
  	uint32_t num_pck_per_batch;
	uint32_t target_pps;
	struct timespec start;
	struct timespec end;
	uint32_t g_size;
	uint32_t g_data_failed;
	uint32_t g_op_failed;
	int g_setup_sess_id;

	Crypto_cipher_algorithm cipher_algo;
    Crypto_cipher_operation cipher_op;
} thread_data_t;

thread_data_t thread_data[THR_CNT];
pthread_t thread[THR_CNT];

const int MAX_CONN_CLIENT_BURST = 64;

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
    tmp += t2 / 1000;

	return tmp;
}

void on_job_complete_cb_0 (int index, struct Dpdk_cryptodev_data_vector* vec, uint32_t size)
{	
    for(uint32_t j = 0; j < size; ++j)
    {
      	if (vec[j].op._op_status != OP_STATUS_SUCC)
      	{
        	thread_data[index].g_op_failed++;
          	continue;
      	}

      	// control plain
      	if (vec[j].op._sess_op == SESS_OP_CREATE)
      	{
        	printf ("session created id: %d\n", vec[j].op._sess_id);
      		thread_data[index].g_setup_sess_id = vec[j].op._sess_id;
      	}
      	if (vec[j].op._sess_op == SESS_OP_CLOSE)
      	{
        	printf ("session closed %d\n", vec[j].op._sess_id);
      	}

      	// data plain
      	if (vec[j].op._sess_op == SESS_OP_ATTACH)
	  	{
			if (vec[j].cipher_buff_list[0].length > vec[j].op._op_outbuff_len)
			{
				printf ("outbuff too small\n");	
				thread_data[index].g_data_failed++;
			}
			else
			{
				clib_memcpy_fast(vec[j].op._op_outbuff_ptr, 
					vec[j].cipher_buff_list[0].data, 
					vec[j].cipher_buff_list[0].length);

				vec[j].op._op_outbuff_len = vec[j].cipher_buff_list[0].length;

				// invalidate memif buffers - will be returned to pool
				vec[j].cipher_buff_list[0].data = NULL;
				vec[j].cipher_buff_list[0].length = 0;
	
				if (thread_data[index].cipher_op == CRYPTO_CIPHER_OP_DECRYPT)
					if ( 0 != memcmp(plaintext,
							vec[j].op._op_outbuff_ptr,
							vec[j].op._op_outbuff_len))
          				thread_data[index].g_data_failed++;
				else  if (thread_data[index].cipher_op == CRYPTO_CIPHER_OP_ENCRYPT)
					if ( 0 != memcmp(ciphertext,
							vec[j].op._op_outbuff_ptr,
							vec[j].op._op_outbuff_len))
          				thread_data[index].g_data_failed++;
				
			}
	  	}
    }

    thread_data[index].g_size += size;
    //printf ("Seq len: %u\n", g_size);
	// control pps
	/*
	static int cc[2] = { 0 };
	if (thread_data[index].g_size > cc[index] * 100000) 
	{
	  cc[index]++;
      timespec_get (&thread_data[index].end, TIME_UTC);

      double tmp = 1000.0 * thread_data[index].g_size / get_delta_usec(thread_data[index].start, thread_data[index].end);
	  printf ("Curr pps %d %f\n", index, tmp);
	}
*/
    if (thread_data[index].g_size == thread_data[index].num_pck + 
					thread_data[index].num_pck_per_batch + 
					1 /*sess cr*/ + 1 /*sess del*/)
    {
      timespec_get (&thread_data[index].end, TIME_UTC);
      printf ("\n\n");
      printf ("Pakcet sequence finished!\n");
      printf ("Seq len: %u\n", thread_data[index].g_size);
      uint64_t t1 = thread_data[index].end.tv_sec - thread_data[index].start.tv_sec;
      uint64_t t2;
      if (thread_data[index].end.tv_nsec > thread_data[index].start.tv_nsec)
      {
        t2 = thread_data[index].end.tv_nsec - thread_data[index].start.tv_nsec;
      }
      else
      {
        t2 = thread_data[index].start.tv_nsec - thread_data[index].end.tv_nsec;
        t1--;
      }
      printf ("Seq time: %lus %luns\n", t1, t2);
      double tmp = t1;
      tmp += t2 / 1e+9;
      tmp = thread_data[index].g_size / tmp;
      printf ("Average pps: %f\n", tmp);
      printf ("Average TP: %f Mb/s\n", (tmp * thread_data[index].packet_size) * 8.0 / 1000000.0);

      printf ("g_data_failed %d\n", thread_data[index].g_data_failed);
      printf ("g_op_failed %d\n", thread_data[index].g_op_failed);
    }
}

///////////////////
// memcpy
//

int generate_packet (void* src_buff, uint32_t* out_len,  uint32_t pck_size)
{
	clib_memcpy_fast(src_buff, ciphertext, 64);
	*out_len = 64;

	return 0;
}

int test_memcpy (int a_pck_size)
{
#if __AVX512F__ //__AVX512BITALG__
printf("memcpy __AVX512F__\n");
#elif __AVX2__
printf("memcpy __AVX2__\n");
#elif __SSSE3__
printf("memcpy __SSSE3__\n");
#else
printf("memcpy default\n");
#endif

  	char dst_buff[2048];
  	char src_buff[2048];
  	uint32_t len;
  	int i;

  	int seq_len = 1000 * 1000000;
  	int pck_size = a_pck_size;

  	generate_packet ((void *)src_buff, &len,  pck_size);

  	struct timespec start, end;
  	memset (&start, 0, sizeof (start));
  	memset (&end, 0, sizeof (end));

  	timespec_get (&start, TIME_UTC);
  	for (i = 0; i < seq_len; i++)
  	{
    	clib_memcpy_fast(dst_buff, src_buff, len);
  	}
  	timespec_get (&end, TIME_UTC);
  	printf ("\n\n");
  	printf ("Pakcet sequence copied!\n");
  	printf ("Seq len: %u\n", seq_len);
  	printf ("Pck size: %u\n", pck_size);
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

  	printf ("Seq time: %lus %luns\n", t1, t2);

	return 0;
}
///////////////////////////

enum { SLEEP_TO_FACTOR = 4 };

void* send_proc(void* data)
{
	int res;
	uint64_t seq = 0;

	long conn_id = *((long*) data);
  	uint32_t num_pck = thread_data[conn_id].num_pck;
  	uint32_t num_pck_per_batch = thread_data[conn_id].num_pck_per_batch;

    ciph_agent_conn_alloc(conn_id, CA_MODE_SLAVE, thread_data[conn_id].cb);

    printf ("start %ld ...\n", conn_id);
    struct Dpdk_cryptodev_data_vector job_sess;
    memset(&job_sess, 0, sizeof(struct Dpdk_cryptodev_data_vector));
	job_sess.op._seq = seq++;
	// sess
    job_sess.op._sess_op = SESS_OP_CREATE;
    job_sess.op._cipher_algo = thread_data[conn_id].cipher_algo;
    job_sess.op._cipher_op = thread_data[conn_id].cipher_op;
    job_sess.cipher_key.data = cipher_key;
    job_sess.cipher_key.length = 16;

    // create session
    ciph_agent_send(conn_id, &job_sess, 1);

    // poll and recv created session id
    while(thread_data[conn_id].g_size < 1 )
      ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);

    struct Dpdk_cryptodev_data_vector job;
    memset(&job, 0, sizeof(struct Dpdk_cryptodev_data_vector));
    // sess
    job.op._sess_id = thread_data[conn_id].g_setup_sess_id;
    job.op._sess_op = SESS_OP_ATTACH;
    // data
    job.cipher_buff_list[0].data = ciphertext;
    job.cipher_buff_list[0].length = 64;
    job.cipher_buff_list[1].data = ciphertext + 64;
    job.cipher_buff_list[1].length = 64;
    job.op._op_in_buff_list_len = 2;

    job.cipher_iv.data = iv;
    job.cipher_iv.length = 16;
	// outbuf
	job.op._op_outbuff_ptr = outbuff;
	job.op._op_outbuff_len = MAX_OUTBUFF_LEN;
	
	// warmup
    for (uint32_t i = 0; i < num_pck_per_batch; ++i)
    {
		job.op._seq = seq++;
		res = ciph_agent_send(conn_id, &job, 1);
		if (res == -2) break;
    }
    ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);

    timespec_get (&thread_data[conn_id].start, TIME_UTC);

    int num_batch = num_pck / num_pck_per_batch;
    
	uint32_t pps = thread_data[conn_id].target_pps / 2;
	int32_t send_to_usec = (1000.0 * 1000.0 * num_pck_per_batch) / pps;
	struct timespec start_send, end_send;
	struct timespec start_sleep, end_sleep;

    printf ("run ... %d\n", send_to_usec);
	int32_t to_sleep = 0;
	uint32_t send_total = 0;
    for(int c = 0; c < num_batch; ++c)
    {
		memset (&start_send, 0, sizeof (start_send));
		timespec_get (&start_send, TIME_UTC);

      	for (uint32_t i = 0; i < num_pck_per_batch; ++i)
      	{
		  	job.op._seq = seq++;
        	res = ciph_agent_send(conn_id, &job, 1);
			if (res == -2) break;
      	}

      	res = ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);
		if (res == -2) break;

		memset (&end_send, 0, sizeof (end_send));
        timespec_get (&end_send, TIME_UTC);

		int32_t send_took = (int32_t) get_delta_usec(start_send, end_send);
		send_total += send_took;
		to_sleep += send_to_usec;
		to_sleep -= send_took;
		while (to_sleep > send_to_usec * SLEEP_TO_FACTOR)
		{
			memset (&start_sleep, 0, sizeof (start_sleep));
			timespec_get (&start_sleep, TIME_UTC);

			usleep(send_to_usec * SLEEP_TO_FACTOR);

			memset (&end_sleep, 0, sizeof (end_sleep));
        	timespec_get (&end_sleep, TIME_UTC);

			int32_t slept = (int32_t) get_delta_usec(start_sleep, end_sleep);
			send_total += slept;
			to_sleep -= slept;
		}
    }

    struct Dpdk_cryptodev_data_vector job_sess_del;
    memset(&job_sess_del, 0, sizeof(struct Dpdk_cryptodev_data_vector));
	job_sess_del.op._seq = seq++;
    // sess
    job_sess_del.op._sess_id = thread_data[conn_id].g_setup_sess_id;
    job_sess_del.op._sess_op = SESS_OP_CLOSE;

    // close session
	ciph_agent_send(conn_id, &job_sess_del, 1);

    // flush
    printf ("flush ...\n");
    while(thread_data[conn_id].g_size < num_pck + num_pck_per_batch  + 1  + 1 )
	{
      	int res = ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);
		if (res == -2) break;
	}

	printf ("Seq len: %u\n", thread_data[conn_id].g_size);

	printf ("send_total: %u\n", send_total);

    ciph_agent_conn_free(conn_id);

	return NULL;
}

int main(int argc, char* argv[])
{
	test_memcpy(64);

    long conn_id_0 = 0;
    long conn_id_1 = 1;

    ciph_agent_init();

	thread_data[0].index = conn_id_0;
	thread_data[0].packet_size = 200;
  	thread_data[0].num_pck = 10000000;
	thread_data[0].target_pps = 160000;
  	thread_data[0].num_pck_per_batch = 32;
    memset (&thread_data[0].start, 0, sizeof (thread_data[0].start));
    memset (&thread_data[0].end, 0, sizeof (thread_data[0].end));
	thread_data[0].g_size = 0;
	thread_data[0].g_setup_sess_id = -1;
	thread_data[0].cb = on_job_complete_cb_0;
	thread_data[0].cipher_algo = CRYPTO_CIPHER_AES_CBC;
    thread_data[0].cipher_op = CRYPTO_CIPHER_OP_DECRYPT;
	
	thread_data[1].index = conn_id_1;
	thread_data[1].packet_size = 200;
  	thread_data[1].num_pck = 10000000 / 2;
	thread_data[1].target_pps = 160000 / 2;
  	thread_data[1].num_pck_per_batch = 32;
    memset (&thread_data[1].start, 0, sizeof (thread_data[1].start));
    memset (&thread_data[1].end, 0, sizeof (thread_data[1].end));
	thread_data[1].g_size = 0;
	thread_data[1].g_setup_sess_id = -1;
	thread_data[1].cb = on_job_complete_cb_0;
	thread_data[1].cipher_algo = CRYPTO_CIPHER_AES_CBC;
    thread_data[1].cipher_op = CRYPTO_CIPHER_OP_ENCRYPT;

	//int s;
	void *res;
    pthread_create (&thread[0], NULL, send_proc, (void *)&conn_id_0);
	pthread_create (&thread[1], NULL, send_proc, (void *)&conn_id_1);
	pthread_join(thread[0], &res);
	pthread_join(thread[1], &res);

    ciph_agent_cleanup();

    return 0;
}
