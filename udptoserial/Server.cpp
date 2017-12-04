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


	// Tcp_server_handler H(io_service_);
	tcp_server_handler_list_.emplace_back(io_service_);

	Tcp_server_handler& handler = tcp_server_handler_list_.back();
#if 0

	// Ugh. Lambda
	acceptor.async_accept(handler.socket()
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(handler, ec);
	}
	);
	acceptor.async_accept(handler.socket(), handle_func);
#endif
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
	//auto handler
	//	= std::make_shared<Tcp_server_handler>(io_service_);
	BOOST_LOG_TRIVIAL(debug) << "Adding first handler to new TCP server port " << port;

	// Kick off a new handler to stand by to receive the next new
	// TCP client connection.
#if 0
	// Ugh. Lambda
	acceptor.async_accept(handler->socket()
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(handler, ec);
	}
	);
#endif
#if 0
	tcp_server_acceptor_map_.at(port).async_accept(handler->socket()
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(handler, ec);
	}
	);
#endif
#if 0
	// FIXME: how do you do this with bind to avoid the lambda?
	auto handler_func = std::bind(&asio_generic_server::handle_new_connection, handler, _1);
	acceptor.async_accept(handler->socket(), handler_func);
#endif
}

#if 0

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
#endif

void asio_generic_server::serial_read_handler(
  const boost::system::error_code& error, // Result of operation.
  std::size_t bytes_transferred           // Number of bytes read.
)
{
	BOOST_LOG_TRIVIAL(debug) << "serial port got " << bytes_transferred << " bytes";

	// And queue up the next async read
	auto handler = std::bind(&asio_generic_server::serial_read_handler, this,
		_1, _2);

	serial_port_.async_read_some
	(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
	handler);
}

void asio_generic_server::add_serial_port (std::string port)
{
	using namespace std::placeholders;
	BOOST_LOG_TRIVIAL(debug) << "Adding new serial port port " << port;

	serial_port_.open(port);
	serial_port_.set_option(asio::serial_port_base::baud_rate(115200));

	auto handler = std::bind(&asio_generic_server::serial_read_handler, this,
		_1, _2);

	serial_port_.async_read_some
	(asio::mutable_buffers_1(serial_read_buffer_raw_, SERIAL_READ_BUFFER_SIZE),
	handler);
}
#if 0
void asio_generic_server::add_serial_port(std::string port)
{
	BOOST_LOG_TRIVIAL(debug) << "Adding new serial port port " << port;

	asio::serial_port sport(io_service_);

	// Let's set up the ASIO acceptor to listen on the TCP server port.
	// There is only one acceptor per port.
	sport.open(port);



	port_->async_read_some( 
		boost::asio::buffer(read_buf_raw_, SERIAL_PORT_READ_BUF_SIZE),
		boost::bind(
			&SerialPort::on_receive_,
			this, boost::asio::placeholders::error, 
boost::asio::placeholders::bytes_transferred));
	sport.async_read_some(sport
		, [=](auto ec)
	{
		handle_new_tcp_server_connection(handler, ec);
	}
	);

	sport.
	acceptor.listen();

}

#endif

void asio_generic_server::start_server()
{
	BOOST_LOG_TRIVIAL(debug) << "Calling I/O Service run.";
	io_service_.run();
}




// Given a currently unstarted HANDLER, this starts up HANDLER then makes 
// the next handler.
#if 1
void asio_generic_server::handle_new_tcp_server_connection(Tcp_server_handler& handler, system::error_code const & error)
{
	if (error)
	{ 
		BOOST_LOG_TRIVIAL(error) << error.message();
		return;
	}

	// Start up the waiting handler.
	// BOOST_LOG_TRIVIAL(debug) << "Starting handler on TCP server " << handler.socket().remote_endpoint().port() << " -> " << handler.socket().local_endpoint().port();
	// handler.start();


	// Queue up a new handler for the next connection..
	// uint16_t port = handler.socket().local_endpoint().port();
	// tcp_server_handler_list_.emplace_back(io_service_);
	// auto& handler2 = tcp_server_handler_list_.back();
	// auto handle_func = std::bind(&asio_generic_server::handle_new_tcp_server_connection,
	//	this, handler2, _1);
	// auto& acceptor = tcp_server_acceptor_map_.at(port);
	// acceptor.async_accept(handler.socket(), handle_func);

}
#endif