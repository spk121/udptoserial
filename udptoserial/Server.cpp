#include "Server.h"

#include <functional>
using namespace std::placeholders;

#if 0
// Given a local PORT on which to listen, this
// procedure binds that port to a TCP socket, then creates
// a HANDLER to stand by to receive the next connection.
void asio_generic_server::start_server(uint16_t port)
{
	auto handler
		= std::make_shared<chat_handler>(io_service_);
	// set up the acceptor to listen on the tcp port
	asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	// Kick off a new handler to stand by to receive the next new
	// connection.
	acceptor_.async_accept(handler->socket()
		, [=](auto ec)
	{
		handle_new_connection(handler, ec);
	}
	);
#if 0
	// start pool of threads to process the asio events
	for (int i = 0; i < thread_count_; ++i)
	{
		thread_pool_.emplace_back([=] {io_service_.run(); });
	}
#else
	io_service_.run();
#endif
}
#endif

// Given a local PORT on which to listen, this
// procedure binds that port to a TCP socket, then creates
// a HANDLER to stand by to receive the next connection.
void asio_generic_server::add_tcp_server_port(uint16_t port)
{
	// Let's set up the ASIO acceptor to listen on the TCP server port.
	// There is only one acceptor per port.
	auto search = tcp_server_acceptor_map_.find(port);
	if (search != tcp_server_acceptor_map_.end())
		return;

	asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
	tcp_server_acceptor_map_.emplace(std::make_pair(port, asio::ip::tcp::acceptor(io_service_)));
	auto& acceptor = tcp_server_acceptor_map_[port];
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
		= std::make_shared<chat_handler>(io_service_);
	// tcp_server_acceptor_map_.insert_or_assign(port, acceptor);

	// Kick off a new handler to stand by to receive the next new
	// TCP client connection.
#if 1
	// Ugh. Lambda
	tcp_server_acceptor_map_[port].async_accept(handler->socket()
		, [=](auto ec)
	{
		handle_new_connection(handler, ec);
	}
	);
#else
	auto handler_func = std::bind(&asio_generic_server::handle_new_connection, handler, _1);
	acceptor.async_accept(handler->socket(), handler_func);
#endif


	io_service_.run();
}

// Given a currently unstarted HANDLER, this starts up HANDLER then makes 
// the next handler.
void asio_generic_server::handle_new_connection(shared_handler_t handler, system::error_code const & error)
{
	if (error)
	{ 
		std::cout << error.message(); 
		return;
	}

	// Start up the waiting handler.
	handler->start();

	// Queue up a new handler for the next connection..
	auto new_handler = std::make_shared<chat_handler>(io_service_);
	auto search = tcp_server_acceptor_map_.find(handler->socket().local_endpoint().port());
	if (search != tcp_server_acceptor_map_.end())
	{
		auto acceptor = search->second;
		acceptor.async_accept(new_handler->socket()
			, [=](auto ec)
		{
			handle_new_connection(new_handler, ec);
		}
		);
	}

}