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

#include <string>
#include <vector>

//#include "udp_packet.h"
#include "input_queue.h"
#include "Serial_port.h"
#include "Configuration.h"
#include "Udp_ports.h"
#include "IPv4.h"

bool go = true;

int main()
{
	go = true;

#if 1
	ipv4_test();
#endif

#ifdef WIN32
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != NO_ERROR)
	{
		printf("Winsock initialization failure with error %d\n", ret);
		return 1;
	}
#endif

	Configuration config("udptoserial.ini");

	serial_port_init(config.serial_port_name, config.baud_rate, config.throttle_baud_rate);
	Udp_ports udp_ports(config.local_ip, config.remote_ip, config.port_numbers);
	input_queue inQueue;

	try {
		std::vector<uint8_t> buf;

		serial_port_open();
		udp_ports.open();

		while (go)
		{
			// Forward from the serial port to the UDP ports.
			serial_port_read(buf);
			inQueue.push_bytes(buf);
			while (inQueue.is_packet_available())
				udp_ports.send_packet(inQueue.pop_packet());

			// Forward from the udp ports to the serial port.
			std::vector<uint8_t> bytes;
			do
			{
				bytes = udp_ports.recv_bytes();
				if (bytes.size() > 0)
					serial_port_write(bytes);
			} while (bytes.size() > 0);

			serial_port_tick();
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif

		}
	}
	catch (std::exception& e)
	{
		fprintf(stderr, "%s", e.what());
		return 1;
	}

	//serial_port.close();
	//udp_ports.close();

	return 0;
}

#if 0

// OK, we have some UDP ports and a serial port to listen on.
// When a UDP port receives a packet, push it on the serial port output queue.
// When the serial port receives bytes, push it on the input queue.

// Flush the serial port output queue onto the serial port, if it is ready.
// Pull packets off of the input queue and send the out as UDP packets.

std::string payload_str{ "Blammo" };
std::vector<uint8_t> payload(payload_str.begin(), payload_str.end());

udp_packet p{ 4000, 4001, payload };
std::vector<uint8_t> out = p.to_bytes();

input_queue IQ;

IQ.push_bytes(out);
if (IQ.is_packet_available())
{
	udp_packet p2 = IQ.pop_packet();
	// If the source port for this packet is a UDP port we have open,
	// we can send it from there.
	// If the source port for this packet if from some other UDP packet,
	// maybe we add a new UDP port, or maybe we ignore it.
	if (sockets.have_port(p2.source_port()))
		sockets.send_packet(p2);
	else
		sockets::send_packet_on_temp_socket(p2);

}

return 0;
}
#endif