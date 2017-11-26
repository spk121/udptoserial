#ifndef U2S_SERIAL_H
#define U2S_SERIAL_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <termios.h>
#endif
#include <stddef.h>

// ON 


typedef enum _serial_port_parity_t serial_port_parity_t;

enum _serial_port_parity_t
{
  SP_NOPARITY,
  SP_ODDPARITY,
  SP_EVENPARITY
};

typedef struct _serial_port_t serial_port_t;

struct _serial_port_t
{
#ifdef WIN32
  HANDLE handle;
#else
  int handle;
#endif
  char *ttyname; 
  int baud_rate;
  int byte_size;
  int stop_bits;
  serial_port_parity_t parity;
};

#ifdef WIN32
int speed_to_bps(DWORD speed);
#else
int speed_to_bps(speed_t speed);
#endif

serial_port_t *serial_port_new(const char *port_name);
void serial_port_info(const serial_port_t *sp);
void serial_port_send (const serial_port_t *port, const char *str, size_t len);

#endif
