
#include "ciph_agent.h"
#include "ciph_agent_c.h"

#ifdef THR_DBG
const uint32_t MAX_CLIENT_CONN_CNT = 2;
pthread_t g_thread_h[MAX_CLIENT_CONN_CNT];
#endif

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
#ifdef THR_DBG
    g_thread_h[cid] = pthread_self();
#endif

    int32_t res = Ciph_agent_client_sngl::instance().conn_alloc(cid, on_ops_complete_CallBk, NULL, NULL);

    return res;
}

int32_t ciph_agent_poll(uint32_t cid, uint16_t qid, uint32_t size)
{
#ifdef THR_DBG
    pthread_t curr_thread_h = pthread_self();

    if (!pthread_equal(g_thread_h[cid] , curr_thread_h)) 
        printf("WARNING!!! ciph_agent_poll call from other thread\n");
#endif

    int32_t res = Ciph_agent_client_sngl::instance().poll(cid, qid, size);

    return res;
}

int32_t ciph_agent_conn_free(uint32_t index)
{
    return Ciph_agent_client_sngl::instance().conn_free(index);
}

int32_t ciph_agent_send(uint32_t cid, uint16_t qid, const Crypto_operation* ops, uint32_t size)
{
#ifdef THR_DBG
    pthread_t curr_thread_h = pthread_self();

    if (!pthread_equal(g_thread_h[cid] , curr_thread_h)) 
        printf("WARNING!!! ciph_agent_send call from other thread\n");
#endif

    int32_t res = Ciph_agent_client_sngl::instance().send(cid, qid, ops, size);

    return res;
}
