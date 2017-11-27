#ifndef U2S_SOCKET_H
#define U2S_SOCKET_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif

#include "queue.h"

typedef enum _udp_socket_state udp_socket_state_t;

enum _udp_socket_state {
  UDP_SOCKET_UNINITIALIZED = 0,
  UDP_SOCKET_ADDRESS_ASSIGNED,
  UDP_SOCKET_CREATING,
  UDP_SOCKET_CREATED,
  UDP_SOCKET_BINDING,
  UDP_SOCKET_READY,
  UDP_SOCKET_CLOSING,
  UDP_SOCKET_CLOSED,
  UDP_SOCKET_FAILED,
};

typedef struct _udp_socket_t udp_socket_t;

struct _udp_socket_t
{
  udp_socket_state_t state;
  int handle;
  time_t creation_time;
  time_t binding_time;
  time_t closing_time;
  struct sockaddr_in name;


};

void udp_socket_assign_port (udp_socket_t *sock, uint16_t port);
int udp_socket_try_create (udp_socket_t *sock);
int udp_socket_try_bind (udp_socket_t *sock);
int udp_socket_msgrecv(udp_socket_t *sock, pkt_queue_t **queue);
int udp_socket_try_close(udp_socket_t *sock);

#endif
