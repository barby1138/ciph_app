#ifndef _CIPH_AGENT_C_H_
#define _CIPH_AGENT_C_H_

#include "data_vectors.h"

typedef void (*on_ops_complete_CallBk_t) (uint32_t, uint16_t, Crypto_operation*, uint32_t);

#ifdef __cplusplus
extern "C"
{
#endif
/*
* initializes communication client
*/
int32_t ciph_agent_init(uint32_t client_id);

/*
* frees and cleans up communication client
*/
int32_t ciph_agent_cleanup();

/*
* allocates connection
* cid - connection unique id
* cb - call back which is called for batch of completed jobs during polling and in polling context
*/
int32_t ciph_agent_conn_alloc(uint32_t cid, on_ops_complete_CallBk_t cb);

/*
* frees connection
* cid - connection unique id
*/
int32_t ciph_agent_conn_free(uint32_t cid);

/*
* sends batch of jobs on connection
* cid - connection unique id
* qid - queue unique id
* ops - pointer to array of crypto ops to send
* size - jobs number in array
*/
int32_t ciph_agent_send(uint32_t cid, uint16_t qid, const Crypto_operation* ops, uint32_t size);

/*
* polls the connection for completed jobs
* cid - connection unique id
* qid - queue unique id
* size - max size of completed jobs to return
*/
int32_t ciph_agent_poll(uint32_t cid, uint16_t qid, uint32_t size);


#ifdef __cplusplus
}
#endif

#endif // _CIPH_AGENT_C_H_
