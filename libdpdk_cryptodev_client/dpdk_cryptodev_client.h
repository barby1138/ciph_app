#ifndef _DPDK_CRYPTODEV_CLIENT_H_
#define _DPDK_CRYPTODEV_CLIENT_H_

#include "options.h"
#include <map>

//1k ue per vnode 2*2 sess per ue
// 6xx = 12k
// 12cc = 24k
class Dpdk_cryptodev_client
{
private:
enum { MAX_SESS_NUM = 2 * 2 * 4000, MAX_CHANNEL_NUM = 4 };

public:
    int init(int argc, char **argv);

    void cleanup();

    int run_jobs(int channel_index, struct Dpdk_cryptodev_data_vector *jobs, uint32_t size);

private:
    int init_inner();

    int verify_devices_capabilities(struct Dpdk_cryptodev_options *options);

    // common mem
    int alloc_common_memory(uint8_t dev_id,  uint16_t qp_id, size_t extra_op_priv_size);

    // ops
    // TODO populate ops depend on action type
    int set_ops_cipher(struct rte_crypto_op **ops, int channel_index, const struct Dpdk_cryptodev_data_vector *test_vectors, uint32_t vec_size);

    int fill_session_pool_socket(int32_t socket_id, uint32_t session_priv_size, uint32_t nb_sessions);

    // jobs
    int vec_output_set(struct rte_crypto_op *op, struct Dpdk_cryptodev_data_vector *vector,
		        // TODO review
		        uint8_t* data);

    void mbuf_set(struct rte_mbuf *mbuf, const struct Dpdk_cryptodev_data_vector *test_vector);

    int run_jobs_inner(int channel_index, uint8_t dev_id, uint8_t qp_id);

    // sessions
    int create_session(int channel_index, uint8_t dev_id, const struct Dpdk_cryptodev_data_vector *test_vector, uint32_t* sess_id);

    int remove_session(int channel_index, uint8_t dev_id, const struct Dpdk_cryptodev_data_vector *test_vector);

    struct rte_cryptodev_sym_session* get_session(int channel_index, uint8_t dev_id, const struct Dpdk_cryptodev_data_vector *test_vector);

private:
    struct Dpdk_cryptodev_options _opts; // = {0};
    uint8_t _enabled_cdevs[RTE_CRYPTO_MAX_DEVS]; // = { 0 };
    int _nb_cryptodevs; // = 0;
    
    // pools are shared between channels
    // pools per socket - we have 1
    struct rte_mempool *_sess_mp;
	struct rte_mempool *_priv_mp;
	struct rte_mempool *_ops_mp;
	struct rte_mempool *_mbuf_mp;
	uint32_t _src_buf_offset;
	uint32_t _dst_buf_offset;

    struct Dpdk_cryptodev_data_vector* t_vecs; // data vec pool
    uint8_t* out_data; // out data buffer pool

    // sess per channel
    struct rte_cryptodev_sym_session* _active_sessions_registry [MAX_CHANNEL_NUM] [MAX_SESS_NUM];
};

#endif // _DPDK_CRYPTODEV_CLIENT_H_