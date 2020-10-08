///////////////////////////////////
// Ciph_agent

#include "ciph_agent.h"

struct timespec start, end;
int packet_size = 200;
int num_pck = 10000000;
int num_pck_per_batch = 32;

void on_job_complete_cb_0 (struct Dpdk_cryptodev_data_vector* vec, uint32_t size)
{
    static int g_size = 0;
    static int g_failed = 0;
/*
    for(int j = 0; j < size; ++j)
    {
		  if ( 0 != memcmp(plaintext,
					vec[j].ciphertext.data,
					vec[j].ciphertext.length))
          g_failed++;
    }
*/
    g_size += size;
    //printf ("Seq len: %u\n", g_size);
    //if (g_size % 100000 == 0)
    // printf("Con_job_complete_cb %d failed %d\n", g_size, g_failed);

    if (g_size > num_pck)
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

      printf ("failed %d\n", g_failed);
    }
}

int ciph_agent_init()
{
    Ciph_agent_sngl::instance().init();
}

int ciph_agent_cleanup()
{
    Ciph_agent_sngl::instance().cleanup();
}

int ciph_agent_conn_alloc(long index, int mode, on_job_complete_cb_t cb)
{
    Ciph_agent_sngl::instance().conn_alloc(index, mode, cb);

    // TODO wait correctly
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // qit == 0 - we use only 1 q
    Ciph_agent_sngl::instance().set_rx_mode(index, 0, "polling");
}

int ciph_agent_poll(long index)
{
    // qit == 0 - we use only 1 q
    Ciph_agent_sngl::instance().poll(index, 0);
}

int ciph_agent_conn_free(long index)
{
    Ciph_agent_sngl::instance().conn_free(index);
}

int ciph_agent_send(long index, struct Dpdk_cryptodev_data_vector* jobs, uint32_t size)
{
    Ciph_agent_sngl::instance().send(index, jobs, size);
}

int main(int argc, char* argv[])
{
    memset (&start, 0, sizeof (start));
    memset (&end, 0, sizeof (end));

    ciph_agent_init();

    ciph_agent_conn_alloc(0, 0, on_job_complete_cb_0);

    struct Dpdk_cryptodev_data_vector job_sess;
    memset(&job_sess, 0, sizeof(struct Dpdk_cryptodev_data_vector));
    // sess
    job_sess.sess._sess_id = 1;
    job_sess.sess._sess_op = SESS_OP_CREATE;
    job_sess.sess._cipher_algo = RTE_CRYPTO_CIPHER_AES_CBC;
    job_sess.sess._cipher_op = RTE_CRYPTO_CIPHER_OP_DECRYPT;
    job_sess.cipher_key.data = cipher_key;
    job_sess.cipher_key.length = 16;

    // open session
    ciph_agent_send(0, &job_sess, 1);

    struct Dpdk_cryptodev_data_vector job;
    memset(&job, 0, sizeof(struct Dpdk_cryptodev_data_vector));
    // sess
    job.sess._sess_id = 1;
    job.sess._sess_op = SESS_OP_ATTACH;
    // data
    job.ciphertext.data = ciphertext;
    job.ciphertext.length = 64;
    job.cipher_iv.data = iv;
    job.cipher_iv.length = 16;

    // warmup
    for (int i = 0; i < num_pck_per_batch; ++i)
    {
      ciph_agent_send(0, &job, 1);
    }
    
    timespec_get (&start, TIME_UTC);

    int num_batch = num_pck / num_pck_per_batch;
    
    for(int c = 0; c < num_batch; ++c)
    {
      for (int i = 0; i < num_pck_per_batch; ++i)
      {
        ciph_agent_send(0, &job, 1);
      }

      ciph_agent_poll(0);
    }

    // flush
    for(int c = 0; c < 10; ++c)
      ciph_agent_poll(0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ciph_agent_conn_free(0);
    ciph_agent_cleanup();

    return 0;
}
