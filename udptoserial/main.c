#include <fcntl.h>		/* open */
#include <limits.h>
#include <stdio.h>		/* perror */
#include <stdbool.h>
#include <stdlib.h>		/* exit */
#include <string.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <termios.h>
#include <unistd.h>		/* close */
#endif

#include "serial.h"
#include "socket.h"

#define UDP_PORT_COUNT (3)
uint16_t ports[UDP_PORT_COUNT] = {4000, 42420, 42421};
udp_socket_t sockets[UDP_PORT_COUNT];

bool quitting = false;

int main()
{
  int actions;
  int ret;
  pkt_queue_t *queue = NULL;
  serial_port_t *port = NULL;
#ifdef WIN32
  WSADATA wsaData;
  int iResult;
  u_long iMode = 0;
#endif

#ifdef WIN32
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != NO_ERROR)
	  printf("Error at WSAStartup()\n");
  port = serial_port_new("COM3");
#else
	port = serial_port_new("ttyS1");
#endif
	serial_port_info(port);
	
  memset (sockets, 0, sizeof(sockets));

  
  while (true)
    {
      actions = 0;
      /* Check in on each UDP socket. */
      for (int i = 0; i < UDP_PORT_COUNT; i ++)
	{
	  udp_socket_state_t state = sockets[i].state;
	  
	  if (quitting)
	    state = UDP_SOCKET_CLOSING;

	  switch (state)
	    {
	    case UDP_SOCKET_UNINITIALIZED:
	      udp_socket_assign_port (&sockets[i], ports[i]);
	      ret = 0;
	      break;
	    case UDP_SOCKET_ADDRESS_ASSIGNED:
	    case UDP_SOCKET_CREATING:
	      ret = udp_socket_try_create(&sockets[i]);
	      break;
	    case UDP_SOCKET_CREATED:
	    case UDP_SOCKET_BINDING:
	      ret = udp_socket_try_bind(&sockets[i]);
	      break;
	    case UDP_SOCKET_READY:
	      ret = udp_socket_msgrecv(&sockets[i], &queue);
	      break;
	    case UDP_SOCKET_CLOSING:
	      ret = udp_socket_try_close(&sockets[i]);
	      break;
	    case UDP_SOCKET_FAILED:
	    default:
	      break;
	    }
	  if (ret == 0)
	    actions ++;
	}
      /* Then check in on the message queue. */
      if (queue)
	{
	  queue = pkt_queue_send(queue, port);
	  actions ++;
#ifdef WIN32
	  Sleep(1);
#else
	  usleep(1);
#endif
	}

	  /* Read all the input waiting on the serial port. */
	  /* If there are no message delimiters in the input, but, the input
	     is very large, dump the input. */
	  /* If there are message delimiters in the input, try to parse a message from
	     the input. */
	  /* If the message is valid and it is addressed from a port we have open, send it
	     out.*/

      /* If nothing is happening, sleep for a bit. */
#ifdef WIN32
	  if (actions == 0)
		  Sleep(1);
#else
	  if (actions == 0)
		  usleep(1000);
#endif
    }
  return 0;
}

#if 0
int main(int argc, char *argv[])
{
  int fd = open("/dev/ttyS4", O_RDWR | O_NOCTTY);
  if (fd == -1)
    {
      perror("Opening serial port");
      exit(1);
    }
  if (isatty(fd) == 1)
    {
      printf("This is a tty\n");
      char buf[100];
      int ret = ttyname_r(fd, buf, 99);
      if (ret != 0)
	{
	  perror("Get tty name");
	  exit(1);
	}
      printf("This tty is named %s\n", buf);
      printf("The minimum maximum line length is %d %d\n", _POSIX_MAX_CANON, MAX_CANON);

      // Get information abbout the terminal
      struct termios tinfo;
      memset(&tinfo, 0, sizeof(struct termios));
      ret = tcgetattr(fd, &tinfo);
      if (ret == -1)
	{
	  perror("tcgetattr");
	  exit(1);
	}
      speed_t speed = cfgetispeed(&tinfo);
      printf("ISPEED %u OSPEED %u\n", speed_to_bps(speed),
	     speed_to_bps(cfgetospeed(&tinfo)));

      /* put the serial port info into raw mode */
      cfmakeraw(&tinfo);
      int result = tcsetattr (fd, TCSANOW, &tinfo);
      if (result < 0)
	{
	  perror("error in tcsetattr");
	  exit(1);
	}
    }
  else
    printf("This is not a tty\n");

  
  return 0;
}

#endif
