#ifndef _CIPH_AGENT_C_H_
#define _CIPH_AGENT_C_H_

#include "data_vectors.h"

enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

typedef void (*on_job_complete_cb_t) (int, struct Dpdk_cryptodev_data_vector*, uint32_t);

/*
* initializes communication client
*/
int ciph_agent_init();

/*
* frees and cleans up communication client
*/
int ciph_agent_cleanup();

/*
* allocates connection
* index - connection index
* cb - call back which is called for batch of completed jobs during polling and in polling context
*/
int ciph_agent_conn_alloc(long index, int mode, on_job_complete_cb_t cb);

/*
* frees connection
* index - connection index
*/
int ciph_agent_conn_free(long index);

/*
* sends batch of jobs on connection
* index - connection index
* jobs - pointer to array of jobs to send
* size - jobs number in array
*/
int ciph_agent_send(long index, struct Dpdk_cryptodev_data_vector* jobs, uint32_t size);

/*
* polls the connection for completed jobs
* index - connection index
* size - max size of completed jobs to return
*/
int ciph_agent_poll(long index, uint32_t size);

#endif // _CIPH_AGENT_C_H_
