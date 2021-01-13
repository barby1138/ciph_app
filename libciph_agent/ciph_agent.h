
#include "stdafx.h"

#include "memif_client.h"
#include "data_vectors.h"

#include "memcpy_fast.h"

//enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

typedef void (*on_ops_complete_CallBk_t) (uint32_t, uint16_t, Crypto_operation*, uint32_t);

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
            if (NULL != vec->cipher_buff_list.buffs[i].data)
            {
                clib_memcpy_fast(buffer, vec->cipher_buff_list.buffs[i].data, vec->cipher_buff_list.buffs[i].length);
            }
            else
            {
                printf("WARN!!! BAD buff - cipher_buff_list.buffs[ %d ].data == NULL\n", i);
            }
            
            buffer += vec->cipher_buff_list.buffs[i].length;
            *len += vec->cipher_buff_list.buffs[i].length;
        }
    }

    if (vec->cipher_key.length)
    {
        if (NULL != vec->cipher_key.data)
        {
            clib_memcpy_fast(buffer, vec->cipher_key.data, vec->cipher_key.length);
        }
        else
        {
            printf("WARN!!! BAD buff - cipher_key.data == NULL\n");
        }

        buffer += vec->cipher_key.length;
        *len += vec->cipher_key.length;
    }

    if (vec->cipher_iv.length)
    {
        if (NULL != vec->cipher_iv.data)
        {
            clib_memcpy_fast(buffer, vec->cipher_iv.data, vec->cipher_iv.length);
        }
        else
        {
            printf("WARN!!! BAD buff - cipher_iv.data == NULL\n");
        }

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
            if (NULL != vec->op.outbuff_ptr)
            {
                clib_memcpy_fast(vec->op.outbuff_ptr, 
					vec->cipher_buff_list.buffs[0].data, 
					vec->cipher_buff_list.buffs[0].length);

		        vec->op.outbuff_len = vec->cipher_buff_list.buffs[0].length;
            }
            else
            {
                vec->op.op_status == CRYPTO_OP_STATUS_FAILED;
                printf("WARN!!! BAD buff - op.outbuff_ptr == NULL\n");
            }
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
    Ciph_vec_burst_serializer(const Crypto_operation* vecs, uint32_t size)
        :   _vecs(vecs), 
            _size(size)
    {}

    inline virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind)
    {
        assert(ind < _size);

        crypto_job_to_buffer((uint8_t*) buff, len, &_vecs[ind]);
    }

private:
    const Crypto_operation* _vecs;
    uint32_t _size;
};

enum { MAX_CONNECTIONS = 20, OPS_POOL_PER_CONN_SIZE = 256 };

class Ciph_comm_agent
{
public:
    int init();
    int cleanup();

    int conn_alloc(uint32_t conn_id, uint32_t mode, on_ops_complete_CallBk_t cb);
    int conn_free(uint32_t conn_id);

    int send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size);

    int poll(uint32_t conn_id, uint16_t qid, uint32_t size);

private:
/*
    static void jobs_2_buffs(Crypto_operation* vecs, uint32_t size, typename Comm_client::Conn_buffer_t* buffs)
    {
        for(int i = 0; i < size; ++i)
            crypto_job_to_buffer((uint8_t*) buffs[i].data, &buffs[i].len, &vecs[i]);
    }
*/
    static void on_recv_cb (uint32_t conn_id, uint16_t qid, const typename Memif_client::Conn_buffer_t* rx_bufs, uint32_t len);

    static void buffs_2_jobs(Crypto_operation* vecs, const typename Memif_client::Conn_buffer_t* buffs, uint32_t buff_size)
    {
        assert(buff_size <= OPS_POOL_PER_CONN_SIZE);

        for(int i = 0; i < buff_size; ++i)
            crypto_job_from_buffer((uint8_t*) buffs[i].data, buffs[i].len, &vecs[i]);
    }

private:
    static on_ops_complete_CallBk_t _cb[MAX_CONNECTIONS];
    static Crypto_operation _pool_vecs[MAX_CONNECTIONS][OPS_POOL_PER_CONN_SIZE];

    Memif_client _client;
};

on_ops_complete_CallBk_t Ciph_comm_agent::_cb[MAX_CONNECTIONS] = { 0 };
Crypto_operation Ciph_comm_agent::_pool_vecs[MAX_CONNECTIONS][OPS_POOL_PER_CONN_SIZE] = { 0 };

static void Ciph_comm_agent::on_recv_cb (uint32_t conn_id, uint16_t qid, const typename Memif_client::Conn_buffer_t* rx_bufs, uint32_t len)
{ 
    buffs_2_jobs(_pool_vecs[conn_id], rx_bufs, len);

    if (NULL != _cb[conn_id])
        _cb[conn_id](conn_id, qid, _pool_vecs[conn_id], len);
}

int Ciph_comm_agent::init()
{
    memset(_pool_vecs, 0, MAX_CONNECTIONS * OPS_POOL_PER_CONN_SIZE * sizeof(Crypto_operation));

    return _client.init();
}

int Ciph_comm_agent::cleanup()
{
    return _client.cleanup();
}

int Ciph_comm_agent::conn_alloc(uint32_t conn_id, uint32_t mode, on_ops_complete_CallBk_t cb)
{
    // TODO check conn_id
    // TODO check if already allocated

    int res;

    _cb[conn_id] = cb;

    typename Memif_client::Conn_config_t conn_config;
    conn_config._on_recv_cb_fn = Ciph_comm_agent::on_recv_cb;
    conn_config._mode = mode;
    //conn_config._q_nb = q_nb;

    res = _client.conn_alloc(conn_id, conn_config);

    return 0;
}

int Ciph_comm_agent::conn_free(uint32_t conn_id)
{
    _cb[conn_id] = NULL;

    return _client.conn_free(conn_id);
}

int Ciph_comm_agent::send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size)
{
    Ciph_vec_burst_serializer ser(vecs, size);

    return _client.send(conn_id, qid, size, ser);
}

int Ciph_comm_agent::poll(uint32_t conn_id, uint16_t qid, uint32_t size)
{
    int res =_client.poll(conn_id, qid, size);

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

typedef Simple_singleton_holder<Ciph_comm_agent > Ciph_agent_sngl;
