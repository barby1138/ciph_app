// ciph_app.cpp
//

#include "stdafx.h"

#include <ctime>
#include <thread>
#include <chrono>
#include <iostream>
#include <unistd.h> //usleep
#include <sys/stat.h> //mkdir
// date time
#include <ctime>
#include <iostream>
#include <locale>

#include "config.h"
#include "memif_client.h"
#include "ciph_agent.h"
#include "dpdk_cryptodev_client.h"

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
  printf("ciph app length %d\n", len);

  int c = 0;
  printf("%03d ", c++);

  for(int i = 0; i < len; ++i)
  {
    printf("%02X%s", data[i], ( i + 1 ) % 16 == 0 ? "\r\n" : " " );

    if (( i + 1 ) % 16 == 0)
      printf("%03d ", c++);
  }

  printf("\r\n");
}

void on_job_complete_cb (uint32_t cid, uint16_t qid, Crypto_operation* pjob, uint32_t size)
{
  if (0 == size)
    return;

  Dpdk_cryptodev_client_sngl::instance().run_jobs(cid, pjob, size);

  //Ciph_agent_server_sngl::instance().send(cid, qid, pjob, size);
}

void on_job_complete_cb_test_1 (uint32_t cid, uint16_t qid, Crypto_operation* pjob, uint32_t size)
{
  if (0 == size)
    return;

  static int cnt = 0;
  cnt++;

  {
  meson::bench_scope_low scope("on_job_complete_cb");

  Dpdk_cryptodev_client_sngl::instance().run_jobs(cid, pjob, size);

  //printf ("APP cid %d qid %d %d\n", cid, qid, size);
  //usleep(500);
  }

  if (cnt % 10000 == 0)
  {
	  TRACE_INFO("%s", profiler_low::instance().dump().c_str());
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

std::string date_time_str()
{
    //std::locale::global(std::locale("ja_JP.utf8"));
    std::time_t t = std::time(nullptr);
    char mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "%F_%R", std::localtime(&t))) 
    {
      return mbstr;
    }
    else
    {
      return "nodate";
    }
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

    enum { MAX_STATS_DUMP_TO_SEC = 6000, MIN_STATS_DUMP_TO_SEC = 10 };

    int mdret;
    mode_t mode = 0755;

    if ((mdret = mkdir("/var/ciph_app", mode)) && errno != EEXIST)
    {
      throw std::runtime_error(quark::strings::format("mkdir failed errno %d ret %d", errno, mdret).c_str());
    }

    //TRACE_INFO ("init mkdir OK: errno %d ret %d", errno, mdret);

		props::instance().load("ciph_app.xml");

    quark::pstring level = getProperty<quark::pstring>(props::instance(), "properties.logger", "level");
    quark::pstring path = getProperty<quark::pstring>(props::instance(), "properties.logger", "path");
    quark::pstring dpdk_init_str = getProperty<quark::pstring>(props::instance(), "properties.dpdk", "init_str");
    quark::pstring memif_conn_ids = getProperty<quark::pstring>(props::instance(), "properties.memif", "conn_ids");
    quark::u32 stats_dump_to_sec = getProperty<quark::u32>(props::instance(), "properties.stats", "dump_to_sec");

    if (stats_dump_to_sec < MIN_STATS_DUMP_TO_SEC || stats_dump_to_sec > MAX_STATS_DUMP_TO_SEC)
    {
      TRACE_WARNING ("stats_dump_to_sec out of range [%u,%u] %u - use default 60", 
                MIN_STATS_DUMP_TO_SEC,
                MAX_STATS_DUMP_TO_SEC,
                stats_dump_to_sec);
      stats_dump_to_sec = 60;
    }

    uint64_t i = 0;
    //uint32_t stats_dump_to_sec = 10;
    //printf("%s\n", level.c_str());
    //printf("%s\n", dpdk_init_str.c_str());
    //printf("%s\n", memif_conn_ids.c_str());

    // DPDK params
    std::string str_dpdk(dpdk_init_str.c_str());

    enum { MAX_PARAMS_CNT = 100 };
    std::vector<quark::pstring> out_dpdk(MAX_PARAMS_CNT);
    quark::strings::split(dpdk_init_str, " ", out_dpdk.begin());

    std::vector<const char*> pchar_dpdk_init_str;
    for (int i = 0; i < out_dpdk.size(); i++)
    {
      if (!out_dpdk[i].empty())
      {
        //printf("%s\n", out_dpdk[i].c_str());
        pchar_dpdk_init_str.push_back(out_dpdk[i].c_str());;
      }
    }

    // ids
    std::vector<quark::pstring> out_ids(MAX_PARAMS_CNT);
    quark::strings::split(memif_conn_ids, ",", out_ids.begin());

    std::vector<uint32_t> uint_memif_ids;
    std::vector<uint32_t> uint_client_ids;
    for (int i = 0; i < out_ids.size(); i++)
    {
      if (!out_ids[i].empty())
      {
        quark::u32 val;
        quark::strings::fromString(quark::pstring(out_ids[i].c_str()), val);
        //printf("%d\n", val);
        uint_client_ids.push_back(val);
      }
    }

    std::string log_file_name(quark::strings::format("ciph_app_%s.log", date_time_str().c_str()).c_str());
    std::string log_file_full_name(quark::strings::format("%s/%s", path.c_str(), log_file_name.c_str()).c_str());

    TRACE_INFO ("Log %s", log_file_full_name.c_str());

		custom_tracer::instance().setFile(log_file_full_name.c_str());

    uint32_t tl = tlWarning;
    if (level.compare("info") == 0)
      tl = tlInfo;
    if (level.compare("debug") == 0)
      tl = tlDebug;

    custom_tracer::instance().setMask(tl);

		TRACE_INFO("Ver: %s", VERSION);

		Dpdk_cryptodev_client_sngl::instance().init(pchar_dpdk_init_str.size(), &pchar_dpdk_init_str[0]);
    Dpdk_cryptodev_client_sngl::instance().set_print_dbg((tl == tlDebug) ? 1 : 0) ;
    
    // local test TP
    /*
    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_SNOW3G_UEA2, CRYPTO_CIPHER_OP_ENCRYPT);

    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_AES_CBC, CRYPTO_CIPHER_OP_ENCRYPT);

    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_SNOW3G_UEA2, CRYPTO_CIPHER_OP_DECRYPT);

    Dpdk_cryptodev_client_sngl::instance().test(CRYPTO_CIPHER_AES_CBC, CRYPTO_CIPHER_OP_DECRYPT);
*/    
    Ciph_agent_server_sngl::instance().init(-1);

    for (uint32_t i : uint_client_ids)
    {
      uint_memif_ids.push_back(i*2);
      uint_memif_ids.push_back(i*2 + 1);
    }

    for (uint32_t i : uint_memif_ids)
    {
      TRACE_INFO("conn_alloc id %d", i);

      Ciph_agent_server_sngl::instance().conn_alloc(i, 
                          //on_job_complete_cb_test_2, 
                          on_job_complete_cb, 
                          on_connect_cb, 
                          on_disconnect_cb);
    }

    int res;
    while(1)
    {
      usleep(100);

      Ciph_agent_server_sngl::instance().poll_all(64);

      if (++i % ( 10000 * stats_dump_to_sec ) == 0)
        Dpdk_cryptodev_client_sngl::instance().print_stats();
    }

    // following never ecec

    for (uint32_t i : uint_memif_ids)
    {
      TRACE_INFO("conn_free id %d", i);

      Ciph_agent_server_sngl::instance().conn_free(i);
    }
    
    Ciph_agent_server_sngl::instance().cleanup();

    Dpdk_cryptodev_client_sngl::instance().cleanup();

		props::instance().save();
	}
	catch (std::exception& e)
	{
		TRACE_ERROR("Exception: %s", e.what());
	}
}

