#ifndef _DPDK_CRYPTODEV_CLIENT_H_
#define _DPDK_CRYPTODEV_CLIENT_H_

#include "options.h"

//1k ue per vnode 2*2 sess per ue
// 6xx = 12k
// 12cc = 24k
class Dpdk_cryptodev_client
{
private:
enum { MAX_SESS_NUM = 2 * 2 * 4000, MAX_CHANNEL_NUM = 32, MAX_DEV_NUM = 2 };

public:
    int init(int argc, char **argv);

    void cleanup();

    int run_jobs(int ch_id, Crypto_operation* jobs, uint32_t size);

private:
    int init_inner();

    int verify_devices_capabilities(Dpdk_cryptodev_options* options);

    // common mem
    int alloc_common_memory(uint8_t dev_id,  uint16_t qp_id, size_t extra_op_priv_size);

    int fill_session_pool_socket(int32_t socket_id, uint32_t session_priv_size, uint32_t nb_sessions);

    // ops
    int set_ops_cipher(rte_crypto_op **ops, int ch_id, const Crypto_operation* vecs, uint32_t vecs_size);

    int run_jobs_inner(int ch_id, uint8_t dev_id, uint8_t qp_id, const Crypto_operation* vecs, uint32_t vecs_size);

    // sessions
    int create_session(int ch_id, uint8_t dev_id, const Crypto_operation& vec, uint32_t* sess_id);

    int remove_session(int ch_id, uint8_t dev_id, const Crypto_operation& vec);

    rte_cryptodev_sym_session* get_session(int ch_id, uint8_t dev_id, const Crypto_operation& vec);

private:
    Dpdk_cryptodev_options _opts; // = {0};
    uint8_t _enabled_cdevs[RTE_CRYPTO_MAX_DEVS]; // = { 0 };
    int _nb_cryptodevs; // = 0;
    
    // pools are shared between channels
    // pools per socket - we have 1
    rte_mempool *_sess_mp;
	rte_mempool *_priv_mp;
	rte_mempool *_ops_mp;
	uint32_t _src_buf_offset;
	uint32_t _dst_buf_offset;

    //Crypto_operation* t_vecs; // data vec pool

    // sess per channel
    rte_cryptodev_sym_session* _active_sessions_registry [MAX_CHANNEL_NUM][MAX_DEV_NUM][MAX_SESS_NUM];
};

#endif // _DPDK_CRYPTODEV_CLIENT_H_