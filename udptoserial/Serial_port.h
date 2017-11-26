#pragma once
#include <cstdint>
#include <string>
#include <vector>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <termios.h>
#include <unistd.h>		/* close */
#endif

// Because of all the Windows API callback complication, we
// use a singleton pseudoclass instead of a proper class.

void serial_port_init(std::string name, uint32_t baud_rate, uint32_t throttle);
void serial_port_open();
void serial_port_read(std::vector<uint8_t>& output);
void serial_port_write(std::vector<uint8_t>& input);
void serial_port_tick();
void serial_port_close();

#if 0
class Serial_port
{
	const static size_t READ_BUFFER_SIZE = 8;
public:
	Serial_port(std::string name, uint32_t baud_rate, uint32_t throttle);
	~Serial_port();

	void open();
	void close();
	std::vector<uint8_t> read();
	void read_and_forward();


private:
	std::string serial_port_name;
	uint32_t baud_rate;
	uint32_t throttle_baud_rate;
#ifdef WIN32
	HANDLE handle;
	bool read_initialized;
	OVERLAPPED async_data;
	uint8_t read_buffer[READ_BUFFER_SIZE];
#else
#endif
};

#endif