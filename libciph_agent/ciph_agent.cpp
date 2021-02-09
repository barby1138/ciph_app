#include "ciph_agent.h"

enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

const uint32_t CIFER_IV_LENGTH = 16;

// is it needed for SNOW?
//#define DO_BLOCK_PAD
#ifdef DO_BLOCK_PAD
const uint32_t BLOCK_LENGTH = 16;
#endif

typedef struct Data_lengths {
    uint32_t ciphertext_length;
    uint32_t cipher_key_length;
    uint32_t cipher_iv_length;
#ifdef DO_BLOCK_PAD
    uint32_t data_offset;
#endif
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
    
    buffer += sizeof(Crypto_operation_context);
    *len += sizeof(Crypto_operation_context);

    Data_lengths data_len;
    data_len.ciphertext_length = 0;
    for (int i = 0; i < vec->cipher_buff_list.buff_list_length; i++)
        data_len.ciphertext_length += vec->cipher_buff_list.buffs[i].length;

#ifdef DO_BLOCK_PAD
    int data_offset = 0;
    if (data_len.ciphertext_length < BLOCK_LENGTH)
    {
        data_offset = BLOCK_LENGTH;
        // for AESNI decoder
        data_len.ciphertext_length += BLOCK_LENGTH;
    }
#endif

    data_len.cipher_key_length = vec->cipher_key.length;
    data_len.cipher_iv_length = vec->cipher_iv.length;

    Data_lengths* buffer_data_len = (Data_lengths* ) buffer;
    buffer_data_len->ciphertext_length = data_len.ciphertext_length;
    buffer_data_len->cipher_key_length = data_len.cipher_key_length;
    buffer_data_len->cipher_iv_length = data_len.cipher_iv_length;
#ifdef DO_BLOCK_PAD
    buffer_data_len->data_offset = data_offset;
#endif
    buffer += sizeof(Data_lengths);
    *len += sizeof(Data_lengths);

#ifdef DO_BLOCK_PAD
    if (data_offset)
    {
        memset(buffer, 0, data_offset);
        buffer += data_offset;
        *len += data_offset;
    }
#endif

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

    buffer += sizeof(Crypto_operation_context);

    Data_lengths* pData_len = (Data_lengths*)buffer;
    buffer += sizeof(Data_lengths);

    // [OT] this is temp patch will be done automatically inside server in REL2 (with dpdk shared mem)
#ifdef DO_BLOCK_PAD
    vec->cipher_buff_list.buffs[0].data = buffer + pData_len->data_offset;
    vec->cipher_buff_list.buffs[0].length = pData_len->ciphertext_length - pData_len->data_offset;
#else
    vec->cipher_buff_list.buffs[0].data = buffer;
    vec->cipher_buff_list.buffs[0].length = pData_len->ciphertext_length;
#endif

    vec->cipher_buff_list.buff_list_length = 1;

    if (vec->op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING)
    {
        if (vec->op.op_status == CRYPTO_OP_STATUS_SUCC)
        {
            if ( NULL != vec->op.outbuff_ptr && vec->cipher_buff_list.buffs[0].length <= vec->op.outbuff_len )
            {
                clib_memcpy_fast(vec->op.outbuff_ptr, 
					vec->cipher_buff_list.buffs[0].data, 
					vec->cipher_buff_list.buffs[0].length);

		        vec->op.outbuff_len = vec->cipher_buff_list.buffs[0].length;
            }
            else
            {
                vec->op.op_status == CRYPTO_OP_STATUS_FAILED;
                printf("ERROR!!! BAD buff or len ptr %x, %d <= %d\n", vec->op.outbuff_ptr, 
                                    vec->cipher_buff_list.buffs[0].length,
                                    vec->op.outbuff_len);
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

//////////////////////////////////
// Ciph_comm_agent_base

template<int _MAX_CONN>  on_ops_complete_CallBk_t Ciph_comm_agent_base<_MAX_CONN>::_cb[_MAX_CONN] = { 0 };
template<int _MAX_CONN>  Crypto_operation Ciph_comm_agent_base<_MAX_CONN>::_pool_vecs[_MAX_CONN][OPS_POOL_PER_CONN_SIZE] = { 0 };

//////////////////////////////////
// Ciph_comm_agent_client
typedef std::lock_guard<std::mutex> LOCK_GUARD;

int32_t Ciph_comm_agent_client::_client_id = -1;
std::mutex Ciph_comm_agent_client::_conn_m[MAX_CONNECTIONS];
uint32_t Ciph_comm_agent_client::_is_conn_active[MAX_CONNECTIONS] = { 0 };

static void Ciph_comm_agent_client::on_recv_cb (uint32_t memif_conn_id, uint16_t qid, const typename Memif_client::Conn_buffer_t* rx_bufs, uint32_t len)
{ 
    uint32_t conn_id = get_conn_id(memif_conn_id);

    assert(0 == check_conn_id(conn_id));
    assert(len <= OPS_POOL_PER_CONN_SIZE);

    for(int i = 0; i < len; ++i)
        crypto_job_from_buffer((uint8_t*) rx_bufs[i].data, rx_bufs[i].len, &_pool_vecs[conn_id][i]);

    if (NULL != _cb[conn_id])
        _cb[conn_id](conn_id, qid, _pool_vecs[conn_id], len);
}

static void Ciph_comm_agent_client::on_connected (uint32_t memif_conn_id) 
{
    uint32_t conn_id = get_conn_id(memif_conn_id);

    assert(0 == check_conn_id(conn_id));
  
    LOCK_GUARD lock (_conn_m[conn_id]);

    printf("on_connected %d", conn_id);

    _is_conn_active[conn_id] = 1;
}

static void Ciph_comm_agent_client::on_disconnected (uint32_t memif_conn_id) 
{
    uint32_t conn_id = get_conn_id(memif_conn_id);

    assert(0 == check_conn_id(conn_id));
  
    LOCK_GUARD lock (_conn_m[conn_id]);

    printf("on_disconnected %d", conn_id);

    _is_conn_active[conn_id] = 0;
}

int Ciph_comm_agent_client::init(int32_t client_id)
{
    _client_id = client_id;

    memset(_pool_vecs, 0, MAX_CONNECTIONS * OPS_POOL_PER_CONN_SIZE * sizeof(Crypto_operation));

    return _client.init();
}

int Ciph_comm_agent_client::cleanup()
{
    return _client.cleanup();
}

int Ciph_comm_agent_client::conn_alloc(uint32_t conn_id, 
                                on_ops_complete_CallBk_t on_ops_complete_CallBk,
                                // dummy - hidden from client
                                on_connect_CallBk_t on_connect_CallBk,
                                on_disconnect_CallBk_t on_disconnect_CallBk)
{
    if (0 != check_conn_id(conn_id))
        return -1;

    _cb[conn_id] = on_ops_complete_CallBk;

    typename Memif_client::Conn_config_t conn_config;
    conn_config._on_recv_cb_fn = Ciph_comm_agent_client::on_recv_cb;
    conn_config._on_connect_fn = Ciph_comm_agent_client::on_connected;
    conn_config._on_disconnect_fn = Ciph_comm_agent_client::on_disconnected;
    conn_config._mode = CA_MODE_SLAVE;

    int res = _client.conn_alloc(get_memif_conn_id(conn_id), conn_config);

    return res;
}

int Ciph_comm_agent_client::conn_free(uint32_t conn_id)
{
    if (0 != check_conn_id(conn_id))
        return -1;

    _cb[conn_id] = NULL;

    return _client.conn_free(get_memif_conn_id(conn_id));
}

int Ciph_comm_agent_client::send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size)
{
    if (0 != check_conn_id(conn_id))
        return -1;

    LOCK_GUARD lock (_conn_m[conn_id]);
    if (0 == _is_conn_active[conn_id])
    {
        //printf("connection %d NOT active\n", conn_id);
        return -2;
    }

    Ciph_vec_burst_serializer ser(vecs, size);

    return _client.send(get_memif_conn_id(conn_id), qid, size, ser);
}

int Ciph_comm_agent_client::poll(uint32_t conn_id, uint16_t qid, uint32_t size)
{
    if (0 != check_conn_id(conn_id))
        return -1;

    LOCK_GUARD lock (_conn_m[conn_id]);
    if (0 == _is_conn_active[conn_id])
    {
        //printf("connection %d NOT active\n", conn_id);
        return -2;
    }

    return _client.poll(get_memif_conn_id(conn_id), qid, size);
}

static inline uint32_t Ciph_comm_agent_client::get_memif_conn_id(uint32_t conn_id) 
{ 
    //printf("get_memif_conn_id conn_id (%d)\n", conn_id);

    return (_client_id == -1 ) ? conn_id : conn_id + MAX_CONN_PER_CLIENT * _client_id; 
}

static inline uint32_t Ciph_comm_agent_client::get_conn_id(uint32_t memif_conn_id) 
{ 
    //printf("get_conn_id memif_conn_id (%d)\n", memif_conn_id);

    return (_client_id == -1 ) ? memif_conn_id : memif_conn_id - MAX_CONN_PER_CLIENT * _client_id; 
}

static inline int32_t Ciph_comm_agent_client::check_conn_id(uint32_t conn_id) 
{
    if (_client_id == -1)
        return 0;

    if (conn_id < MAX_CONN_PER_CLIENT)
        return 0;

    printf("conn_id out of range conn_id (%d) >= MAX_CONN_PER_CLIENT (%d)\n", conn_id, MAX_CONN_PER_CLIENT);

    return -1;
}

//////////////////////////////////
// Ciph_comm_agent_server

static void Ciph_comm_agent_server::on_recv_cb (uint32_t conn_id, uint16_t qid, const typename Memif_client::Conn_buffer_t* rx_bufs, uint32_t len)
{ 
    assert(len <= OPS_POOL_PER_CONN_SIZE);

    for(int i = 0; i < len; ++i)
        crypto_job_from_buffer((uint8_t*) rx_bufs[i].data, rx_bufs[i].len, &_pool_vecs[conn_id][i]);

    if (NULL != _cb[conn_id])
        _cb[conn_id](conn_id, qid, _pool_vecs[conn_id], len);
}

int Ciph_comm_agent_server::init(int32_t client_id)
{
    memset(_pool_vecs, 0, MAX_CONNECTIONS * OPS_POOL_PER_CONN_SIZE * sizeof(Crypto_operation));

    return _client.init();
}

int Ciph_comm_agent_server::cleanup()
{
    return _client.cleanup();
}

int Ciph_comm_agent_server::conn_alloc(uint32_t conn_id, 
                                on_ops_complete_CallBk_t on_ops_complete_CallBk,
                                on_connect_CallBk_t on_connect_CallBk,
                                on_disconnect_CallBk_t on_disconnect_CallBk)
{
    _cb[conn_id] = on_ops_complete_CallBk;

    typename Memif_client::Conn_config_t conn_config;
    conn_config._on_recv_cb_fn = Ciph_comm_agent_server::on_recv_cb;
    conn_config._on_connect_fn = on_connect_CallBk;
    conn_config._on_disconnect_fn = on_disconnect_CallBk;
    conn_config._mode = CA_MODE_MASTER;

    int res = _client.conn_alloc(conn_id, conn_config);

    return res;
}

int Ciph_comm_agent_server::conn_free(uint32_t conn_id)
{
    _cb[conn_id] = NULL;

    return _client.conn_free(conn_id);
}

int Ciph_comm_agent_server::send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size)
{
    Ciph_vec_burst_serializer ser(vecs, size);

    return _client.send(conn_id, qid, size, ser);
}

int Ciph_comm_agent_server::poll(uint32_t conn_id, uint16_t qid, uint32_t size)
{
    return _client.poll_00(conn_id, qid, size);
}
