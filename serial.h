#ifndef U2S_SERIAL_H
#define U2S_SERIAL_H

#include <termios.h>

typedef struct _serial_port_t serial_port_t;

struct _serial_port_t
{
  int bps;
};

int speed_to_bps(speed_t speed);

void serial_port_send (serial_port_t *port, char *str);

#endif
