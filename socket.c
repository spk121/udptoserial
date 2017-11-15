#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "socket.h"

void udp_socket_assign_port (udp_socket_t *sock, uint16_t port)
{
  assert (sock->state == UDP_SOCKET_UNINITIALIZED);
  
  if (port < IPPORT_RESERVED)
    fprintf(stderr, "Warning: %d is a port number that is reserved for standard servers.\n", port);

  memset(&(sock->name), 0, sizeof(struct sockaddr_in));
  sock->name.sin_family = AF_INET;
  sock->name.sin_port = htons(port);
  sock->name.sin_addr.s_addr = htonl(INADDR_ANY);
  sock->state = UDP_SOCKET_ADDRESS_ASSIGNED;
}

int udp_socket_try_create (udp_socket_t *sock)
{
  if (sock->state != UDP_SOCKET_CREATING)
    {
      sock->state = UDP_SOCKET_CREATING;
      sock->creation_time = time(NULL);
    }
  else if (sock->creation_time > time(NULL))
    return -1;

  sock->handle = socket (PF_INET, SOCK_DGRAM, 0 /* default */ );
  if (sock->handle < 0)
    {
      /* Some errors aren't going to get better.  */
      if (errno == EPROTONOSUPPORT || errno == EACCES)
	{
	  perror("socket create");
	  sock->state = UDP_SOCKET_FAILED;
	  return -1;
	}
      /* Some errors might better.  Set the retry timer for 60 seconds.  */
      else
	{
	  perror("socket create");
	  sock->creation_time = time(NULL) + 60;
	  return -1;
	}
    }

  sock->state = UDP_SOCKET_CREATED;
  return 0;
}

int udp_socket_try_bind (udp_socket_t *sock)
{
  int bound;
  
  if (sock->state != UDP_SOCKET_BINDING)
    {
      sock->state = UDP_SOCKET_BINDING;
      sock->binding_time = time(NULL);
    }
  else if (sock->binding_time > time(NULL))
    return -1;

  bound = bind (sock->handle, (struct sockaddr *)&(sock->name), sizeof(struct sockaddr_in));
  if (bound < 0)
    {
      /* Most errors won't get better if retried.  */
      if (errno != EADDRINUSE)
	{
	  perror("binding socket");
	  sock->state = UDP_SOCKET_FAILED;
	  return -1;
	}
      /* Else, set a timer and try again later.  */
      perror("binding socket");
      sock->binding_time = time(NULL) + 60;
      return -1;
    }

  sock->state = UDP_SOCKET_READY;
  return 0;
}

int udp_socket_msgrecv(udp_socket_t *sock, pkt_queue_t **queue)
{
  char buf[1500];
  struct sockaddr_in inaddr;
  socklen_t addrlen;
      
  ssize_t bytes_received = recvfrom (sock->handle, (void *)buf, 1500, 0 /* = read */,
				     (struct sockaddr *)&inaddr, &addrlen);
  if (bytes_received < 0)
    {
      if (errno != EINTR && errno != EWOULDBLOCK)
	{
	  perror("reading socket");
	  sock->state = UDP_SOCKET_FAILED;
	  close(sock->handle);
	  return -1;
	}
      return 0;
    }
  else if (bytes_received > 0)
    {
      /* Inaddr has the port of the source of the message.
	 This socket is the port of the destination of the message.
	 We package the ports and payload to the serial port queue. */
      #if 0
      *queue = pkt_queue_append(*queue,
				ntohs(inaddr.sin_port),
				ntohs(sock->name.sin_port),
				buf,
				bytes_received);
      #endif
    }
  return 0;
}

int udp_socket_try_close(udp_socket_t *sock)
{
  int ret;

  if (sock->state != UDP_SOCKET_CLOSING)
    {
      sock->state = UDP_SOCKET_CLOSING;
      sock->closing_time = time(NULL);
    }
  else if (sock->closing_time > time(NULL))
    return -1;

  
  ret = close(sock->handle);
  if (ret < 0)
    {
      /* Some errors aren't going to get better.  */
      if (errno != EINTR)
	{
	  perror("socket close");
	  sock->state = UDP_SOCKET_FAILED;
	  return -1;
	}
      /* Some errors might better.  Set the retry timer for 60 seconds.  */
      else
	{
	  perror("socket close");
	  sock->closing_time = time(NULL) + 60;
	  return -1;
	}
    }

  sock->state = UDP_SOCKET_CLOSED;
  sock->handle = -1;
  return 0;
}
