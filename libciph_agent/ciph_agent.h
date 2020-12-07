#include "stdafx.h"

#include <functional>
#include <thread>

#include "memif_client.h"
#include "data_vectors.h"

#include <cstring>

#include "memcpy_fast.h"

//enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

typedef void (*on_job_complete_cb_t) (uint32_t, struct Dpdk_cryptodev_data_vector*, uint32_t);

struct Data_lengths {
    uint32_t ciphertext_length;
    uint32_t cipher_key_length;
    uint32_t cipher_iv_length;
};

void crypto_job_to_buffer(uint8_t* buffer, uint32_t* len, struct Dpdk_cryptodev_data_vector* vec)
{
    *len = 0;

    clib_memcpy_fast(buffer, &vec->op, sizeof(vec->op));
    buffer += sizeof(vec->op);
    *len += sizeof(vec->op);

    struct Data_lengths data_lnn;
    data_lnn.ciphertext_length = 0;
    for (int i = 0; i < vec->op._op_in_buff_list_len; i++)
        data_lnn.ciphertext_length += vec->cipher_buff_list[i].length;

    data_lnn.cipher_key_length = vec->cipher_key.length;
    data_lnn.cipher_iv_length = vec->cipher_iv.length;

    clib_memcpy_fast(buffer, &data_lnn, sizeof(struct Data_lengths));
    buffer += sizeof(struct Data_lengths);
    *len += sizeof(struct Data_lengths);

    for (int i = 0; i < vec->op._op_in_buff_list_len; i++)
    {
        if (vec->cipher_buff_list[i].length)
        {
            clib_memcpy_fast(buffer, vec->cipher_buff_list[i].data, vec->cipher_buff_list[i].length);
            buffer += vec->cipher_buff_list[i].length;
            *len += vec->cipher_buff_list[i].length;
        }
    }

    if (vec->cipher_key.length)
    {
        clib_memcpy_fast(buffer, vec->cipher_key.data, vec->cipher_key.length);
        buffer += vec->cipher_key.length;
        *len += vec->cipher_key.length;
    }

    if (vec->cipher_iv.length)
    {
        clib_memcpy_fast(buffer, vec->cipher_iv.data, vec->cipher_iv.length);
        buffer += vec->cipher_iv.length;
        *len += vec->cipher_iv.length;
    }
}

void crypto_job_from_buffer(uint8_t* buffer, uint32_t len, struct Dpdk_cryptodev_data_vector* vec)
{
    clib_memcpy_fast(&vec->op, buffer, sizeof(vec->op));
    buffer += sizeof(vec->op);

    struct Data_lengths* pData_lnn = (struct Data_lengths*)buffer;
    buffer += sizeof(struct Data_lengths);

    vec->cipher_buff_list[0].data = buffer;
    vec->cipher_buff_list[0].length = pData_lnn->ciphertext_length;
    vec->op._op_in_buff_list_len = 1;

    buffer += pData_lnn->ciphertext_length;
    vec->cipher_key.data = buffer;
    vec->cipher_key.length = pData_lnn->cipher_key_length;

    buffer += vec->cipher_key.length;
    vec->cipher_iv.data = buffer;
    vec->cipher_iv.length = pData_lnn->cipher_iv_length;
}

class Ciph_vec_burst_serializer : public IMsg_burst_serializer
{
public:
    Ciph_vec_burst_serializer(struct Dpdk_cryptodev_data_vector* vecs, uint32_t size)
        : _vecs(vecs), _size(size)
    {}

    inline virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind)
    {
        // TODO ind > size
        crypto_job_to_buffer((uint8_t*) buff, len, &_vecs[ind]);
    }

private:
    struct Dpdk_cryptodev_data_vector* _vecs;
    uint32_t _size;
};

enum { MAX_CONNECTIONS = 20 };

template<class Comm_client>
class Ciph_comm_agent
{
public:
typedef Comm_client Comm_client_t;

public:
    int init();
    int cleanup();

    int conn_alloc(uint32_t index, uint32_t mode, on_job_complete_cb_t cb);
    int conn_free(uint32_t index);

    int send(uint32_t index, struct Dpdk_cryptodev_data_vector* vecs, uint32_t size);
    static void on_recv_cb (uint32_t index, const typename Comm_client::Conn_buffer_t* rx_bufs, uint32_t len);

    int set_rx_mode(uint32_t index, uint32_t qid, char *mode);
    int poll(uint32_t index, uint32_t qid, uint32_t size);

private:
/*
    static void jobs_2_buffs(struct Dpdk_cryptodev_data_vector* vecs, uint32_t size, typename Comm_client::Conn_buffer_t* buffs)
    {
        for(int i = 0; i < size; ++i)
            crypto_job_to_buffer((uint8_t*) buffs[i].data, &buffs[i].len, &vecs[i]);
    }
*/
    static void buffs_2_jobs(struct Dpdk_cryptodev_data_vector* vecs, const typename Comm_client::Conn_buffer_t* buffs, uint32_t buff_size)
    {
        for(int i = 0; i < buff_size; ++i)
            crypto_job_from_buffer((uint8_t*) buffs[i].data, buffs[i].len, &vecs[i]);
    }

private:
    static on_job_complete_cb_t _cb[MAX_CONNECTIONS];
    static struct Dpdk_cryptodev_data_vector* _pool_vecs[MAX_CONNECTIONS];

    Comm_client _client;
};

template<class Comm_client> 
on_job_complete_cb_t Ciph_comm_agent<Comm_client>::_cb[MAX_CONNECTIONS] = { 0 };
template<class Comm_client> 
struct Dpdk_cryptodev_data_vector* Ciph_comm_agent<Comm_client>::_pool_vecs[MAX_CONNECTIONS] = { 0 };

template<class Comm_client> 
static void Ciph_comm_agent<Comm_client>::on_recv_cb (uint32_t index, const typename Comm_client::Conn_buffer_t* rx_bufs, uint32_t len)
{ 
    //TODO check index
    buffs_2_jobs(_pool_vecs[index], rx_bufs, len);

    if (NULL != _cb[index])
        _cb[index](index, _pool_vecs[index], len);
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client> ::init()
{
    for(int i = 0; i < MAX_CONNECTIONS; i++)
    {
        _pool_vecs[i] = (struct Dpdk_cryptodev_data_vector*) malloc(256 * sizeof(struct Dpdk_cryptodev_data_vector));
        // TODO check null
        memset(_pool_vecs[i], 0, 256 * sizeof(struct Dpdk_cryptodev_data_vector));
    }

    return _client.init();
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::cleanup()
{
    return _client.cleanup();
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::conn_alloc(uint32_t index, uint32_t mode, on_job_complete_cb_t cb)
{
    //TODO check index
    // TODO check if already allocated
    int res;

    _cb[index] = cb;

    typename Comm_client::Conn_config_t conn_config;
    conn_config._on_recv_cb_fn = Ciph_comm_agent<Comm_client>::on_recv_cb;
    conn_config._mode = mode;
    res = _client.conn_alloc(index, conn_config);

    return 0;
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::conn_free(uint32_t index)
{
    _cb[index] = NULL;

    return _client.conn_free(index);
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::send(uint32_t index, struct Dpdk_cryptodev_data_vector* vecs, uint32_t size)
{
    //TODO check index
    Ciph_vec_burst_serializer ser(vecs, size);

    return _client.send(index, size, ser);
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::set_rx_mode(uint32_t index, uint32_t qid, char *mode)
{
    return _client.set_rx_mode(index, qid, mode);
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::poll(uint32_t index, uint32_t qid, uint32_t size)
{
    int res =_client.poll(index, qid, size);

    return res;
}

typedef quark::singleton_holder<Ciph_comm_agent<Memif_client> > Ciph_agent_sngl;
