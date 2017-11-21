#ifndef U2S_SERIAL_H
#define U2S_SERIAL_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <termios.h>
#endif

typedef struct _serial_port_t serial_port_t;

struct _serial_port_t
{
#ifdef WIN32
	HANDLE handle;
#endif;
  int bps;
};

DCB x;

#ifdef WIN32
int speed_to_bps(DWORD speed);
#else
int speed_to_bps(speed_t speed);
#endif

serial_port_t *serial_port_new(const char *port_name);
void serial_port_send (serial_port_t *port, char *str);

#endif
