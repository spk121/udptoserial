#include "socket.h"

int socket_set_non_binding (int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  int s = fcntl (fp, F_SETFL, flags);
  if (s == -1)
    {
      perror("fcntl");
      return -1;
    }
  return 0;
}
