// memif_client.h
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _MEMIF_CLIENT_H_
#define _MEMIF_CLIENT_H_

#include <vector>
#include <thread>

extern "C" {
#include <libmemif.h>
}

struct Memif_client_conn_config;

class IMsg_burst_serializer
{
public:
    virtual void serialize(uint8_t* buff, uint32_t* len, uint32_t ind) = 0;
};
/*
class IComm_client
{
public:
    virtual int init() = 0;
    virtual int cleanup() = 0;

    virtual int conn_alloc(long index, const IComm_client_config_t& conn_config) = 0;
    virtual int conn_free(long index) = 0;

    virtual void send_buff(int index, uint64_t size, IMsg_burst_serializer& ser) = 0;

    virtual void print_info() = 0;
};
*/
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

    // priv?
    int set_rx_mode (long index, long qid, char *mode);

    int poll (long index, long qid);
    void send(long index, uint64_t size, IMsg_burst_serializer& ser);

    void print_info ();

private:
    void run();
    int poll_event(int timeout);

    int user_input_handler();

private:
    enum op_state_e { IDLE, RUNNING };
    op_state_e _op_state;

    std::thread _host_thread;
};

typedef void (*on_recv_cb_fn_t) (long index, const Memif_client::Conn_buffer_t* rx_bufs, uint32_t len);

struct Memif_client_conn_config
{
    on_recv_cb_fn_t _on_recv_cb_fn;
    long _mode;
};

#endif // _MEMIF_CLIENT_H_
