///////////////////////////////////
// ciph_app_test

// TODO to stdafx.h (pch.h)
//#include <assert.h>

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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// libcommon
#include "exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ciph_agent_c.h"
#include <thread>

uint8_t plaintext[64] = {
0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b
};

uint8_t ciphertext[64] = {
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

const int packet_size = 200;
const int num_pck = 10000000;
const int num_pck_per_batch = 32;

const int MAX_CONN_CLIENT_BURST = 64;

int g_size = 0;
int g_data_failed = 0;
int g_op_failed = 0;

struct timespec start, end;

int g_setup_sess_id = -1;

int last_rcv_seq = 0;

void on_job_complete_cb_0 (struct Dpdk_cryptodev_data_vector* vec, uint32_t size)
{
    for(uint32_t j = 0; j < size; ++j)
    {
      	if (vec[j].op._op_status != OP_STATUS_SUCC)
      	{
        	g_op_failed++;
          	continue;
      	}

      	// control plain
      	if (vec[j].op._sess_op == SESS_OP_CREATE)
      	{
        	printf ("session created id: %d\n", vec[j].op._sess_id);
      		g_setup_sess_id = vec[j].op._sess_id;
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
				g_data_failed++;
			}
			else
			{
				memcpy(vec[j].op._op_outbuff_ptr, 
					vec[j].cipher_buff_list[0].data, 
					vec[j].cipher_buff_list[0].length);

				vec[j].op._op_outbuff_len = vec[j].cipher_buff_list[0].length;

				// invalidate memif buffers - will be returned to pool
				vec[j].cipher_buff_list[0].data = NULL;
				vec[j].cipher_buff_list[0].length = 0;
	
				if ( 0 != memcmp(plaintext,
						vec[j].op._op_outbuff_ptr,
						vec[j].op._op_outbuff_len))
          			g_data_failed++;
			}
	  	}
    }

    g_size += size;
    //printf ("Seq len: %u\n", g_size);

    if (g_size == num_pck + num_pck_per_batch + 1 /*sess cr*/ + 1 /*sess del*/)
    {
      timespec_get (&end, TIME_UTC);
      printf ("\n\n");
      printf ("Pakcet sequence finished!\n");
      printf ("Seq len: %u\n", g_size);
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
      double tmp = t1;
      tmp += t2 / 1e+9;
      tmp = g_size / tmp;
      printf ("Average pps: %f\n", tmp);
      printf ("Average TP: %f Mb/s\n", (tmp * packet_size) * 8.0 / 1000000.0);

      printf ("g_data_failed %d\n", g_data_failed);
      printf ("g_op_failed %d\n", g_op_failed);
    }
}

int main(int argc, char* argv[])
{
	uint64_t seq;

    long conn_id = 0;

    memset (&start, 0, sizeof (start));
    memset (&end, 0, sizeof (end));

    ciph_agent_init();

    ciph_agent_conn_alloc(conn_id, CA_MODE_SLAVE, on_job_complete_cb_0);

	while(1)
	{
	g_size = 0;
    struct Dpdk_cryptodev_data_vector job_sess;
    memset(&job_sess, 0, sizeof(struct Dpdk_cryptodev_data_vector));
	job_sess.op._seq = seq++;
	// sess
    job_sess.op._sess_op = SESS_OP_CREATE;
    job_sess.op._cipher_algo = CRYPTO_CIPHER_AES_CBC;
    job_sess.op._cipher_op = CRYPTO_CIPHER_OP_DECRYPT;
    job_sess.cipher_key.data = cipher_key;
    job_sess.cipher_key.length = 16;

    // create session
    ciph_agent_send(conn_id, &job_sess, 1);

    // poll and recv created session id
    while(g_size < 1 /*sess*/)
      ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);

    struct Dpdk_cryptodev_data_vector job;
    memset(&job, 0, sizeof(struct Dpdk_cryptodev_data_vector));
    // sess
    job.op._sess_id = g_setup_sess_id;
    job.op._sess_op = SESS_OP_ATTACH;
    // data
    job.cipher_buff_list[0].data = ciphertext;
    job.cipher_buff_list[0].length = 32;
    job.cipher_buff_list[1].data = ciphertext + 32;
    job.cipher_buff_list[1].length = 32;
    job.op._op_in_buff_list_len = 2;

    job.cipher_iv.data = iv;
    job.cipher_iv.length = 16;
	// outbuf
	job.op._op_outbuff_ptr = outbuff;
	job.op._op_outbuff_len = MAX_OUTBUFF_LEN;
	
	// warmup
    for (int i = 0; i < num_pck_per_batch; ++i)
    {
		job.op._seq = seq++;
		ciph_agent_send(conn_id, &job, 1);
    }
    ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);

    timespec_get (&start, TIME_UTC);

    int num_batch = num_pck / num_pck_per_batch;
    
    printf ("run ...\n");
    for(int c = 0; c < num_batch; ++c)
    {
      	for (int i = 0; i < num_pck_per_batch; ++i)
      	{
		  	job.op._seq = seq++;
        	ciph_agent_send(conn_id, &job, 1);
      	}
      	ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);
    }

    struct Dpdk_cryptodev_data_vector job_sess_del;
    memset(&job_sess_del, 0, sizeof(struct Dpdk_cryptodev_data_vector));
	job_sess_del.op._seq = seq++;
    // sess
    job_sess_del.op._sess_id = g_setup_sess_id;
    job_sess_del.op._sess_op = SESS_OP_CLOSE;

    // close session
	ciph_agent_send(conn_id, &job_sess_del, 1);

    // flush
    printf ("flush ...\n");
    while(g_size < num_pck + num_pck_per_batch  + 1  + 1 )
	{
      	ciph_agent_poll(conn_id, MAX_CONN_CLIENT_BURST);
	}

	printf ("Seq len: %u\n", g_size);
	}

    ciph_agent_conn_free(conn_id);

    ciph_agent_cleanup();

    return 0;
}
