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

void on_job_complete_cb (uint32_t index, Crypto_operation* pjob, uint32_t size)
{
  Dpdk_cryptodev_client_sngl::instance().run_jobs(index, pjob, size);

  //usleep(500);
  Ciph_agent_sngl::instance().send(index, pjob, size);
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

		custom_tracer::instance().setFile(".ciph_app.log");
		custom_tracer::instance().setMask(tlInfo);

		TRACE_INFO("Ver: %s", VERSION);

		bench_scope_low scope("ciph_app main");

	  char* v[] = { "app",
					"--vdev", 
					"crypto_aesni_mb_pmd", 
          "--vdev", 
          "crypto_snow3g",
					//"--no-huge", 
					"--",
					//"--devtype",
					//"crypto_aesni_mb",
					//"--devtype",
					//"crypto_snow3g",
          "--buffer-sz",
          "1024"
					};
    int c = 8;

		Dpdk_cryptodev_client_sngl::instance().init(c, v);
    
    Ciph_agent_sngl::instance().init();

    Ciph_agent_sngl::instance().conn_alloc(0, 1, on_job_complete_cb);
    Ciph_agent_sngl::instance().conn_alloc(1, 1, on_job_complete_cb);

    int res;
    while(1)
    {
        usleep(100);
        res = Ciph_agent_sngl::instance().poll(0, 0, 64);
        res = Ciph_agent_sngl::instance().poll(1, 0, 64);     
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

	TRACE_INFO("%s", profiler_low::instance().dump().c_str());
}

