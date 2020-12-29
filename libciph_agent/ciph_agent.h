
#include "stdafx.h"

#include "memif_client.h"
#include "data_vectors.h"

#include "memcpy_fast.h"

//enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

typedef void (*on_ops_complete_CallBk_t) (uint32_t, Crypto_operation*, uint32_t);

#define CIFER_IV_LENGTH 16

typedef struct Data_lengths {
    uint32_t ciphertext_length;
    uint32_t cipher_key_length;
    uint32_t cipher_iv_length;
}Data_lengths;

void crypto_job_to_buffer(uint8_t* buffer, uint32_t* len,  Crypto_operation* vec)
{
    *len = 0;

    Crypto_operation_context* buffer_op = (Crypto_operation_context*) buffer;
	buffer_op->op_status = vec->op.op_status;
	buffer_op->op_ctx_ptr = vec->op.op_ctx_ptr;
    buffer_op->outbuff_ptr = vec->op.outbuff_ptr;
	buffer_op->outbuff_len = vec->op.outbuff_len;
	buffer_op->seq = vec->op.seq;
	buffer_op->op_type = vec->op.op_type;
	buffer_op->sess_id = vec->op.sess_id;
	buffer_op->cipher_algo = vec->op.cipher_algo;
	buffer_op->cipher_op = vec->op.cipher_op;
    
    buffer += sizeof(vec->op);
    *len += sizeof(vec->op);

    Data_lengths data_len;
    data_len.ciphertext_length = 0;
    for (int i = 0; i < vec->cipher_buff_list.buff_list_length; i++)
        data_len.ciphertext_length += vec->cipher_buff_list.buffs[i].length;

    data_len.cipher_key_length = vec->cipher_key.length;
    data_len.cipher_iv_length = vec->cipher_iv.length;

    Data_lengths* buffer_data_len = (Data_lengths* ) buffer;
    buffer_data_len->ciphertext_length = data_len.ciphertext_length;
    buffer_data_len->cipher_key_length = data_len.cipher_key_length;
    buffer_data_len->cipher_iv_length = data_len.cipher_iv_length;

    buffer += sizeof(Data_lengths);
    *len += sizeof(Data_lengths);

    for (int i = 0; i < vec->cipher_buff_list.buff_list_length; i++)
    {
        if (vec->cipher_buff_list.buffs[i].length)
        {
            clib_memcpy_fast(buffer, vec->cipher_buff_list.buffs[i].data, vec->cipher_buff_list.buffs[i].length);
            buffer += vec->cipher_buff_list.buffs[i].length;
            *len += vec->cipher_buff_list.buffs[i].length;
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

void crypto_job_from_buffer(uint8_t* buffer, uint32_t len, Crypto_operation* vec)
{
    Crypto_operation_context* buffer_op = (Crypto_operation_context*) buffer;
	vec->op.op_status = buffer_op->op_status;
	vec->op.op_ctx_ptr = buffer_op->op_ctx_ptr;
    vec->op.outbuff_ptr = buffer_op->outbuff_ptr;
	vec->op.outbuff_len = buffer_op->outbuff_len;
	vec->op.seq = buffer_op->seq;
	vec->op.op_type = buffer_op->op_type;
	vec->op.sess_id = buffer_op->sess_id;
	vec->op.cipher_algo = buffer_op->cipher_algo;
	vec->op.cipher_op = buffer_op->cipher_op;

    buffer += sizeof(vec->op);

    Data_lengths* pData_len = (Data_lengths*)buffer;
    buffer += sizeof(Data_lengths);

    // [OT] this is temp patch will be done automatically inside server in REL2 (with dpdk shared mem)
    vec->cipher_buff_list.buffs[0].data = buffer;
    vec->cipher_buff_list.buffs[0].length = pData_len->ciphertext_length;
    vec->cipher_buff_list.buff_list_length = 1;

    if (vec->op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING && vec->op.op_status == CRYPTO_OP_STATUS_SUCC)
    {
        if (vec->op.outbuff_len < vec->cipher_buff_list.buffs[0].length)
        {
            vec->op.op_status == CRYPTO_OP_STATUS_FAILED;
			printf ("outbuff too small %u vs %u\n", vec->op.outbuff_len, vec->cipher_buff_list.buffs[0].length);	
        }
        else
        {
            // TODO check ptr / len
            clib_memcpy_fast(vec->op.outbuff_ptr, 
					vec->cipher_buff_list.buffs[0].data, 
					vec->cipher_buff_list.buffs[0].length);

		    vec->op.outbuff_len = vec->cipher_buff_list.buffs[0].length;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    buffer += pData_len->ciphertext_length;
    vec->cipher_key.data = buffer;
    vec->cipher_key.length = pData_len->cipher_key_length;

    buffer += vec->cipher_key.length;
    vec->cipher_iv.data = buffer;
    vec->cipher_iv.length = pData_len->cipher_iv_length;
}

class Ciph_vec_burst_serializer : public IMsg_burst_serializer
{
public:
    Ciph_vec_burst_serializer(Crypto_operation* vecs, uint32_t size)
        : _vecs(vecs), _size(size)
    {}

    inline virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind)
    {
        // TODO ind > size
        crypto_job_to_buffer((uint8_t*) buff, len, &_vecs[ind]);
    }

private:
    Crypto_operation* _vecs;
    uint32_t _size;
};

enum { MAX_CONNECTIONS = 20, OPS_POOL_PER_CONN_SIZE = 256 };

template<class Comm_client>
class Ciph_comm_agent
{
public:
typedef Comm_client Comm_client_t;

public:
    int init();
    int cleanup();

    int conn_alloc(uint32_t index, uint32_t mode, on_ops_complete_CallBk_t cb);
    int conn_free(uint32_t index);

    int send(uint32_t index, Crypto_operation* vecs, uint32_t size);
    static void on_recv_cb (uint32_t index, const typename Comm_client::Conn_buffer_t* rx_bufs, uint32_t len);

    int set_rx_mode(uint32_t index, uint32_t qid, char *mode);
    int poll(uint32_t index, uint32_t qid, uint32_t size);

private:
/*
    static void jobs_2_buffs(Crypto_operation* vecs, uint32_t size, typename Comm_client::Conn_buffer_t* buffs)
    {
        for(int i = 0; i < size; ++i)
            crypto_job_to_buffer((uint8_t*) buffs[i].data, &buffs[i].len, &vecs[i]);
    }
*/
    static void buffs_2_jobs(Crypto_operation* vecs, const typename Comm_client::Conn_buffer_t* buffs, uint32_t buff_size)
    {
        for(int i = 0; i < buff_size; ++i)
            crypto_job_from_buffer((uint8_t*) buffs[i].data, buffs[i].len, &vecs[i]);
    }

private:
    static on_ops_complete_CallBk_t _cb[MAX_CONNECTIONS];
    static Crypto_operation* _pool_vecs[MAX_CONNECTIONS];

    Comm_client _client;
};

template<class Comm_client> 
on_ops_complete_CallBk_t Ciph_comm_agent<Comm_client>::_cb[MAX_CONNECTIONS] = { 0 };
template<class Comm_client> 
Crypto_operation* Ciph_comm_agent<Comm_client>::_pool_vecs[MAX_CONNECTIONS] = { 0 };

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
        _pool_vecs[i] = (Crypto_operation*) malloc(OPS_POOL_PER_CONN_SIZE * sizeof(Crypto_operation));
        // TODO check null
        memset(_pool_vecs[i], 0, OPS_POOL_PER_CONN_SIZE * sizeof(Crypto_operation));
    }

    return _client.init();
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::cleanup()
{
    // TODO cleanup _pool_vecs
    return _client.cleanup();
}

template<class Comm_client> 
int Ciph_comm_agent<Comm_client>::conn_alloc(uint32_t index, uint32_t mode, on_ops_complete_CallBk_t cb)
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
int Ciph_comm_agent<Comm_client>::send(uint32_t index, Crypto_operation* vecs, uint32_t size)
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

template<typename T>
class Simple_singleton_holder {
public:
    static T& instance();

private:
    Simple_singleton_holder(const Simple_singleton_holder&);
    Simple_singleton_holder& operator= (const Simple_singleton_holder);
};

template<typename T>
T& Simple_singleton_holder<T>::instance()
{
    static T instance;
    return instance;
}

typedef Simple_singleton_holder<Ciph_comm_agent<Memif_client> > Ciph_agent_sngl;
