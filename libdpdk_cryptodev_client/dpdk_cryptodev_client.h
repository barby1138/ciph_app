#ifndef _DPDK_CRYPTODEV_CLIENT_H_
#define _DPDK_CRYPTODEV_CLIENT_H_

#include "options.h"

//1k ue per vnode 2*2 sess per ue
// 6xx = 12k
// 12cc = 24k

struct Dpdk_cryptodev_device
{
    // TODO algo arr
    // needed only for spec algo rte op settings ( Ex. len << 3 )
    uint32_t algo;
    uint8_t cdev_id;
};

enum { MAX_JOBS_BURST_SIZE = 64 };
struct Dev_vecs_idxs_t
{
	uint32_t dev_vecs_idxs[MAX_JOBS_BURST_SIZE];
};


struct Conn_stats
{
    uint32_t conn_id;
    uint32_t sess_count;
};

struct Dpdk_cryptodev_client_stats
{
    uint64_t run_jobs_cnt;

    // crypto stats
    uint64_t op_processed;

    uint64_t op_failed_total;

    uint64_t op_failed_create_sess;

    // SAME?
    uint64_t op_failed_get_sess;
    uint64_t op_failed_invalid_cdev;

    uint64_t op_failed_remove_session;

    uint64_t op_failed_in_agent;

    uint64_t op_failed_op_type_unk;

    uint64_t op_failed_to_cipher;

    uint64_t op_failed_after_cipher_not_proc;

    uint32_t burst_avg_size;

    uint32_t pck_avg_size;
    
    uint32_t avg_pps;

    // general stats
    //std::string time_up;

    // memif stats?
    Conn_stats conn_stats[20];
    uint32_t active_conn_count;

// private
    uint32_t burst_pck_total_size;
    uint32_t burst_total_size;
};

class Dpdk_cryptodev_client
{
private:
enum { MAX_SESS_NUM = 2 * 2 * 4000, MAX_CHANNEL_NUM = 32, MAX_DEV_NUM = 2 };

public:
    int init(int argc, char **argv);

    void cleanup();

    int run_jobs(int ch_id, Crypto_operation* jobs, uint32_t size);

    int init_conn(int ch_id);

    int cleanup_conn(int ch_id);

    int set_print_dbg(int do_enable)
    { _print_dbg = (do_enable == 0) ? 0 : 1 ; }

    void print_stats();

private:
    int init_inner();

    int verify_devices_capabilities(Dpdk_cryptodev_options* options);

    // common mem
    int alloc_common_memory(uint8_t dev_id,  uint16_t qp_id, size_t extra_op_priv_size);

    int fill_session_pool_socket(int32_t socket_id, uint32_t session_priv_size, uint32_t nb_sessions);

    // process
    int preprocess_jobs(int ch_id, 
                        Crypto_operation* jobs, 
                        uint32_t size, 
                        uint32_t* dev_ops_size_arr, 
                        Dev_vecs_idxs_t* dev_vecs_idxs_arr);

    int postprocess_jobs(int ch_id, Crypto_operation* jobs, uint32_t size);

    int run_jobs_inner(int ch_id, 
                        uint8_t dev_id, 
                        uint8_t qp_id, 
                        Crypto_operation* vecs, 
                        uint32_t dev_vecs_size, 
                        uint32_t* dev_vecs_idxs);

    int set_ops_cipher(rte_crypto_op **ops, 
                        int ch_id, 
                        uint8_t dev_id, 
                        Crypto_operation* vecs, 
                        uint32_t ops_nb, 
                        uint32_t* dev_vecs_idxs);

    int set_ops_cipher_result(	rte_crypto_op **ops, // processed
						Crypto_operation* vecs, 
					    uint32_t ops_nb, // deqd
						uint32_t* dev_vecs_idxs,
					    uint32_t ops_deqd_total // deqd before
                        );

    // sessions
    int create_session(int ch_id, const Crypto_operation& vec, uint32_t* sess_id);

    int remove_session(int ch_id, const Crypto_operation& vec);

    int remove_all_sessions(int ch_id);

    rte_cryptodev_sym_session* get_session(int ch_id,  const Crypto_operation& vec);

// tests
public:
    int test(Crypto_cipher_algorithm algo, Crypto_cipher_operation op_type);
    int run_jobs_test(int ch_id, Crypto_operation* jobs, uint32_t size);
private:
    int32_t test_create_session(long cid, uint64_t seq, Crypto_cipher_algorithm algo, Crypto_cipher_operation op_type);
    int32_t test_cipher(long cid, uint64_t seq, uint64_t sess_id, Crypto_cipher_operation op_type);
    int32_t test_cipher_burst(long cid, uint64_t seq, uint64_t sess_id, Crypto_cipher_operation op_type);

private:
    Dpdk_cryptodev_options _opts; // = {0};
    Dpdk_cryptodev_device _enabled_cdevs[RTE_CRYPTO_MAX_DEVS]; // = { 0 };
    int _nb_cryptodevs; // = 0;
    
    // pools are shared between channels
    // pools per socket - we have 1
    rte_mempool *_sess_mp;
	rte_mempool *_priv_mp;
	rte_mempool *_ops_mp;
	uint32_t _src_buf_offset;
	uint32_t _dst_buf_offset;

    // sess per channel
    rte_cryptodev_sym_session* _active_sessions_registry [MAX_CHANNEL_NUM][MAX_DEV_NUM][MAX_SESS_NUM];
    
    int _print_dbg;

    Dpdk_cryptodev_client_stats _stats;
};

#endif // _DPDK_CRYPTODEV_CLIENT_H_