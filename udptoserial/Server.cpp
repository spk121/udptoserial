#include "Server.h"

#include <functional>
using namespace std::placeholders;


#include <boost/log/trivial.hpp>

// Given a local PORT on which to listen, this
// procedure binds that port to a TCP socket, then creates
// a HANDLER to stand by to receive the next connection.
void asio_generic_server::add_tcp_server_port(uint16_t port)
{
	BOOST_LOG_TRIVIAL(debug) << "Adding new TCP server port " << port;

	// Let's set up the ASIO acceptor to listen on the TCP server port.
	// There is only one acceptor per port.
	auto search = tcp_server_acceptor_map_.find(port);
	if (search != tcp_server_acceptor_map_.end())
		return;

	// We start off making an acceptor that we will later use to join a handler
	// to new connections on this port.
	asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
	// Stuff a new empty acceptor into the port-to-acceptor map.
	tcp_server_acceptor_map_.emplace(std::make_pair(port, asio::ip::tcp::acceptor(io_service_)));
	// Bind that acceptor to be a TCP server socket on this port.
	auto& acceptor = tcp_server_acceptor_map_.at(port);
	acceptor.open(endpoint.protocol());
	acceptor.set_option(tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();

	// Here we actually construct the first handler on the heap.
	// This is a strange thing that ASIO people like to do.
	// So the handler doesn't actually appear as a private member of the parent
	// server class.  This 'make_shared' stuff is part of 
	// how the lifetime of the handler is managed.  If 
	// this handler ever returns from one of its
	// methods, it is done and is automatically removed.
	// Remember that a TCP port can have many clients, so
	// the number of handlers for each port can vary, which
	// is why you can't just pair up a port and a handler.
	auto handler
		= std::make_shared<Tcp_server_handler>(io_service_);
	BOOST_LOG_TRIVIAL(debug) << "Adding first handler to new TCP server port " << port;

	// Kick off a new handler to stand by to receive the next new
	// TCP client connection.
#if 1
	// Ugh. Lambda
	acceptor.async_accept(handler->socket()
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(handler, ec);
	}
	);
#if 0
	tcp_server_acceptor_map_.at(port).async_accept(handler->socket()
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(handler, ec);
	}
	);
#endif
#else
	// FIXME: how do you do this with bind to avoid the lambda?
	auto handler_func = std::bind(&asio_generic_server::handle_new_connection, handler, _1);
	acceptor.async_accept(handler->socket(), handler_func);
#endif



}

void asio_generic_server::start_server()
{
	BOOST_LOG_TRIVIAL(debug) << "Calling I/O Service run.";
	io_service_.run();
}

// Given a currently unstarted HANDLER, this starts up HANDLER then makes 
// the next handler.
void asio_generic_server::handle_new_tcp_server_connection(shared_tcp_server_handler_t handler, system::error_code const & error)
{
	if (error)
	{ 
		BOOST_LOG_TRIVIAL(error) << error.message();
		return;
	}

	// Start up the waiting handler.
	BOOST_LOG_TRIVIAL(debug) << "Starting handler on TCP server " << handler->socket().remote_endpoint().port() << " -> " << handler->socket().local_endpoint().port();
	handler->start();

	// Queue up a new handler for the next connection..
	auto new_handler = std::make_shared<Tcp_server_handler>(io_service_);
	BOOST_LOG_TRIVIAL(debug) << "Adding handler to TCP server port " << handler->socket().local_endpoint().port();
	tcp_server_acceptor_map_.at(handler->socket().local_endpoint().port()).async_accept(new_handler->socket()
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(new_handler, ec);
	}
	);
}