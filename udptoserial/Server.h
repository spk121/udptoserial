#pragma once

#ifdef WIN32
#include <sdkddkver.h>
#endif
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <thread>
#include <deque>
#include <iostream>
#include "Tcp_server_handler.h"

using namespace boost;
using namespace boost::asio::ip;


class asio_generic_server
{
	using shared_tcp_server_handler_t = std::shared_ptr<Tcp_server_handler>;
public:
	asio_generic_server(int thread_count = 1)
		: thread_count_(thread_count)
		, tcp_server_acceptor_map_()
	{}

	// Given a local port on which to listen, this procedure
	// binds that port to a TCP socket, then creates
	// a HANDLER to stand by and receive the next connection.
	void add_tcp_server_port(uint16_t);

	// Given a local PORT on which to listen, this
	// procedure binds that port to a TCP socket, then creates
	// a HANDLER to stand by to receive the next connection.
	void start_server();

	

private:

	// Given a HANDLER which is standing by, this starts up HANDLER to
	// deal with the new connection, then makes 
	// another handler to stand by for the next connection.
	void handle_new_tcp_server_connection(shared_tcp_server_handler_t handler, system::error_code const & error);

	int thread_count_;
	std::vector<std::thread> thread_pool_;
	asio::io_service io_service_;
	std::map<uint16_t, asio::ip::tcp::acceptor> tcp_server_acceptor_map_;
};