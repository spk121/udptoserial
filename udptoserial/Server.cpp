#include "Server.h"

// Given a local PORT on which to listen, this
// procedure binds that port to a TCP socket, then creates
// a HANDLER to stand by to receive the next connection.
template <typename ConnectionHandler>
void asio_generic_server::start_server(uint16_t port)
{
	auto handler
		= std::make_shared<ConnectionHandler>(io_service_);
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


// Given a currently unstarted HANDLER, this starts up HANDLER then makes 
// the next handler.
template <typename ConnectionHandler>
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
	auto new_handler = std::make_shared<ConnectionHandler>(io_service_);
	acceptor_.async_accept(new_handler->socket()
		, [=](auto ec)
	{
		handle_new_connection(new_handler, ec);
	}
	);
}