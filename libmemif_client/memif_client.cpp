// memif_client.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdint.h>
#include <net/if.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <asm/byteorder.h>
#include <byteswap.h>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <time.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "memif_client.h"

extern "C" {
#include <libmemif.h>
}

#define APP_NAME "APP"
#define IF_NAME "memif_connection"

#include <mutex> 
std::recursive_mutex m;
typedef std::lock_guard<std::recursive_mutex> LOCK_GUARD;

/* maximum tx/rx memif buffers */
#define MAX_MEMIF_BUFS 64
#define MAX_CONNS 50
#define ICMPR_HEADROOM 64

#define BUFF_SIZE 2048 - ICMPR_HEADROOM

#define DBG 
#define LOG 
#define INFO 

typedef struct
{
  uint16_t index;
  /* memif conenction handle */
  memif_conn_handle_t conn;
  /* tx buffers */
  memif_buffer_t *tx_bufs;
  /* allocated tx buffers counter */
  /* number of tx buffers pointing to shared memory */
  uint16_t tx_buf_num;
  /* rx buffers */
  memif_buffer_t *rx_bufs;
  /* allcoated rx buffers counter */
  /* number of rx buffers pointing to shared memory */
  uint16_t rx_buf_num;
  /* interface ip address */
  uint8_t ip_addr[4];
  uint16_t seq;
  uint64_t tx_counter, rx_counter, tx_err_counter;
  uint64_t t_sec, t_nsec;

  on_recv_cb_fn_t fn;

  int connected;
} memif_connection_t;

int epfd;
memif_connection_t memif_connection[MAX_CONNS];
//static uint8_t enable_log;    
long ctx[MAX_CONNS];

int add_epoll_fd (int fd, uint32_t events)
{
  if (fd < 0)
    {
      DBG ("invalid fd %d", fd);
      return -1;
    }
  struct epoll_event evt;
  memset (&evt, 0, sizeof (evt));
  evt.events = events;
  evt.data.fd = fd;
  if (epoll_ctl (epfd, EPOLL_CTL_ADD, fd, &evt) < 0)
    {
      DBG ("epoll_ctl: %s fd %d", strerror (errno), fd);
      return -1;
    }
  DBG ("fd %d added to epoll", fd);
  return 0;
}

int mod_epoll_fd (int fd, uint32_t events)
{
  if (fd < 0)
    {
      DBG ("invalid fd %d", fd);
      return -1;
    }
  struct epoll_event evt;
  memset (&evt, 0, sizeof (evt));
  evt.events = events;
  evt.data.fd = fd;
  if (epoll_ctl (epfd, EPOLL_CTL_MOD, fd, &evt) < 0)
    {
      DBG ("epoll_ctl: %s fd %d", strerror (errno), fd);
      return -1;
    }
  DBG ("fd %d moddified on epoll", fd);
  return 0;
}

int del_epoll_fd (int fd)
{
  if (fd < 0)
    {
      DBG ("invalid fd %d", fd);
      return -1;
    }
  struct epoll_event evt;
  memset (&evt, 0, sizeof (evt));
  if (epoll_ctl (epfd, EPOLL_CTL_DEL, fd, &evt) < 0)
    {
      DBG ("epoll_ctl: %s fd %d", strerror (errno), fd);
      return -1;
    }
  DBG ("fd %d removed from epoll", fd);
  return 0;
}

/* informs user about connected status. private_ctx is used by user to identify
   connection (multiple connections WIP) */
int on_connect (memif_conn_handle_t conn, void *private_ctx)
{
  printf ("on_connect\n");

  // [OT] is called after create queues
  LOCK_GUARD lock(m);

  memif_refill_queue (conn, 0, -1, ICMPR_HEADROOM);

  long index = *((long *)private_ctx);
  if (index >= MAX_CONNS)
  {
    INFO ("connection array overflow");
    return -1;
  }
  if (index < 0)
  {
    INFO ("don't even try...");
    return -1;
  }
  memif_connection_t *c = &memif_connection[index];
  if (c->index != index)
  {
    INFO ("invalid context: %ld/%u", index, c->index);
    return -1;
  }

  {
    c->connected = 1;
    printf ("memif connected! %d ind %d\n", c->connected, index);
  }

  return 0;
}

int on_disconnect (memif_conn_handle_t conn, void *private_ctx)
{
  printf ("on_disconnect\n");

  // [OT] is called before delete queues
  LOCK_GUARD lock(m);

  long index = *((long *)private_ctx);
  if (index >= MAX_CONNS)
  {
    INFO ("connection array overflow\n");
    return -1;
  }
  if (index < 0)
  {
    INFO ("don't even try...\n");
    return -1;
  }
  memif_connection_t *c = &memif_connection[index];
  if (c->index != index)
  {
    INFO ("invalid context: %ld/%u\n", index, c->index);
    return -1;
  }

  {
    c->connected = 0;
    printf ("memif disconnected! %d ind %d\n", c->connected, index);
  }

  return 0;
}

/* user needs to watch new fd or stop watching fd that is about to be closed.
    control fd will be modified during connection establishment to minimize CPU
   usage */
int control_fd_update (int fd, uint8_t events, void *ctx)
{
  /* convert memif event definitions to epoll events */
  if (events & MEMIF_FD_EVENT_DEL)
    return del_epoll_fd (fd);

  uint32_t evt = 0;
  if (events & MEMIF_FD_EVENT_READ)
    evt |= EPOLLIN;
  if (events & MEMIF_FD_EVENT_WRITE)
    evt |= EPOLLOUT;

  if (events & MEMIF_FD_EVENT_MOD)
    return mod_epoll_fd (fd, evt);

  return add_epoll_fd (fd, evt);
}

int on_interrupt02 (memif_conn_handle_t conn, void *private_ctx, uint16_t qid)
{
    long index = *((long *)private_ctx);
    if (index >= MAX_CONNS)
    {
      INFO ("connection array overflow");
      return 0;
    }
    if (index < 0)
    {
      INFO ("don't even try...");
      return 0;
    }
    memif_connection_t *c = &memif_connection[index];
    if (c->index != index)
    {
      INFO ("invalid context: %ld/%u", index, c->index);
      return 0;
    }

    int err = MEMIF_ERR_SUCCESS, ret_val;
    int i;
    uint16_t rx, tx;

    do
    {
        // receive data from shared memory buffers 
        err = memif_rx_burst (c->conn, qid, c->rx_bufs, MAX_MEMIF_BUFS, &rx);
        ret_val = err;
        if ((err != MEMIF_ERR_SUCCESS) && (err != MEMIF_ERR_NOBUF))
        {
          INFO ("memif_rx_burst: %s", memif_strerror (err));
          goto error;
        }
        //printf ("recv %d\n", rx);

        c->rx_buf_num += rx;
        c->rx_counter += rx;

        if (rx > 0)
          c->fn(index, c->rx_bufs, rx);

        err = memif_refill_queue (c->conn, qid, rx, ICMPR_HEADROOM);
        if (err != MEMIF_ERR_SUCCESS)
            INFO ("memif_buffer_free: %s", memif_strerror (err));
        c->rx_buf_num -= rx;
    }
    while (ret_val == MEMIF_ERR_NOBUF);

    return 0;

error:
    err = memif_refill_queue (c->conn, qid, rx, ICMPR_HEADROOM);
    if (err != MEMIF_ERR_SUCCESS)
        INFO ("memif_buffer_free: %s", memif_strerror (err));
    c->rx_buf_num -= rx;
    DBG ("freed %d buffers. %u/%u alloc/free buffers", rx, c->rx_buf_num, MAX_MEMIF_BUFS - c->rx_buf_num);

    return 0;
}

int on_interrupt02_poll (long index, uint16_t qid, uint32_t count)
{
    if (index >= MAX_CONNS)
    {
      INFO ("connection array overflow");
      return -1;
    }
    if (index < 0)
    {
      INFO ("don't even try...");
      return -1;
    }
    memif_connection_t *c = &memif_connection[index];
    if (c->index != index)
    {
      INFO ("invalid context: %ld/%u", index, c->index);
      return -1;
    }

    int err = MEMIF_ERR_SUCCESS, ret_val;
    int i;
    uint16_t rx, tx;

    if (0 == count)
    {
      printf ("on_interrupt02_poll 0 == count\n");
      return 0;
    }

    do
    {
        // receive data from shared memory buffers 
        err = memif_rx_burst (c->conn, 
                    qid, 
                    c->rx_bufs, 
                    MAX_MEMIF_BUFS, 
                    &rx);
        ret_val = err;
        if ((err != MEMIF_ERR_SUCCESS) && (err != MEMIF_ERR_NOBUF))
        {
          printf ("memif_rx_burst: %s\n", memif_strerror (err));
          goto error;
        }

        c->rx_buf_num += rx;
        c->rx_counter += rx;

        if (rx > 0)
          c->fn(index, c->rx_bufs, rx);

        err = memif_refill_queue (c->conn, qid, rx, ICMPR_HEADROOM);
        if (err != MEMIF_ERR_SUCCESS)
            printf ("memif_buffer_free: %s\n", memif_strerror (err));
        c->rx_buf_num -= rx;

        //count -= rx;
    }
    while (ret_val == MEMIF_ERR_NOBUF /*&& count*/);

    //printf ("recv ret_val %d\n", ret_val);
    //printf ("recv rx %d\n", rx);

    return 0;

error:
    err = memif_refill_queue (c->conn, qid, rx, ICMPR_HEADROOM);
    if (err != MEMIF_ERR_SUCCESS)
        printf ("memif_buffer_free: %s\n", memif_strerror (err));
    c->rx_buf_num -= rx;
    DBG ("freed %d buffers. %u/%u alloc/free buffers", rx, c->rx_buf_num, MAX_MEMIF_BUFS - c->rx_buf_num);

    return -1;
}

void Memif_client::run()
{
  while (_op_state == RUNNING)
  {
    if (poll_event (/*-1*/ 1000) < 0)
    {
      printf ("poll_event error!\n");
    }
  }
}

int Memif_client::init()
{
  epfd = epoll_create (1);
  add_epoll_fd (0, EPOLLIN);

  /* initialize memory interface */
  int err, i;
    
  err = memif_init (control_fd_update, APP_NAME, NULL, NULL, NULL);
  if (err != MEMIF_ERR_SUCCESS)
  {
    INFO ("memif_init: %s", memif_strerror (err));
    cleanup ();

    return -1;
  }

  for (i = 0; i < MAX_CONNS; i++)
  {
    memset (&memif_connection[i], 0, sizeof (memif_connection_t));
    ctx[i] = i;
  }

  _op_state = RUNNING;
	_host_thread = std::thread(&Memif_client::run, this);

  return 0;
}

int Memif_client::cleanup ()
{
  int err;
  long i;

  _op_state = IDLE;
  _host_thread.join();
  
  for (i = 0; i < MAX_CONNS; i++)
  {
      memif_connection_t *c = &memif_connection[i];
      if (c->conn)
      {
        memif_delete (i);
      }
  }

  err = memif_cleanup ();
  if (err != MEMIF_ERR_SUCCESS)
    INFO ("memif_delete: %s", memif_strerror (err));

  return 0;
}

int Memif_client::conn_alloc (long index, const Memif_client::Conn_config_t& conn_config)
{
    if (index >= MAX_CONNS)
    {
      INFO ("connection array overflow");
      return -1;
    }

    if (index < 0)
    {
      INFO ("don't even try...");
      return -1;
    }
    memif_connection_t *c = &memif_connection[index];

    memif_conn_args_t args;
    memset (&args, 0, sizeof (args));
    args.is_master = conn_config._mode;
    args.log2_ring_size = 14;
    args.buffer_size = 2048;
    args.num_s2m_rings = 1;
    args.num_m2s_rings = 1;
    strncpy ((char *)args.interface_name, IF_NAME, strlen (IF_NAME));
    args.mode = 0;

    args.interface_id = index;
    int err;

    c->connected = 0;
      
    // [OT] we use polling mode
    err = memif_create (&c->conn, &args, on_connect, on_disconnect, /*on_interrupt02*/NULL, &ctx[index]);
    if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_create: %s", memif_strerror (err));
      return -1;
    }

    c->index = index;
    /* alloc memif buffers */
    c->rx_buf_num = 0;
    c->rx_bufs = (memif_buffer_t *)malloc (sizeof (memif_buffer_t) * MAX_MEMIF_BUFS);
    c->tx_buf_num = 0;
    c->tx_bufs = (memif_buffer_t *)malloc (sizeof (memif_buffer_t) * MAX_MEMIF_BUFS);

    c->ip_addr[0] = 192;
    c->ip_addr[1] = 168;
    c->ip_addr[2] = c->index + 1;
    c->ip_addr[3] = 2;

    c->seq = c->tx_err_counter = c->tx_counter = c->rx_counter = 0;

    c->fn = conn_config._on_recv_cb_fn;

    // TODO review
    if (!args.is_master)
      while(c->connected == 0)
      {
          // accept connection
          usleep(1000 * 1000);
          printf ("memif alloc connected: %d\n", c->connected);
      }  

//  set_rx_mode(index, 0, "polling");

    return 0;
}

int Memif_client::conn_free (long index)
{
  if (index >= MAX_CONNS)
  {
      INFO ("connection array overflow");
      return -1;
  }
  if (index < 0)
  {
      INFO ("don't even try...");
      return -1;
  }
  memif_connection_t *c = &memif_connection[index];

  if (c->rx_bufs)
    free (c->rx_bufs);
  c->rx_bufs = NULL;
  c->rx_buf_num = 0;
  if (c->tx_bufs)
    free (c->tx_bufs);
  c->tx_bufs = NULL;
  c->tx_buf_num = 0;

  int err;
  /* disconenct then delete memif connection */
  err = memif_delete (&c->conn);
  if (err != MEMIF_ERR_SUCCESS)
    INFO ("memif_delete: %s", memif_strerror (err));
  if (c->conn != NULL)
    INFO ("memif delete fail");
  return 0;
}

int Memif_client::set_rx_mode (long index, long qid, char *mode)
{
  if (index >= MAX_CONNS)
  {
      INFO ("connection array overflow");
      return -1;
  }
  if (index < 0)
  {
      INFO ("don't even try...");
      return -1;
  }
  memif_connection_t *c = &memif_connection[index];

  if (c->conn == NULL)
  {
      INFO ("no connection at index %ld", index);
      return -1;
  }

  if (strncmp (mode, "interrupt", 9) == 0)
  {
      memif_set_rx_mode (c->conn, MEMIF_RX_MODE_INTERRUPT, qid);
  }
  else if (strncmp (mode, "polling", 7) == 0)
  {
      memif_set_rx_mode (c->conn, MEMIF_RX_MODE_POLLING, qid);
  }
  else
    INFO ("expected rx mode <interrupt|polling>");

  return 0;
}

int Memif_client::poll (long index, long qid, uint32_t size)
{
  LOCK_GUARD lock(m);

  int res;

  if (index >= MAX_CONNS)
  {
    INFO ("connection array overflow");
    return -1;
  }
  if (index < 0)
  {
    INFO ("don't even try...");
    return -1;
  }
  memif_connection_t *c = &memif_connection[index];
  if (c->index != index)
  {
    INFO ("invalid context: %ld/%u", index, c->index);
    return -1;
  }

  if (c->connected == 0)
  {
    //usleep(100);
    //printf ("not connected\n");
    return -2;
  }  

  res = on_interrupt02_poll (index, qid, size);

  return res;
}

void Memif_client::print_info ()
{
  memif_details_t md;
  ssize_t buflen;
  char *buf;
  int err, i, e;
  buflen = 2048;
  buf = malloc (buflen);
  printf ("MEMIF DETAILS\n");
  printf ("==============================\n");
  for (i = 0; i < MAX_CONNS; i++)
  {
      memif_connection_t *c = &memif_connection[i];

      memset (&md, 0, sizeof (md));
      memset (buf, 0, buflen);

      err = memif_get_details (c->conn, &md, buf, buflen);
      if (err != MEMIF_ERR_SUCCESS)
      {
          if (err != MEMIF_ERR_NOCONN)
            INFO ("%s", memif_strerror (err));
          continue;
      }

      printf ("interface index: %d\n", i);

      printf ("\tinterface ip: %u.%u.%u.%u\n", c->ip_addr[0], c->ip_addr[1], c->ip_addr[2], c->ip_addr[3]);
      printf ("\tinterface name: %s\n", (char *)md.if_name);
      printf ("\tapp name: %s\n", (char *)md.inst_name);
      printf ("\tremote interface name: %s\n", (char *)md.remote_if_name);
      printf ("\tremote app name: %s\n", (char *)md.remote_inst_name);
      printf ("\tid: %u\n", md.id);
      printf ("\tsecret: %s\n", (char *)md.secret);
      printf ("\trole: ");
      if (md.role)
        printf ("slave\n");
      else
        printf ("master\n");
      printf ("\tmode: ");
      switch (md.mode)
      {
        case 0:
          printf ("ethernet\n");
          break;
        case 1:
          printf ("ip\n");
          break;
        case 2:
          printf ("punt/inject\n");
          break;
        default:
          printf ("unknown\n");
          break;
      }
      printf ("\tsocket filename: %s\n", (char *)md.socket_filename);
      printf ("\trx queues:\n");
      for (e = 0; e < md.rx_queues_num; e++)
      {
          printf ("\t\tqueue id: %u\n", md.rx_queues[e].qid);
          printf ("\t\tring size: %u\n", md.rx_queues[e].ring_size);
          printf ("\t\tring rx mode: %s\n",
                  md.rx_queues[e].flags ? "polling" : "interrupt");
          printf ("\t\tring head: %u\n", md.rx_queues[e].head);
          printf ("\t\tring tail: %u\n", md.rx_queues[e].tail);
          printf ("\t\tbuffer size: %u\n", md.rx_queues[e].buffer_size);
      }
      printf ("\ttx queues:\n");
      for (e = 0; e < md.tx_queues_num; e++)
      {
          printf ("\t\tqueue id: %u\n", md.tx_queues[e].qid);
          printf ("\t\tring size: %u\n", md.tx_queues[e].ring_size);
          printf ("\t\tring rx mode: %s\n",
                  md.tx_queues[e].flags ? "polling" : "interrupt");
          printf ("\t\tring head: %u\n", md.tx_queues[e].head);
          printf ("\t\tring tail: %u\n", md.tx_queues[e].tail);
          printf ("\t\tbuffer size: %u\n", md.tx_queues[e].buffer_size);
      }
      printf ("\tlink: ");
      if (md.link_up_down)
        printf ("up\n");
      else
      {
          printf ("down\n");
          printf ("\treason: %s\n", md.error);
      }
      printf ("\tcounters: %u.%u %u.%u\n", c->tx_counter, c->rx_buf_num, c->tx_counter, c->tx_err_counter);
  }
  free (buf);
}

enum { MAX_SEND_RETRIES = 1000000 };
enum { RETRY_WAIT_FACTOR = 2 };

int Memif_client::send(long index, uint64_t size, IMsg_burst_serializer& ser)
{
    LOCK_GUARD guard(m);

    uint64_t count = size;

    if (index >= MAX_CONNS)
    {
      INFO ("connection array overflow");
      return -1;
    }
    if (index < 0)
    {
      INFO ("don't even try...");
      return -1;
    }
    memif_connection_t *c = &memif_connection[index];
    if (c->conn == NULL)
    {
      printf ("No connection at index %d. Thread exiting...\n", index);
      return -1;
    }

    if (c->connected == 0)
    {
      printf ("send not connected\n");
      return -2;
    }  
    
    uint16_t tx, i, ind = 0;
    int err = MEMIF_ERR_SUCCESS;
    uint32_t seq = 0;

    int retries = 0;
    int log = 0;
    while (count)
    {
          retries++;

          //if (retries == 2 || retries == 4 || retries == 8)
/*
          if (retries % 4 == 0)
          {
            usleep(100);
          }
*/
          if (retries > MAX_SEND_RETRIES)
          {
              //printf ("retries > MAX_SEND_RETRIES - are we disconnected?\n");
              m.unlock();
              usleep(100 * 1000);
              m.lock();
              if (c->connected == 0)
              {
                printf ("disconnect while send - not connected\n");
                return -2;
              }  
          }

          i = 0;
          // qid == 0 - we use only 1 q
          err = memif_buffer_alloc (
              c->conn, 
              0, 
              c->tx_bufs,
              MAX_MEMIF_BUFS > count ? count : MAX_MEMIF_BUFS, 
              &tx, 
              BUFF_SIZE);
          if (log)
              printf("buff %d %d\n", err, tx);
          if ((err != MEMIF_ERR_SUCCESS) && (err != MEMIF_ERR_NOBUF_RING))
          {
              printf ("memif_buffer_alloc: failed %s\n", memif_strerror (err));
              return -1;
          }

          c->tx_buf_num += tx;

          while (tx)
          {
              while (tx > 2)
              {
                  ser.serialize((uint8_t*)c->tx_bufs[i].data, &c->tx_bufs[i].len, ind);
                  ser.serialize((uint8_t*)c->tx_bufs[i + 1].data, &c->tx_bufs[i + 1].len, ind + 1);
                  
                  i += 2;
                  tx -= 2;
                  ind += 2;
              }
              
              ser.serialize((uint8_t*)c->tx_bufs[i].data, &c->tx_bufs[i].len, ind);

              i++;
              tx--;
              ind++;
          }

          // qid == 0 - we use only 1 q
          err = memif_tx_burst (c->conn, 0, c->tx_bufs, c->tx_buf_num, &tx);
          if (log)
            printf("tx %d %d\n", err, tx);
          if (err != MEMIF_ERR_SUCCESS)
          {
              printf ("memif_tx_burst failed: %s\n", memif_strerror (err));
              return -1;
          }
          c->tx_buf_num -= tx;
          c->tx_counter += tx;
          count -= tx;
    }

    //if (retries > 2)
      //printf("retr %d\n", retries);

    return 0;
}

int Memif_client::user_input_handler ()
{
  char *in = (char *)malloc (256);
  char *ui = fgets (in, 256, stdin);
  char *end;
  long a, b, c;
  if (in[0] == '\n')
    goto done;
  ui = strtok (in, " ");
  if (strncmp (ui, "exit", 4) == 0)
  {
      free (in);
      cleanup ();
      exit (EXIT_SUCCESS);
  }
  else if (strncmp (ui, "del", 3) == 0)
  {
      ui = strtok (NULL, " ");
      if (ui != NULL)
        conn_free (strtol (ui, &end, 10));
      else
        INFO ("expected id");
      goto done;
  }
  else if (strncmp (ui, "show", 4) == 0)
  {
      print_info ();
      goto done;
  }
  else
  {
      INFO ("unknown command: %s", ui);
      goto done;
  }

  return 0;
done:
  free (in);
  return 0;
}

int Memif_client::poll_event (int timeout)
{
  struct epoll_event evt;
  int app_err = 0, memif_err = 0, en = 0;
  uint32_t events = 0;
  struct timespec start, end;
  memset (&evt, 0, sizeof (evt));
  evt.events = EPOLLIN | EPOLLOUT;
  sigset_t sigset;
  sigemptyset (&sigset);
  en = epoll_pwait (epfd, &evt, 1, timeout, &sigset);
  /* id event polled */
  timespec_get (&start, TIME_UTC);
  if (en < 0)
  {
      DBG ("epoll_pwait: %s", strerror (errno));
      return -1;
  }
  if (en > 0)
  {
      /* this app does not use any other file descriptors than stds and memif
       * control fds */
      if (evt.data.fd > 2)
      {
          /* event of memif control fd */
          /* convert epolle events to memif events */
          if (evt.events & EPOLLIN)
            events |= MEMIF_FD_EVENT_READ;
          if (evt.events & EPOLLOUT)
            events |= MEMIF_FD_EVENT_WRITE;
          if (evt.events & EPOLLERR)
            events |= MEMIF_FD_EVENT_ERROR;
          memif_err = memif_control_fd_handler (evt.data.fd, events);
          if (memif_err != MEMIF_ERR_SUCCESS)
            INFO ("memif_control_fd_handler: %s", memif_strerror (memif_err));
      }
      else if (evt.data.fd == 0)
      {
          app_err = user_input_handler ();
      }
      else
      {
          DBG ("unexpected event at memif_epfd. fd %d", evt.data.fd);
      }
  }

  timespec_get (&end, TIME_UTC);
  LOG ("interrupt: %ld", end.tv_nsec - start.tv_nsec);

  if ((app_err < 0) || (memif_err < 0))
  {
      if (app_err < 0)
        DBG ("user input handler error");
      if (memif_err < 0)
        DBG ("memif control fd handler error");
      return -1;
  }

  return 0;
}
