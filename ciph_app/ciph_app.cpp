// ciph_app.cpp
//

#include "stdafx.h"

#include "config.h"

#include <ctime>
#include <thread>
#include <chrono>
#include <iostream>

#include "dpdk_cryptodev_client.h"
#include "memif_client.h"

#include "ciph_agent.h"

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

struct Dpdk_cryptodev_data_vector g_job[2][100000];
uint32_t g_size[2];
uint32_t g_index[2] = { 0 };
uint32_t g_tot_size[2] = { 0 };
uint32_t g_tot_size_1[2] = { 0 };

void on_job_complete_cb (uint32_t index, struct Dpdk_cryptodev_data_vector* pjob, uint32_t size)
{
  Dpdk_cryptodev_client_sngl::instance().run_jobs(index, pjob, size);
  
  //for(int j = 0; j < size; ++j)
    //printf("id %d\n", pjob[j].op._sess_op);
    //print_buff(pjob[j].cipher_buff_list[0].data, pjob[j].cipher_buff_list[0].length);

  Ciph_agent_sngl::instance().send(index, pjob, size);
  /*
  for (int i = 0; i < size; i++)
  {
    memcpy(&g_job[index][g_index[index] + i], &pjob[i], sizeof(struct Dpdk_cryptodev_data_vector));
  }
  g_size[index] += size;
  g_index[index] += size;

  printf("g_index %d %d\n", g_index[0], g_index[1]);
  */
  //g_tot_size_1[index] += g_size[index];    
  //printf("g_tot_size_1 %d %d\n", g_tot_size_1[0], g_tot_size_1[1]);
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
		custom_tracer::instance().setMask(tlDebug);

		bench_scope_low scope("ciph_app main");

	  char* v[] = { "app",
					"--vdev", 
					"crypto_aesni_mb_pmd", 
          "--vdev", 
          "crypto_snow3g",
					"--no-huge", 
					"--",
					//"--devtype",
					//"crypto_aesni_mb",
					//"--devtype",
					//"crypto_snow3g",
          "--buffer-sz",
          "64"
					};
    int c = 9;

		Dpdk_cryptodev_client_sngl::instance().init(c, v);
    
    Ciph_agent_sngl::instance().init();

    Ciph_agent_sngl::instance().conn_alloc(0, 1, on_job_complete_cb);
    Ciph_agent_sngl::instance().conn_alloc(1, 1, on_job_complete_cb);

    int res;
    while(1)
    {
        usleep(1000);
        res = Ciph_agent_sngl::instance().poll(0, 0, 64);
        /*
        if (res == 0 && g_size[0] > 0)
        {
          g_tot_size[0] += g_size[0];
          printf("g_size 0 %d\n", g_size[0]);
          Ciph_agent_sngl::instance().send(0, g_job[0], g_size[0]);
          g_size[0] = 0;
          g_index[0] = 0;
        }
*/
        res = Ciph_agent_sngl::instance().poll(1, 0, 64);     
        /*
        if (res == 0 && g_size[1] > 0)
        {
          g_tot_size[1] += g_size[1];
          printf("g_size 1 %d\n", g_size[1]);
          Ciph_agent_sngl::instance().send(1, g_job[1], g_size[1]);
          g_size[1] = 0;
          g_index[1] = 0;
        }
        */
        //printf("g_tot_size %d %d\n", g_tot_size[0], g_tot_size[1]);
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

