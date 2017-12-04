#pragma once

#ifdef WIN32
#include <sdkddkver.h>
#endif
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/serial_port.hpp>
#include <thread>
#include <deque>
#include <iostream>
#include <list>
//#include "Tcp_server_handler.h"

using namespace boost;
using namespace boost::asio::ip;


class asio_generic_server
{
	struct Tcp_server_handler
	{
		asio::io_service& service_;
		asio::ip::tcp::socket socket_;
		asio::io_service::strand write_strand_;
		asio::streambuf in_packet_;
		std::deque<std::string> send_packet_queue_;
		uint32_t remote_addr_BE_;

		Tcp_server_handler(asio::io_service& service)
			: service_(service)
		, socket_(service)
		, write_strand_(service)
		{}	
		boost::asio::ip::tcp::socket& socket() {
			return socket_;
	 	}
		 void start() {}

	};

public:
	asio_generic_server(int thread_count = 1)
		: thread_count_(thread_count)
		, tcp_server_acceptor_map_()
		, tcp_server_handler_list_()
		, serial_port_(io_service_)
	{}

	// Given a local port on which to listen, this procedure
	// binds that port to a TCP socket, then creates
	// a HANDLER to stand by and receive the next connection.
	void add_tcp_server_port(uint16_t);


	void add_serial_port(std::string name);

	// Given a local PORT on which to listen, this
	// procedure binds that port to a TCP socket, then creates
	// a HANDLER to stand by to receive the next connection.
	void start_server();

	

private:

	// Given a HANDLER which is standing by, this starts up HANDLER to
	// deal with the new connection, then makes 
	// another handler to stand by for the next connection.
	void handle_new_tcp_server_connection(Tcp_server_handler& handler, system::error_code const & error);

void serial_read_handler(
  const boost::system::error_code& error, // Result of operation.
  std::size_t bytes_transferred           // Number of bytes read.
);
	int thread_count_;
	std::vector<std::thread> thread_pool_;
	asio::io_service io_service_;
	std::map<uint16_t, asio::ip::tcp::acceptor> tcp_server_acceptor_map_;
	std::list<Tcp_server_handler> tcp_server_handler_list_;

	// Serial port.  There's only one, so let's make them
	// static.
	const static size_t SERIAL_READ_BUFFER_SIZE = 8*1024;
	unsigned char serial_read_buffer_raw_[SERIAL_READ_BUFFER_SIZE];
	// asio::mutable_buffer serial_read_buffer_;
	asio::serial_port serial_port_;
};