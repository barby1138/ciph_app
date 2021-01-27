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

#include <regex>

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

    quark::pstring level = getProperty<quark::pstring>(props::instance(), "properties.logger", "level");
    quark::pstring dpdk_init_str = getProperty<quark::pstring>(props::instance(), "properties.dpdk", "init_str");
    quark::pstring memif_conn_ids = getProperty<quark::pstring>(props::instance(), "properties.memif", "conn_ids");

    //printf("%s\n", level.c_str());
    //printf("%s\n", dpdk_init_str.c_str());
    //printf("%s\n", memif_conn_ids.c_str());

    // DPDK params
    std::string str_dpdk(dpdk_init_str.c_str());
    std::regex regex_dpdk("\\s+");
 
    std::vector<std::string> out_dpdk(
                    std::sregex_token_iterator(str_dpdk.begin(), str_dpdk.end(), regex_dpdk, -1),
                    std::sregex_token_iterator()
                    );

    std::vector<const char*> pchar_dpdk_init_str(out_dpdk.size(), nullptr);
    for (int i = 0; i < out_dpdk.size(); i++)
      pchar_dpdk_init_str[i]= out_dpdk[i].c_str();

    // ids
    std::string str_ids(memif_conn_ids.c_str());
    std::regex regex_ids(",\\s*");
 
    std::vector<std::string> out_ids(
                    std::sregex_token_iterator(str_ids.begin(), str_ids.end(), regex_ids, -1),
                    std::sregex_token_iterator()
                    );

    std::vector<uint32_t> uint_memif_ids(out_ids.size(), 0);
    for (int i = 0; i < out_ids.size(); i++)
    {
      quark::u32 val;
      quark::strings::fromString(quark::pstring(out_ids[i].c_str()), val);
      uint_memif_ids[i] = val;
    }

		custom_tracer::instance().setFile("ciph_app.log");
		custom_tracer::instance().setMask(tlInfo);

		TRACE_INFO("Ver: %s", VERSION);

		Dpdk_cryptodev_client_sngl::instance().init(pchar_dpdk_init_str.size(), &pchar_dpdk_init_str[0]);

    // local test TP
    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_SNOW3G_UEA2, CRYPTO_CIPHER_OP_ENCRYPT);

    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_AES_CBC, CRYPTO_CIPHER_OP_ENCRYPT);

    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_SNOW3G_UEA2, CRYPTO_CIPHER_OP_DECRYPT);

    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_AES_CBC, CRYPTO_CIPHER_OP_DECRYPT);
    
    Ciph_agent_sngl::instance().init();

    for (uint32_t i : uint_memif_ids)
    {
      Ciph_agent_sngl::instance().conn_alloc(i, 1, 
                          //on_job_complete_cb_test_2, 
                          on_job_complete_cb, 
                          on_connect_cb, on_disconnect_cb);
    }

    int res;
    while(1)
    {
      usleep(1000);
/*
      {
      meson::bench_scope_low scope("poll");
*/
      for (uint32_t i : uint_memif_ids)
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

