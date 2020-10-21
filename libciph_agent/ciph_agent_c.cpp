///////////////////////////////////
// Ciph_agent

#include "ciph_agent.h"
#include "ciph_agent_c.h"

int ciph_agent_init()
{
    return Ciph_agent_sngl::instance().init();
}

int ciph_agent_cleanup()
{
    return Ciph_agent_sngl::instance().cleanup();
}

int ciph_agent_conn_alloc(long index, int mode, on_job_complete_cb_t cb)
{
    int res = Ciph_agent_sngl::instance().conn_alloc(index, mode, cb);

    return res;
}

int ciph_agent_poll(long index, uint32_t size)
{
    // qit == 0 - we use only 1 q
    return Ciph_agent_sngl::instance().poll(index, 0, size);
}

int ciph_agent_conn_free(long index)
{
    return Ciph_agent_sngl::instance().conn_free(index);
}

int ciph_agent_send(long index, struct Dpdk_cryptodev_data_vector* jobs, uint32_t size)
{
    int res = Ciph_agent_sngl::instance().send(index, jobs, size);
    if (0 != res)
      	printf("send error agent\n");
    return res;
}
