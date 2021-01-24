// ciph_app.cpp
//

#include "stdafx.h"

#include "config.h"

#include <ctime>
#include <thread>
#include <chrono>
#include <iostream>

#include "memif_client.h"

#include "ciph_agent.h"

#include "dpdk_cryptodev_client.h"

#include <unistd.h> //usleep

typedef tracer
<
thread_formatter<>,
combined_writer<console_writer, file_writer>
> tracer_impl;

typedef quark::singleton_holder
<
tracer_impl,
quark::create_static,
quark::default_lifetime,
quark::class_level_lockable<tracer_impl>
> custom_tracer;

typedef quark::singleton_holder<Dpdk_cryptodev_client> Dpdk_cryptodev_client_sngl;

static bool bound = bindTracer<custom_tracer>();

void usage()
{
  TRACE_INFO("usage: archiver config.xml");
}

void print_buff(uint8_t* data, int len)
{
  fprintf(stdout, "ciph app length %d\n", len);

  int c = 0;
  fprintf(stdout, "%03d ", c++);

  for(int i = 0; i < len; ++i)
  {
    fprintf(stdout, "%02X%s", data[i], ( i + 1 ) % 16 == 0 ? "\r\n" : " " );

    if (( i + 1 ) % 16 == 0)
      fprintf(stdout, "%03d ", c++);
  }

  fprintf(stdout, "\r\n");
}

void on_job_complete_cb (uint32_t cid, uint16_t qid, Crypto_operation* pjob, uint32_t size)
{
  if (0 == size)
    return;

  Dpdk_cryptodev_client_sngl::instance().run_jobs(cid, pjob, size);

  Ciph_agent_sngl::instance().send(cid, qid, pjob, size);
}

void on_job_complete_cb_test_1 (uint32_t cid, uint16_t qid, Crypto_operation* pjob, uint32_t size)
{
  static int cnt = 0;
  cnt++;

  {
  meson::bench_scope_low scope("on_job_complete_cb");

  {
  meson::bench_scope_low scope("run_jobs");

  Dpdk_cryptodev_client_sngl::instance().run_jobs(cid, pjob, size);
  }

  //printf ("APP cid %d qid %d %d\n", cid, qid, size);
  //usleep(500);

  {
  meson::bench_scope_low scope("send");

  Ciph_agent_sngl::instance().send(cid, qid, pjob, size);
  }

  }

  if (cnt % 10000 == 0)
  {
	  TRACE_INFO("%s", profiler_low::instance().dump().c_str());
  }
}

void on_job_complete_cb_test_2 (uint32_t cid, uint16_t qid, Crypto_operation* pjob, uint32_t size)
{
  if (0 == size)
    return;

  if (qid == 0)
  {
    Dpdk_cryptodev_client_sngl::instance().run_jobs_test(cid, pjob, size);
    //Dpdk_cryptodev_client_sngl::instance().run_jobs(cid, pjob, size);
    Ciph_agent_sngl::instance().send(cid, qid, pjob, size);
  }

  if (qid == 1)
  {
    Dpdk_cryptodev_client_sngl::instance().run_jobs(cid, pjob, size);
    Ciph_agent_sngl::instance().send(cid, qid, pjob, size);
  }
  
}


void on_connect_cb (uint32_t cid)
{
  TRACE_INFO("on_connect_cb %d", cid);
}

void on_disconnect_cb (uint32_t cid, uint16_t qid, Crypto_operation* pjob, uint32_t size)
{
  TRACE_INFO("on_disconnect_cb %d", cid);

  Dpdk_cryptodev_client_sngl::instance().cleanup_conn(cid);
}

int main(int argc, char** argv)
{
	try
	{
		if (argc < 2)
		{
			usage();
			return 1;
		}

		props::instance().load("ciph_app.xml");

		custom_tracer::instance().setFile("ciph_app.log");
		custom_tracer::instance().setMask(tlInfo);

		TRACE_INFO("Ver: %s", VERSION);

	  char* v[] = { "app",
					"--vdev", 
					"crypto_aesni_mb_pmd", 
          "--vdev", 
          "crypto_snow3g",
					//"--no-huge", 
					"--"
					};
    int c = 6;

		Dpdk_cryptodev_client_sngl::instance().init(c, v);

    // local test TP
    for (int i = 0; i < 5; ++i)
    {
      // local test
      Dpdk_cryptodev_client_sngl::instance().test();
    }

    Ciph_agent_sngl::instance().init();

    for (int i = 0; i < 6; ++i)
    {
      Ciph_agent_sngl::instance().conn_alloc(i, 1, 
                          //on_job_complete_cb_test_2, 
                          on_job_complete_cb, 
                          on_connect_cb, on_disconnect_cb);
    }

    int res;
    while(1)
    {
      usleep(100);
/*
      {
      meson::bench_scope_low scope("poll");
*/
      for (int i = 0; i < 6; ++i)
      {
        res = Ciph_agent_sngl::instance().poll_00(i, 0, 64);     
        res = Ciph_agent_sngl::instance().poll_00(i, 1, 64);  
      }   
//      }
    }

    Ciph_agent_sngl::instance().conn_free(0);
    Ciph_agent_sngl::instance().conn_free(1);

    Ciph_agent_sngl::instance().cleanup();

    Dpdk_cryptodev_client_sngl::instance().cleanup();

		props::instance().save();
	}
	catch (std::exception& e)
	{
		TRACE_ERROR("Exception: %s", e.what());
	}

}

