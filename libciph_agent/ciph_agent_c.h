#ifndef _CIPH_AGENT_C_H_
#define _CIPH_AGENT_C_H_

#include "data_vectors.h"

enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

typedef void (*on_job_complete_cb_t) (int, struct Dpdk_cryptodev_data_vector*, uint32_t);

int ciph_agent_init();

int ciph_agent_cleanup();

int ciph_agent_conn_alloc(long index, int mode, on_job_complete_cb_t cb);

int ciph_agent_conn_free(long index);

int ciph_agent_send(long index, struct Dpdk_cryptodev_data_vector* jobs, uint32_t size);

int ciph_agent_poll(long index, uint32_t size);

#endif // _CIPH_AGENT_C_H_
