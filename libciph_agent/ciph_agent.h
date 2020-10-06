#include "stdafx.h"

#include <functional>
#include <thread>

#include "memif_client.h"
#include "data_vectors.h"

#include <cstring>

// test data

uint8_t plaintext[64] = {
0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b
};

/* cipher text */
uint8_t ciphertext[64] = {
0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55
};

/* iv */
uint8_t iv[] = {
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

/* cipher key */
uint8_t cipher_key[] = {
0xE4, 0x23, 0x33, 0x8A, 0x35, 0x64, 0x61, 0xE2, 0x49, 0x03, 0xDD, 0xC6, 0xB8, 0xCA, 0x55, 0x7A
};

struct Data_lengths {
    uint32_t ciphertext_length;
    uint32_t cipher_key_length;
    uint32_t cipher_iv_length;
};

void crypto_job_to_buffer(uint8_t* buffer, uint32_t* len, struct Dpdk_cryptodev_data_vector* vec)
{
    *len = 0;

    memcpy(buffer, &vec->sess, sizeof(vec->sess));
    buffer += sizeof(vec->sess);
    *len += sizeof(vec->sess);

    struct Data_lengths data_lnn;
    data_lnn.ciphertext_length = vec->ciphertext.length;
    data_lnn.cipher_key_length = vec->cipher_key.length;
    data_lnn.cipher_iv_length = vec->cipher_iv.length;

    memcpy(buffer, &data_lnn, sizeof(struct Data_lengths));
    buffer += sizeof(struct Data_lengths);
    *len += sizeof(struct Data_lengths);

    if (vec->ciphertext.length)
    {
        memcpy(buffer, vec->ciphertext.data, vec->ciphertext.length);
        buffer += vec->ciphertext.length;
        *len += vec->ciphertext.length;
    }
    
    if (vec->cipher_key.length)
    {
        memcpy(buffer, vec->cipher_key.data, vec->cipher_key.length);
        buffer += vec->cipher_key.length;
        *len += vec->cipher_key.length;
    }

    memcpy(buffer, vec->cipher_iv.data, vec->cipher_iv.length);
    buffer += vec->cipher_iv.length;
    *len += vec->cipher_iv.length;
}

void crypto_job_from_buffer(uint8_t* buffer, uint32_t len, struct Dpdk_cryptodev_data_vector* vec)
{
    memcpy(&vec->sess, buffer, sizeof(vec->sess));
    buffer += sizeof(vec->sess);

    struct Data_lengths* pData_lnn = (struct Data_lengths*)buffer;
    buffer += sizeof(struct Data_lengths);

    vec->ciphertext.data = buffer;
    vec->ciphertext.length = pData_lnn->ciphertext_length;

    buffer += vec->ciphertext.length;
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

    virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind)
    {
        // TODO ind > size
        crypto_job_to_buffer((uint8_t*) buff, len, &_vecs[ind]);
    }

private:
    struct Dpdk_cryptodev_data_vector* _vecs;
    uint32_t _size;
};

enum { MAX_CONNECTIONS = 2 };

typedef void (*on_job_complete_cb_t) (struct Dpdk_cryptodev_data_vector*, uint32_t);

template<class Comm_client>
class Ciph_comm_agent
{
public:
typedef Comm_client Comm_client_t;

public:
    int init();
    int cleanup();

    int conn_alloc(long index, int mode, on_job_complete_cb_t cb);
    int conn_free(long index);

    int send(long index, struct Dpdk_cryptodev_data_vector* vecs, uint32_t size);
    static void on_recv_cb (long index, const typename Comm_client::Conn_buffer_t* rx_bufs, uint32_t len);

    int set_rx_mode(long index, long qid, char *mode);
    int poll(long index, long qid);

private:
    static void jobs_2_buffs(struct Dpdk_cryptodev_data_vector* vecs, uint32_t size, typename Comm_client::Conn_buffer_t* buffs)
    {
        for(int i = 0; i < size; ++i)
            crypto_job_to_buffer((uint8_t*) buffs[i].data, &buffs[i].len, &vecs[i]);
    }

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
static void Ciph_comm_agent<Comm_client>::on_recv_cb (long index, const typename Comm_client::Conn_buffer_t* rx_bufs, uint32_t len)
{ 
    //TODO check index
    buffs_2_jobs(_pool_vecs[index], rx_bufs, len);

    if (NULL != _cb[index])
        _cb[index](_pool_vecs[index], len);
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

    _client.init();

    return 0;
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::cleanup()
{
    _client.cleanup();

    return 0;
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::conn_alloc(long index, int mode, on_job_complete_cb_t cb)
{
    //TODO check index
    // TODO check if already allocated
    _cb[index] = cb;

    typename Comm_client::Conn_config_t conn_config;
    conn_config._on_recv_cb_fn = Ciph_comm_agent<Comm_client>::on_recv_cb;
    conn_config._mode = mode; // slave
    _client.conn_alloc(index, conn_config);

    return 0;
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::conn_free(long index)
{
    _cb[index] = NULL;

    _client.conn_free(index);

    return 0;
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::send(long index, struct Dpdk_cryptodev_data_vector* vecs, uint32_t size)
{
    //TODO check index
    Ciph_vec_burst_serializer ser(vecs, size);

    _client.send(index, size, ser);

    return 0;
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::set_rx_mode(long index, long qid, char *mode)
{
    return _client.set_rx_mode(index, qid, mode);
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::poll(long index, long qid)
{
    return _client.poll(index, qid);
}

typedef singleton_holder<Ciph_comm_agent<Memif_client> > Ciph_agent_sngl;
