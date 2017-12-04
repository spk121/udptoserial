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
#include <map>
#include <list>
#include <sstream>
#include <boost/log/trivial.hpp>
#include "../libhorizr/libhorizr.h"

//#include "udp_packet.h"
#include "input_queue.h"
// #include "Serial_port.h"
#include "Configuration.h"
// #include "Udp_ports.h"
// #include "IPv4.h"
#include "Tcp_server_handler.h"
#include <functional>
using namespace std::placeholders;

bool go = true;
asio::io_service io_service_;
std::map<uint16_t, std::shared_ptr<asio::ip::tcp::acceptor>> tcp_server_acceptor_map_;
std::shared_ptr<asio::serial_port> serial_port_;
std::list<std::shared_ptr<Tcp_server_handler>> tcp_server_handler_list_;

void tcp_server_accept_handler(std::shared_ptr<Tcp_server_handler> handler, const boost::system::error_code& ec)
{
	if (ec)
	{ 
		BOOST_LOG_TRIVIAL(error) << ec.message();
		return;
	}
	BOOST_LOG_TRIVIAL(debug) << "Accepted connection on TCP port " << handler->socket().local_endpoint().port();	


	// Start up the waiting handler.
	BOOST_LOG_TRIVIAL(debug) << "Starting handler on TCP server " << handler->socket().remote_endpoint().port() << " -> " << handler->socket().local_endpoint().port();
	handler->start();


	// Queue up a new handler for the next connection..
	uint16_t port = handler->socket().local_endpoint().port();
	std::shared_ptr<Tcp_server_handler> handler2 = std::make_shared<Tcp_server_handler>(io_service_, serial_port_);
	tcp_server_handler_list_.push_back(handler2);
	auto func = std::bind(tcp_server_accept_handler, handler2, _1);
	auto p_tcp_acptr = tcp_server_acceptor_map_.at(port);	
	p_tcp_acptr->async_accept(handler2->socket(), func);	
}


const static size_t SERIAL_READ_BUFFER_SIZE = 8*1024;
unsigned char serial_read_buffer_raw_[SERIAL_READ_BUFFER_SIZE];
std::vector<uint8_t> serial_read_buffer_unprocessed_;


void serial_read_handler(
  const boost::system::error_code& error, // Result of operation.
  std::size_t bytes_transferred           // Number of bytes read.
)
{
	BOOST_LOG_TRIVIAL(debug) << "serial port got " << bytes_transferred << " bytes";
	serial_read_buffer_unprocessed_.insert(serial_read_buffer_unprocessed_.begin(), (unsigned char *)&serial_read_buffer_raw_[0], serial_read_buffer_raw_ + bytes_transferred);

	// See if this is a complete SLIP message
	std::vector<uint8_t> slip_msg;
	size_t bytes_decoded;
	bytes_decoded = slip_decode(slip_msg, serial_read_buffer_unprocessed_, false);
	if (bytes_decoded > 0)
	{
		bool ret = ip_bytevector_validate(slip_msg);
		#if 0
		if (msg_type == MSG_TYPE_TCP)
			// Check if that port is open and has a handler
			// If it is open, use the existing port to write.
			// If it isn't open, open a new ephemeral port
			;
		else if (msg_type == MSG_TYPE_UDP)
			// Check if that port is open and has a handler
			// If it is open, use the existing port to write.
			// If it isn't open, open a new ephemeral port
			;
		else
			// unhandled message type, or junk
			;
			#endif
	}
	// And queue up the next async read

	serial_port_->async_read_some
	(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
	serial_read_handler);
}

int main()
{
	go = true;

#if 1
	// ipv4_test();
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

	std::string port = "/dev/ttyUSB0";
	BOOST_LOG_TRIVIAL(debug) << "Adding new serial port port " << port;
	serial_port_ = std::make_shared<asio::serial_port>(io_service_);
	serial_port_->open(port);
	serial_port_->set_option(asio::serial_port_base::baud_rate(115200));

	// Queue up an async read handler

	serial_port_->async_read_some
	(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
	serial_read_handler);

	for (uint16_t port = 8888; port <= 8889; port ++)
	{
		// Configuration config("udptoserial.ini");
		BOOST_LOG_TRIVIAL(debug) << "Adding new TCP server port " << port;

		// We start off making an acceptor that we will later use to join a handler
		// to new connections on this port.
		asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
		// Stuff a new empty acceptor into the port-to-acceptor map.
		auto p_tcp_acptr = std::make_shared<asio::ip::tcp::acceptor>(io_service_);
		p_tcp_acptr->open(endpoint.protocol());
		p_tcp_acptr->set_option(tcp::acceptor::reuse_address(true));
		p_tcp_acptr->bind(endpoint);
		p_tcp_acptr->listen();

		tcp_server_acceptor_map_.insert(std::make_pair(port, p_tcp_acptr));

		// Queue up an async handler
		std::shared_ptr<Tcp_server_handler> handler = std::make_shared<Tcp_server_handler>(io_service_, serial_port_);
		tcp_server_handler_list_.push_back(handler);
		auto func = std::bind(tcp_server_accept_handler, handler, _1);
		p_tcp_acptr->async_accept(handler->socket(), func);
	}


	io_service_.run();
#if 0
	asio_generic_server server;
	server.add_tcp_server_port(8888);
	server.add_tcp_server_port(8889);
	server.add_serial_port("/dev/ttyUSB0");
	server.start_server();
#endif
#if 0
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
#endif
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
