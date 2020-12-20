#ifndef _CIPH_AGENT_C_H_
#define _CIPH_AGENT_C_H_

#include "data_vectors.h"

enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

typedef void (*on_jobs_complete_CallBk_t) (uint32_t, struct Dpdk_cryptodev_data_vector*, uint32_t);

#ifdef __cplusplus
extern "C"
{
#endif
/*
* initializes communication client
*/
int32_t ciph_agent_init();

/*
* frees and cleans up communication client
*/
int32_t ciph_agent_cleanup();

/*
* allocates connection
* id - connection unique id
* cb - call back which is called for batch of completed jobs during polling and in polling context
*/
int32_t ciph_agent_conn_alloc(uint32_t id, uint32_t mode, on_jobs_complete_CallBk_t cb);

/*
* frees connection
* id - connection unique id
*/
int32_t ciph_agent_conn_free(uint32_t id);

/*
* sends batch of jobs on connection
* id - connection unique id
* jobs - pointer to array of jobs to send
* size - jobs number in array
*/
int32_t ciph_agent_send(uint32_t id, struct Dpdk_cryptodev_data_vector* jobs, uint32_t size);

/*
* polls the connection for completed jobs
* id - connection unique id
* size - max size of completed jobs to return
*/
int32_t ciph_agent_poll(uint32_t id, uint32_t size);


#ifdef __cplusplus
}
#endif

#endif // _CIPH_AGENT_C_H_
