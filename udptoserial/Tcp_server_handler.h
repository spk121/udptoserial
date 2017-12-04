#pragma once
#ifdef WIN32
#include <sdkddkver.h>
#endif
#include <memory>
#include <deque>
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/log/trivial.hpp>

using namespace boost;
using namespace boost::asio::ip;


class Tcp_server_handler
	: public std::enable_shared_from_this<Tcp_server_handler>
{
public:
	Tcp_server_handler(asio::io_service& service, std::shared_ptr<asio::serial_port> sport) ;
	~Tcp_server_handler();

	boost::asio::ip::tcp::socket& socket()
	{
		return socket_;
	}
	void start()
	{
		read_packet();
	}
	void read_packet();
	void read_packet_done(system::error_code const & error, std::size_t bytes_transferred);
	void send(std::string msg);
	void packet_send_done(system::error_code const & error);
	void start_packet_send();

private:

	void queue_message(std::string message);

	asio::io_service& service_;
	asio::ip::tcp::socket socket_;
	asio::io_service::strand write_strand_;
	asio::streambuf in_packet_;
	std::deque<std::string> send_packet_queue_;
	uint32_t remote_addr_BE_;
	std::shared_ptr<asio::serial_port> serial_port_;
};

