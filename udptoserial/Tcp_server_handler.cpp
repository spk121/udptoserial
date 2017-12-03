#include "Tcp_server_handler.h"
#include "IPv4.h"

Tcp_server_handler::Tcp_server_handler(asio::io_service & service)
	: service_(service)
	, socket_(service)
	, write_strand_(service)
{
}

Tcp_server_handler::~Tcp_server_handler()
{
}

void Tcp_server_handler::read_packet()
{
	BOOST_LOG_TRIVIAL(debug) << "read_packet called";
	asio::async_read_until(socket_,
		in_packet_,
		"",  // Not waiting on any specific delimiter: send what you got.
		[me = shared_from_this()]
	(system::error_code const & ec
		, std::size_t bytes_xfer)
	{

		me->read_packet_done(ec, bytes_xfer);
	});
}



void Tcp_server_handler::read_packet_done(system::error_code const & error, std::size_t bytes_transferred)
{
	if (error)
	{ 
		BOOST_LOG_TRIVIAL(error) << error.message();
		return;
	}
	std::istream stream(&in_packet_);
	std::string packet_string;
	stream >> packet_string;
	BOOST_LOG_TRIVIAL(debug) << "received packet "<< socket_.remote_endpoint().address() << ":" << socket_.remote_endpoint().port() << " -> " << socket_.local_endpoint().address() << ":" << socket_.local_endpoint().port();

	// Now we prep this for transmission, using the following mapping.
	// From our point of view, this socket is local_address:server_port.
	// It is a forwarder for a server on destination_address:server_port.
	// It received a packet from remote_address:client_port.
	// The packet's SOURCE is remote_address:client_port.
	// The packet's DEST is local_address:server_port.

	// When we construct a forwarding packet
	// That packet's SOURCE is remote_address:client_port
	// That packet's DEST is destination_address:server_port

	// destination_address is from the .ini file 
	struct ip4_hdr iphdr;
	struct tcp_hdr tcphdr;
	ip4_tcp_headers_default_set(&iphdr, &tcphdr,
		socket_.remote_endpoint().address().to_v4().to_ulong(), socket_.remote_endpoint().port(),
		remote_addr_BE_, socket_.local_endpoint().port(),
		(uint8_t *)packet_string.c_str(), packet_string.size());
	uint8_t *binary_msg = (uint8_t *)malloc(sizeof (iphdr) + sizeof(tcphdr) + packet_string.size());
	memcpy(binary_msg, &iphdr, sizeof(iphdr));
	memcpy(binary_msg + sizeof(iphdr), &tcphdr, sizeof(tcphdr));
	memcpy(binary_msg + sizeof(iphdr) + sizeof(tcphdr), packet_string.c_str(), packet_string.size());
	std::string binary_str((char *)binary_msg, sizeof(iphdr) + sizeof(tcphdr) + packet_string.size());
	free(binary_msg);
	for (auto c : binary_str)
	{
		if (isprint(c))
			printf("%02x '%c'", c, c);
		else
			printf("%02x ", c);
	}
	read_packet();
}

void Tcp_server_handler::send(std::string msg)
{
	service_.post(write_strand_.wrap([me = shared_from_this(), msg]()
	{
		me->queue_message(msg);
	}));
}


void Tcp_server_handler::packet_send_done(system::error_code const & error)
{
	if (!error)
	{
		send_packet_queue_.pop_front();
		if (!send_packet_queue_.empty()) { start_packet_send(); }
	}
}

void Tcp_server_handler::start_packet_send()
{
	send_packet_queue_.front() += "\0";
	async_write(socket_
		, asio::buffer(send_packet_queue_.front())
		, write_strand_.wrap([me = shared_from_this()]
		(system::error_code const & ec
			, std::size_t)
	{
		me->packet_send_done(ec);
	}
	));
}

void Tcp_server_handler::queue_message(std::string message)
{
	bool write_in_progress = !send_packet_queue_.empty();
	send_packet_queue_.push_back(std::move(message));
	if (!write_in_progress)
	{
		start_packet_send();
	}
}