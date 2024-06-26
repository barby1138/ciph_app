// memif_client.h
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _MEMIF_CLIENT_H_
#define _MEMIF_CLIENT_H_

#include <vector>
#include <thread>

extern "C" {
#include "libmemif.h"
}

struct Memif_client_conn_config;

class IMsg_burst_serializer
{
public:
    virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind) = 0;
};

typedef void (*cpoy_buffer_to_buffer_fn_t) (uint8_t* in_buff, uint32_t in_len, uint8_t* out_buff, uint32_t* out_len);

class Memif_client
{
public:
typedef Memif_client_conn_config Conn_config_t; 
typedef memif_buffer_t Conn_buffer_t; 

public:
    int init();
    int cleanup();

    int conn_alloc(long index, const Memif_client::Conn_config_t& conn_config);
    int conn_free(long index);

    // MAX pck count polled == MAX burst size == 64
    int poll(long index, uint16_t qid);
    // rollback
    int poll_00(long index, uint16_t qid, cpoy_buffer_to_buffer_fn_t cpy_fn);

    int send(long index, uint16_t qid, uint64_t size, IMsg_burst_serializer& ser);

    void print_info ();

private:
    void run();
    int poll_event(int timeout);

    int user_input_handler();
    
    int set_rx_mode (long index, long qid, char *mode);

private:
    enum op_state_e { IDLE, RUNNING };
    op_state_e _op_state;

    std::thread _host_thread;
};

/*
class Memif_connection
{
public:
    int poll(long qid, uint32_t size);
    int send(uint64_t size, IMsg_burst_serializer& ser);
};
*/

typedef void (*on_recv_cb_fn_t) (long index, uint16_t qid, const Memif_client::Conn_buffer_t* rx_bufs, uint32_t len);
typedef void (*on_connect_fn_t) (long index);
typedef void (*on_disconnect_fn_t) (long index);

struct Memif_client_conn_config
{
    on_recv_cb_fn_t _on_recv_cb_fn;
    on_connect_fn_t _on_connect_fn;
    on_disconnect_fn_t _on_disconnect_fn;
    //uint16_t _q_nb;
    uint16_t _mode;
};

#endif // _MEMIF_CLIENT_H_
