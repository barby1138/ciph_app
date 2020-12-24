///////////////////////////////////
// Ciph_agent
#include "ciph_agent.h"
#include "ciph_agent_c.h"

int32_t ciph_agent_init()
{
    return Ciph_agent_sngl::instance().init();
}

int32_t ciph_agent_cleanup()
{
    return Ciph_agent_sngl::instance().cleanup();
}

int32_t ciph_agent_conn_alloc(uint32_t index, uint32_t mode, on_ops_complete_CallBk_t cb)
{
    int32_t res = Ciph_agent_sngl::instance().conn_alloc(index, mode, cb);

    return res;
}

int32_t ciph_agent_poll(uint32_t index, uint32_t size)
{
    // qit == 0 - we use only 1 q
    int32_t res = Ciph_agent_sngl::instance().poll(index, 0, size);

    return res;
}

int32_t ciph_agent_conn_free(uint32_t index)
{
    return Ciph_agent_sngl::instance().conn_free(index);
}

int32_t ciph_agent_send(uint32_t index, Crypto_operation* jobs, uint32_t size)
{
    int32_t res = Ciph_agent_sngl::instance().send(index, jobs, size);

    return res;
}
