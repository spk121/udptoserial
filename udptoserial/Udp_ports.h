#pragma once
#include <cstdint>
#include <vector>
#include <map>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Windows.h>
#include <WinSock2.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif
#include "udp_packet.h"

class Udp_ports
{
public:
	Udp_ports();
	Udp_ports(std::string local_ip, std::string remote_ip, std::vector<uint16_t>);
	~Udp_ports();

	void open();
	void close();
	void send_packet(udp_packet& P);
	std::vector<uint8_t> recv_bytes();
	//void read_and_forward(Serial_port& serial_port);

private:
	std::string default_local_ip;
	std::string default_remote_ip;
	std::map<uint16_t, SOCKET> port_map;
};

