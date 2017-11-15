#include <sys/socket.h>

/* This pseudoclass creates a UDP packet that listens for or send datagrams on a local port.
   It will try to connect.  */


void loop(int port)
{
  /* Create a localhost port address. */
  if (port_number < IPPORT_RESERVED)
    fprintf(stderr, "Warning: %d is using a port number that is reserved for standard servers.\n", port);

  /* Make the address.  */
  struct sockaddr_in name;
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  /* Create the socket.  */
  int sock = 0;
  timer_T1 = now();
  while (sock <= 0 && quitting == FALSE)
    {
      if (timer_T1 > now())
	sleep(1);
      else
	{
	  sock = socket (PF_INET,
			 SOCK_DGRAM,
			 0 /* Default */ );
	  if (sock < 0)
	    {
	      /* Some errors aren't going to get better.  */
	      if (errno == EPROTONOSUPPORT || errno == EACCESS)
		{
		  /* log error here */
		  return -1;
		}
	      /* Some errors might better.  Set the retry timer for 60 seconds.  */
	      else
		{
		  timer_T1 = now() + 60;
		}
	    }
	}
    }

  if (quitting == TRUE)
    {
      if (sock > 0)
	close (sock);
      return 0;
    }

  /* Try binding the socket to this address.  */
  int bound = -1;
  int timer_T1 = now();
  while (bound < 0 && quitting == FALSE)
    {
      if (timer_T2 > now())
	sleep(1);
      else
	{
	  bound = bind (sock, &name, sizeof(struct sockaddr_in));
	  if (bound < 0)
	    {
	      /* Most errors won't get better if retried.  */
	      if (errno != EADDRINUSE)
		{
		  perror("binding socket");
		  return -1;
		}
	      /* Else, set a timer and try again later.  */
	      perror("binding socket");
	      timer_T2 = now() + 60;
	    }
	}
    }
    
  if (quitting == TRUE)
    {
      if (sock > 0)
	close (sock);
      return 0;
    }

  /* Socket's main loop, waiting for packets. */
  ssize_t bytes_received = -1;
  char buf[1500];
  while (1)
    {
      struct sockaddr_in inaddr;
      
      bytes_received = recvfrom (sock, (void *)buf, 1500, 0 /* = read */, &inaddr, sizeof(inaddr));
      if (bytes_received < 0)
	{
	  perror("reading from socket");
	  if (errno != EINTR)
	      return 1;
	  if (errno == EINTR)
	    {
	      sleep(1);
	      continue;
	    }
	}
      else if (bytes_received > 0)
	{
	  /* Inaddr has the port of the source of the message.
	     This socket is the port of the destination of the message.
	     We package the ports and payload to the serial port queue. */
	  enqueue_packet(ntohs(inaddr.sin_port), port, buf, bytes_received);
	}
    }

	  
enum udp_sock_state {
  UDPSOCK_UNINITIALIZED = 0;
  UDPSOCK_
struct udp_sock
{
  struct sockaddr addr;
  socklen_t addr_len;
  int fd;
};

void func()
{
  struct udp_sock S;
  memset (S, 0, sizeof(S));
  
  /* Create the socket. */
  int S.fd = socket (PF_INET,
			SOCK_DGRAM,
			0 /* Default */ );
  if (S.fd == -1)
    {
      /* Some errors aren't going to get better.  */
      if (errno == EPROTONOSUPPORT || errno == EACCESS)
	;
      /* Some errors might better.  Set the retry timer.  */
      else
	{
	  
    }
  
