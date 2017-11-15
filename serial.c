#include <termios.h>
#include "serial.h"

int speed_to_bps(speed_t speed)
{
  if (speed == 0)
    return 0;
  else if (speed == B50)
    return 50;
  else if (speed == B75)
    return 75;
  else if (speed == B110)
    return 110;
  else if (speed == B134)
    return 134;
  else if (speed == B150)
    return 150;
  else if (speed == B200)
    return 200;
  else if (speed == B300)
    return 300;
  else if (speed == B600)
    return 600;
  else if (speed == B1200)
    return 1200;
  else if (speed == B1800)
    return 1800;
  else if (speed == B2400)
    return 2400;
  else if (speed == B4800)
    return 4800;
  else if (speed == B9600)
    return 9600;
  else if (speed == B19200)
    return 19200;
  else if (speed == B38400)
    return 38400;
  else if (speed == B57600)
    return 57600;
  else if (speed == B115200)
    return 115200;
  else if (speed == B230400)
    return 230400;

  return -1;
}
