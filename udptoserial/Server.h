#pragma once

#ifdef WIN32
#include <sdkddkver.h>
#endif
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <thread>
#include <deque>
#include <iostream>


using namespace boost;
using namespace boost::asio::ip;


// The async loop for chat handler ls
// start-> read_packet (starts first async read) -> read_packet_done (finishes read, starts another)

class chat_handler
	: public std::enable_shared_from_this<chat_handler>
{
public:
	chat_handler(asio::io_service& service)
		: service_(service)
		, socket_(service)
		, write_strand_(service)
	{}
	boost::asio::ip::tcp::socket& socket()
	{
		return socket_;
	}
	void start()
	{
		read_packet();
	}
	void read_packet()
	{
		asio::async_read_until(socket_,
			in_packet_,
			"",  // Not waiting on any specific delimiter: send what you got.
			[me = shared_from_this()]
		(system::error_code const & ec
			, std::size_t bytes_xfer)
		{
			auto Addr = me->socket().remote_endpoint().address();
			auto Port = me->socket().remote_endpoint().port();
			auto Prot = me->socket().remote_endpoint().protocol();
			auto Addr2 = me->socket().local_endpoint().address();
			auto Port2 = me->socket().local_endpoint().port();
			auto Prot2 = me->socket().local_endpoint().protocol();
			me->read_packet_done(ec, bytes_xfer);
		});
	}
	void read_packet_done(system::error_code const & error
		, std::size_t bytes_transferred)
	{
		if (error) { std::cout << error.message(); return; }
		std::istream stream(&in_packet_);
		std::string packet_string;
		stream >> packet_string;
		printf("received packet %s\n", packet_string.c_str());
		//for (auto c : packet_string)
		//	printf("%02x ", c);
		//printf("\n");
		// do something with it
		
		// Let's try echoing it back....
		send(packet_string);


		read_packet();
	}
	void send(std::string msg)
	{
		service_.post(write_strand_.wrap([me = shared_from_this(),msg]()
		{
			me->queue_message(msg);
		}));
	}
	void packet_send_done(system::error_code const & error)
	{
		if (!error)
		{
			send_packet_queue.pop_front();
			if (!send_packet_queue.empty()) { start_packet_send(); }
		}
	}
	void start_packet_send()
	{
		send_packet_queue.front() += "\0";
		async_write(socket_
			, asio::buffer(send_packet_queue.front())
			, write_strand_.wrap([me = shared_from_this()]
			(system::error_code const & ec
				, std::size_t)
		{
			me->packet_send_done(ec);
		}
		));
	}
private:
	void queue_message(std::string message)
	{
		bool write_in_progress = !send_packet_queue.empty();
		send_packet_queue.push_back(std::move(message));
		if (!write_in_progress)
		{
			start_packet_send();
		}
	}
	asio::io_service& service_;
	asio::ip::tcp::socket socket_;
	asio::io_service::strand write_strand_;
	asio::streambuf in_packet_;
	std::deque<std::string> send_packet_queue;
};


class asio_generic_server
{
	using shared_handler_t = std::shared_ptr<chat_handler>;
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
	void start_server(uint16_t port);

private:

	// Given a HANDLER which is standing by, this starts up HANDLER to
	// deal with the new connection, then makes 
	// another handler to stand by for the next connection.
	void handle_new_connection(shared_handler_t handler, system::error_code const & error);

	int thread_count_;
	std::vector<std::thread> thread_pool_;
	asio::io_service io_service_;
	std::map<uint16_t, asio::ip::tcp::acceptor> tcp_server_acceptor_map_;
};