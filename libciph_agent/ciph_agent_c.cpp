
#include "ciph_agent.h"
#include "ciph_agent_c.h"

int32_t ciph_agent_init(uint32_t client_id)
{
    return Ciph_agent_client_sngl::instance().init(client_id);
}

int32_t ciph_agent_cleanup()
{
    return Ciph_agent_client_sngl::instance().cleanup();
}

int32_t ciph_agent_conn_alloc(uint32_t cid, on_ops_complete_CallBk_t on_ops_complete_CallBk)
{
    int32_t res = Ciph_agent_client_sngl::instance().conn_alloc(cid, on_ops_complete_CallBk, NULL, NULL);

    return res;
}

int32_t ciph_agent_poll(uint32_t cid, uint16_t qid, uint32_t size)
{
    int32_t res = Ciph_agent_client_sngl::instance().poll(cid, qid, size);

    return res;
}

int32_t ciph_agent_conn_free(uint32_t index)
{
    return Ciph_agent_client_sngl::instance().conn_free(index);
}

int32_t ciph_agent_send(uint32_t cid, uint16_t qid, const Crypto_operation* ops, uint32_t size)
{
    int32_t res = Ciph_agent_client_sngl::instance().send(cid, qid, ops, size);

    return res;
}
