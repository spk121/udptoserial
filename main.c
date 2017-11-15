#include <fcntl.h>		/* open */
#include <unistd.h>		/* close */
#include <stdio.h>		/* perror */
#include <stdlib.h>		/* exit */
#include <limits.h>
#include <termios.h>
#include <string.h>
#include "serial.h"

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

