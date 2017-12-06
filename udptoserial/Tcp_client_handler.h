#pragma once
#ifdef WIN32
#include <sdkddkver.h>
#endif
#include <memory>
#include <deque>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/log/trivial.hpp>


using namespace boost;
using namespace boost::asio::ip;


class Tcp_client_handler
	: public std::enable_shared_from_this<Tcp_client_handler>
{
public:
	Tcp_client_handler(asio::io_service& service,
        asio::ip::tcp::endpoint& _source,
        asio::ip::tcp::endpoint& _dest);
	~Tcp_client_handler();

    // An op so can we match some input packet from the serial
    // port to this connection. 
    bool endpoints_match(const asio::ip::tcp::endpoint& _source,
        const asio::ip::tcp::endpoint& _dest)
    {
        return (endpoint_source_orig_ == _source) && (endpoint_dest_orig_ == _dest);
    }
    // If we match, send a message to the server.
    void send_to_server(const std::string& msg);

    // The async read handler.
    void read_from_server();

private:

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
    asio::ip::tcp::endpoint endpoint_source_orig_;
    asio::ip::tcp::endpoint endpoint_dest_orig_;
	asio::ip::tcp::socket socket_;
	asio::io_service::strand write_strand_;
	asio::streambuf in_packet_;
	std::deque<std::string> send_packet_queue_;
	std::shared_ptr<asio::serial_port> serial_port_;
};

