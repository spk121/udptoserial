#include "Tcp_client_handler.h"
//#include "IPv4.h"
//#include "../libhorizr/slip.h"
#include "Serial_port.h"

void serial_port_send(asio::ip::tcp::endpoint& src, asio::ip::tcp::endpoint& dest, std::string binary_string);

Tcp_client_handler::Tcp_client_handler(asio::io_service& service,
        asio::ip::tcp::endpoint& _source,
        asio::ip::tcp::endpoint& _dest)	
    : service_(service)
	, socket_(service)
	, write_strand_(service)
	, endpoint_source_orig_(_source)
    , endpoint_dest_orig_(_dest)
{
	BOOST_LOG_TRIVIAL(debug) << "TCP client handler constructed for "
        << _source.address().to_string() << ":" << _source.port()
        << " -> " << _dest.address().to_string() << ":" << _dest.port();
        
}

Tcp_client_handler::~Tcp_client_handler()
{
	puts("TCP client handler destructed");
}

// When info comes from the slow serial port to the fast internal
// networking, we ship it ASAP.  No async here.
void Tcp_client_handler::send_to_remote(const std::string& msg)
{
	BOOST_LOG_TRIVIAL(debug) << "Send to TCP server"
    << " orig " << endpoint_source_orig_.address().to_string() << ":" << endpoint_source_orig_.port()
    << " dest " << endpoint_dest_orig_.address().to_string() << ":" << endpoint_dest_orig_.port();

    // just ship it ASAP
    socket_.send(asio::const_buffers_1(msg.c_str(), msg.size()));
}

// This is the beginning of
void Tcp_client_handler::read_from_server()
{
	BOOST_LOG_TRIVIAL(debug) << "read_packet called";
	asio::async_read_until(socket_,
		in_packet_,
		"",  // Not waiting on any specific delimiter: send what you got.
		[me = shared_from_this()]
	(system::error_code const & ec
		, std::size_t bytes_xfer)
	{

		me->read_from_server_done(ec, bytes_xfer);
	});
}

void Tcp_client_handler::read_from_server_done(system::error_code const & error, std::size_t bytes_transferred)
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

    // Enqueue this packet onto the serial port
    serial_port_send(endpoint_source_orig_, endpoint_dest_orig, packet_string);

    // Get ready to handle another message
    read_from_server();
}
#if 0

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
		htonl(socket_.remote_endpoint().address().to_v4().to_ulong()),
		htons(socket_.remote_endpoint().port()),
		htonl(remote_addr_BE_),
		htons(socket_.local_endpoint().port()),
		(uint8_t *)packet_string.c_str(), packet_string.size());
	uint8_t *binary_msg = (uint8_t *)malloc(sizeof (iphdr) + sizeof(tcphdr) + packet_string.size());
	memcpy(binary_msg, &iphdr, sizeof(iphdr));
	memcpy(binary_msg + sizeof(iphdr), &tcphdr, sizeof(tcphdr));
	memcpy(binary_msg + sizeof(iphdr) + sizeof(tcphdr), packet_string.c_str(), packet_string.size());
	std::string binary_str((char *)binary_msg, sizeof(iphdr) + sizeof(tcphdr) + packet_string.size());
	free(binary_msg);
	printf("----\n");
	for (auto c : binary_str)
	{
		printf("%02x ", (unsigned)c);
	}
	printf("\n");
	for (auto c : binary_str)
	{
		if (c < 32)
			printf("^%c",c+64);
		else
			printf("%c", c);
	}
	printf("\n");
	std::vector<uint8_t> source;
	for (auto x: binary_str)
		source.push_back((uint8_t)x);
	std::vector<uint8_t> dest;
    slip_encode(dest, source, true);	
	std::string dest_str;
	for (auto x: dest)
		dest_str.push_back(x);
	send(dest_str);
	read_packet();
}

void Tcp_client_handler::send(std::string msg)
{
	service_.post(write_strand_.wrap([me = shared_from_this(), msg]()
	{
		me->queue_message(msg);
	}));
}


void Tcp_client_handler::packet_send_done(system::error_code const & error)
{
	if (!error)
	{
		send_packet_queue_.pop_front();
		if (!send_packet_queue_.empty()) { start_packet_send(); }
	}
}

void Tcp_client_handler::start_packet_send()
{
	send_packet_queue_.front() += "\0";
	async_write(*serial_port_
		, asio::buffer(send_packet_queue_.front())
		, write_strand_.wrap([me = shared_from_this()]
		(system::error_code const & ec
			, std::size_t)
	{
		me->packet_send_done(ec);
	}
	));
}

void Tcp_client_handler::queue_message(std::string message)
{
	bool write_in_progress = !send_packet_queue_.empty();
	send_packet_queue_.push_back(std::move(message));
	if (!write_in_progress)
	{
		start_packet_send();
	}
}

#endif