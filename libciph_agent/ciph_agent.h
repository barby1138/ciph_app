
#include "stdafx.h"

#include "memif_client.h"
#include "data_vectors.h"

#include "memcpy_fast.h"

#include <mutex> 

typedef void (*on_ops_complete_CallBk_t) (uint32_t, uint16_t, Crypto_operation*, uint32_t);
typedef void (*on_connect_CallBk_t) (uint32_t);
typedef void (*on_disconnect_CallBk_t) (uint32_t);

template<int _MAX_CONN>
class Ciph_comm_agent_base
{
protected:
    enum { MAX_CONNECTIONS = _MAX_CONN, OPS_POOL_PER_CONN_SIZE = 256 };
    
public:
    virtual int init(int32_t client_id /*not used for server*/) = 0;

    virtual int cleanup() = 0;

    virtual int conn_alloc( uint32_t conn_id,
                    on_ops_complete_CallBk_t on_ops_complete_CallBk,
                    on_connect_CallBk_t on_connect_CallBk,
                    on_disconnect_CallBk_t on_disconnect_CallBk) = 0;

    virtual int conn_free(uint32_t conn_id) = 0;

    virtual int send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size) = 0;

    virtual int poll(uint32_t conn_id, uint16_t qid, uint32_t size) = 0;

protected:
    static on_ops_complete_CallBk_t _cb[_MAX_CONN];
    static Crypto_operation _pool_vecs[_MAX_CONN][OPS_POOL_PER_CONN_SIZE];

    Memif_client _client;
};

class Ciph_comm_agent_client : public Ciph_comm_agent_base<2>
{

private:
    enum { MAX_CONN_PER_CLIENT = MAX_CONNECTIONS };

public:
    virtual int init(int32_t client_id);

    virtual int cleanup();

    virtual int conn_alloc( uint32_t conn_id, 
                    on_ops_complete_CallBk_t on_ops_complete_CallBk,
                    on_connect_CallBk_t on_connect_CallBk,
                    on_disconnect_CallBk_t on_disconnect_CallBk);

    virtual int conn_free(uint32_t conn_id);

    virtual int send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size);

    virtual int poll(uint32_t conn_id, uint16_t qid, uint32_t size);

private:
    static void on_recv_cb (uint32_t conn_id, uint16_t qid, const typename Memif_client::Conn_buffer_t* rx_bufs, uint32_t len);
    // hide those from client
    static void on_connected (uint32_t conn_id);
    static void on_disconnected (uint32_t conn_id);

    static inline uint32_t get_memif_conn_id(uint32_t conn_id);
    static inline uint32_t get_conn_id(uint32_t memif_conn_id);
    static inline int32_t check_conn_id(uint32_t conn_id);
    static int32_t _client_id;

private:
    static std::recursive_mutex _conn_m[MAX_CONNECTIONS];
    static uint32_t _is_conn_active[MAX_CONNECTIONS];
};

class Ciph_comm_agent_server : public Ciph_comm_agent_base<20>
{
public:
    enum { MAX_CONN = MAX_CONNECTIONS };

public:
    virtual int init(int32_t client_id);
    
    virtual int cleanup();

    virtual int conn_alloc( uint32_t conn_id,
                    on_ops_complete_CallBk_t on_ops_complete_CallBk,
                    on_connect_CallBk_t on_connect_CallBk,
                    on_disconnect_CallBk_t on_disconnect_CallBk);

    virtual int conn_free(uint32_t conn_id);

    virtual int send(uint32_t conn_id, uint16_t qid, const Crypto_operation* vecs, uint32_t size);

    virtual int poll(uint32_t conn_id, uint16_t qid, uint32_t size);

private:
    static void on_recv_cb (uint32_t conn_id, uint16_t qid, const typename Memif_client::Conn_buffer_t* rx_bufs, uint32_t len);
};

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

typedef Simple_singleton_holder<Ciph_comm_agent_client > Ciph_agent_client_sngl;
typedef Simple_singleton_holder<Ciph_comm_agent_server > Ciph_agent_server_sngl;
