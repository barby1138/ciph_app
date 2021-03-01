#include "ciph_agent.h"

enum { CA_MODE_SLAVE = 0, CA_MODE_MASTER = 1 };

//const uint32_t BLOCK_LENGTH = 16;
const uint32_t BUFF_HEADROOM = 32;
const uint32_t BUFF_SIZE = 1600; // aligned to BLOCK_LENGTH

const uint32_t CIFER_IV_LENGTH = 16;
const uint32_t MAX_PCK_LEN = 1504; // 1500 aligned to BLOCK_LENGTH

typedef struct Data_lengths {
    uint32_t ciphertext_length;
    uint32_t cipher_key_length;
    uint32_t cipher_iv_length;
}Data_lengths;

void crypto_job_to_buffer(uint8_t* buffer, uint32_t* len,  Crypto_operation* vec)
{
    *len = 0;

    if (NULL == vec)
    {
        printf("WARNING!!! crypto_job_to_buffer NULL == vac\n");

        return;
    }
    
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
	buffer_op->reserved_1 = vec->op.reserved_1;

    buffer += sizeof(Crypto_operation_context);
    *len += sizeof(Crypto_operation_context);

    Data_lengths data_len;
    data_len.ciphertext_length = 0;
    for (int i = 0; i < vec->cipher_buff_list.buff_list_length; i++)
        data_len.ciphertext_length += vec->cipher_buff_list.buffs[i].length;

    if (data_len.ciphertext_length > MAX_PCK_LEN)
    {
        printf("WARNING!!! data_len.ciphertext_length > MAX_PCK_LEN %d\n", data_len.ciphertext_length);

        buffer_op->op_status = CRYPTO_OP_STATUS_FAILED;

        Data_lengths* buffer_data_len = (Data_lengths* ) buffer;
        buffer_data_len->ciphertext_length = 0;
        buffer_data_len->cipher_key_length = 0;
        buffer_data_len->cipher_iv_length = 0;

        buffer += sizeof(Data_lengths);
        *len += sizeof(Data_lengths);

        return;
    }

    data_len.cipher_key_length = vec->cipher_key.length;
    data_len.cipher_iv_length = vec->cipher_iv.length;

    Data_lengths* buffer_data_len = (Data_lengths* ) buffer;
    buffer_data_len->ciphertext_length = data_len.ciphertext_length;
    buffer_data_len->cipher_key_length = data_len.cipher_key_length;
    buffer_data_len->cipher_iv_length = data_len.cipher_iv_length;

    //printf("crypto_job_to_buffer %d\n", data_len.cipher_key_length);

    buffer += sizeof(Data_lengths);
    *len += sizeof(Data_lengths);

    buffer += BUFF_HEADROOM;
    *len += BUFF_HEADROOM;

    uint8_t* buffer_next = buffer + BUFF_SIZE; // n * 16
    uint32_t len_next = *len + BUFF_SIZE;

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

    buffer = buffer_next;
    *len = len_next;

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
    if (NULL == vec)
    {
        printf("WARNING!!! crypto_job_from_buffer NULL == vac\n");

        return;
    }

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
	vec->op.reserved_1 = buffer_op->reserved_1;

    buffer += sizeof(Crypto_operation_context);

    Data_lengths* pData_len = (Data_lengths*)buffer;
    buffer += sizeof(Data_lengths);

    buffer += BUFF_HEADROOM;

    vec->cipher_buff_list.buffs[0].data = buffer;
    vec->cipher_buff_list.buffs[0].length = pData_len->ciphertext_length;

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

    buffer += BUFF_SIZE;

    vec->cipher_key.data = buffer;
    vec->cipher_key.length = pData_len->cipher_key_length;

    //printf("crypto_job_from_buffer %d\n", vec->cipher_key.length);

    buffer += vec->cipher_key.length;
    vec->cipher_iv.data = buffer;
    vec->cipher_iv.length = pData_len->cipher_iv_length;
}

/*
void crypto_job_to_buffer(uint8_t* buffer, uint32_t* len,  Crypto_operation* vec)
{
    *len = 0;

    if (NULL == vec)
    {
        printf("WARNING!!! NULL == vac\n");

        return;
    }
    
    Crypto_operation* buffer_op = (Crypto_operation*) buffer;
	buffer_op->op.op_status = vec->op.op_status;
	buffer_op->op.op_ctx_ptr = vec->op.op_ctx_ptr;
    buffer_op->op.outbuff_ptr = vec->op.outbuff_ptr;
	buffer_op->op.outbuff_len = vec->op.outbuff_len;
	buffer_op->op.seq = vec->op.seq;
	buffer_op->op.op_type = vec->op.op_type;
	buffer_op->op.sess_id = vec->op.sess_id;
	buffer_op->op.cipher_algo = vec->op.cipher_algo;
	buffer_op->op.cipher_op = vec->op.cipher_op;
	buffer_op->op.reserved_1 = vec->op.reserved_1;

    uint32_t total_ciphertext_length = 0;
    for (int i = 0; i < vec->cipher_buff_list.buff_list_length; i++)
        total_ciphertext_length += vec->cipher_buff_list.buffs[i].length;

    if (total_ciphertext_length > MAX_PCK_LEN)
    {
        printf("WARNING!!! total_ciphertext_length > MAX_PCK_LEN %d\n", total_ciphertext_length);

        buffer_op->op.op_status = CRYPTO_OP_STATUS_FAILED;

        buffer_op->cipher_key.length = 0;
	    buffer_op->cipher_iv.length = 0;
        buffer_op->cipher_buff_list.buff_list_length = 0;

        return;
    }

    buffer_op->cipher_key.length = vec->cipher_key.length;
	buffer_op->cipher_iv.length = vec->cipher_iv.length;
    buffer_op->cipher_buff_list.buff_list_length = 1;
    buffer_op->cipher_buff_list.buffs[0].length = total_ciphertext_length;

    buffer += sizeof(Crypto_operation);
    *len += sizeof(Crypto_operation);

    buffer += BUFF_HEADROOM;
    *len += BUFF_HEADROOM;

    buffer_op->cipher_buff_list.buffs[0].data = buffer;

    uint8_t* buffer_next = buffer + BUFF_SIZE; // n * 16
    uint32_t len_next = *len + BUFF_SIZE;

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

    buffer = buffer_next;
    *len = len_next;

    buffer_op->cipher_key.data = buffer;

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

    buffer_op->cipher_iv.data = buffer;

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

void crypto_job_from_buffer(uint8_t* buffer, uint32_t len, Crypto_operation** vec)
{
    //Crypto_operation* buffer_op = (Crypto_operation*) buffer;
	*vec = (Crypto_operation*) buffer;

    //////////////////////////////////////////////////////////////////////////////////////////////////
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
}
*/

class Ciph_vec_burst_serializer : public IMsg_burst_serializer
{
public:
    Ciph_vec_burst_serializer(const Crypto_operation* vecs, uint32_t size)
        :   _vecs(vecs), 
            _size(size)
    {
        // printf("Ciph_vec_burst_serializer vec %X size %d\n", &_vecs[0], _size);
    }

    inline virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind)
    {
        if (ind < _size)
        {
            crypto_job_to_buffer((uint8_t*) buff, len, &_vecs[ind]);
        }
        else
        {
            printf("WARNING!!! invalid (ind < size) ind %d size %d\n", ind, _size);
        }
    }

private:
    const Crypto_operation* _vecs;
    uint32_t _size;
};

void ctx_to_buffer(uint8_t* buffer, uint32_t* len,  Crypto_operation* vec)
{
    *len = 0;

    if (NULL == vec)
    {
        printf("WARNING!!! crypto_job_to_buffer NULL == vac\n");

        return;
    }
    
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
	buffer_op->reserved_1 = vec->op.reserved_1;

    buffer += sizeof(Crypto_operation_context);
    *len += sizeof(Crypto_operation_context);
}

void copy_buffer_to_buffer(uint8_t* in_buffer, uint32_t in_len, uint8_t* out_buffer, uint32_t* out_len)
{
    Crypto_operation_context* out_buffer_op = (Crypto_operation_context*) out_buffer;
    Crypto_operation_context* in_buffer_op = (Crypto_operation_context*) in_buffer;
	out_buffer_op->op_status = in_buffer_op->op_status;
	out_buffer_op->op_ctx_ptr = in_buffer_op->op_ctx_ptr;
    out_buffer_op->outbuff_ptr = in_buffer_op->outbuff_ptr;
	out_buffer_op->outbuff_len = in_buffer_op->outbuff_len;
	out_buffer_op->seq = in_buffer_op->seq;
	out_buffer_op->op_type = in_buffer_op->op_type;
	out_buffer_op->sess_id = in_buffer_op->sess_id;
	out_buffer_op->cipher_algo = in_buffer_op->cipher_algo;
	out_buffer_op->cipher_op = in_buffer_op->cipher_op;
	out_buffer_op->reserved_1 = in_buffer_op->reserved_1;

    in_buffer += sizeof(Crypto_operation_context);
    out_buffer += sizeof(Crypto_operation_context);

    Data_lengths* in_pData_len = (Data_lengths*)in_buffer;
    Data_lengths* out_pData_len = (Data_lengths*)out_buffer;
    out_pData_len->ciphertext_length = in_pData_len->ciphertext_length;
    out_pData_len->cipher_key_length = in_pData_len->cipher_key_length;
    out_pData_len->cipher_iv_length = in_pData_len->cipher_iv_length;

    //printf("copy_buffer_to_buffer %d %d\n", in_pData_len->cipher_key_length, out_pData_len->cipher_key_length);

    in_buffer += sizeof(Data_lengths);
    out_buffer += sizeof(Data_lengths);

    in_buffer += BUFF_HEADROOM;
    out_buffer += BUFF_HEADROOM;

    if (in_pData_len->ciphertext_length)
        clib_memcpy_fast(out_buffer, 
					in_buffer, 
					in_pData_len->ciphertext_length);

    in_buffer += BUFF_SIZE;
    out_buffer += BUFF_SIZE;

    if (in_pData_len->cipher_key_length)
        clib_memcpy_fast(out_buffer, 
					in_buffer, 
					in_pData_len->cipher_key_length);

    in_buffer += in_pData_len->cipher_key_length;
    out_buffer += in_pData_len->cipher_key_length;

    if (in_pData_len->cipher_iv_length)
        clib_memcpy_fast(out_buffer, 
					in_buffer, 
					in_pData_len->cipher_iv_length);
}

//////////////////////////////////
// Ciph_comm_agent_base

template<int _MAX_CONN>  on_ops_complete_CallBk_t Ciph_comm_agent_base<_MAX_CONN>::_cb[_MAX_CONN] = { 0 };
template<int _MAX_CONN>  Crypto_operation Ciph_comm_agent_base<_MAX_CONN>::_pool_vecs[_MAX_CONN][OPS_POOL_PER_CONN_SIZE] = { 0 };
//template<int _MAX_CONN>  Crypto_operation* Ciph_comm_agent_base<_MAX_CONN>::_ppool_vecs[_MAX_CONN][OPS_POOL_PER_CONN_SIZE] = { 0 };

//////////////////////////////////
// Ciph_comm_agent_client
typedef std::lock_guard<std::recursive_mutex> LOCK_GUARD;

int32_t Ciph_comm_agent_client::_client_id = -1;
std::recursive_mutex Ciph_comm_agent_client::_conn_m[MAX_CONNECTIONS];
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
    conn_config._mode = CA_MODE_SLAVE; //

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

    uint32_t poll_count = (size % 64) ? (size / 64) + 1 : size / 64;
    do
    {
        _client.poll(get_memif_conn_id(conn_id), qid);
    } 
    while(--poll_count);

    return 0;
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

    //[OT] patch serialize ctx back to buff
    uint32_t out_len;
    for(int i = 0; i < len; ++i)
        ctx_to_buffer((uint8_t*) rx_bufs[i].data, &out_len, &_pool_vecs[conn_id][i]);
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
    conn_config._mode = CA_MODE_MASTER; //

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

    uint32_t poll_count = (size % 64) ? (size / 64) + 1 : size / 64;
    do
    {
        _client.poll_00(conn_id, qid, copy_buffer_to_buffer);
    } 
    while(--poll_count);

    return 0;
}
