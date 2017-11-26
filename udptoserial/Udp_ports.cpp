#include "Udp_ports.h"


Udp_ports::Udp_ports()
	: port_map{}
{
	hostent* localHost;

	localHost = gethostbyname("");
	localIP = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);
}

Udp_ports::Udp_ports(std::string _local_ip, std::string _remote_ip, std::vector<uint16_t> _port_numbers)
	: port_map{},
	default_local_ip{ _local_ip },
	default_remove_ip{ _remote_ip }
{
	for (auto n : _port_numbers)
		port_map.emplace(n, 0);
}


Udp_ports::~Udp_ports()
{
	for (auto& s : port_map)
		closesocket(s.second);
	port_map.clear();
}

static void throw_win32_sock_error(const char *func_name)
{
	// Retrieve the system error message for the last-error code
	// and throw an exception.

	const size_t message_len_max = 8 * 1024;
	char system_message[message_len_max];
	char output_message[message_len_max];
	int dw = WSAGetLastError();

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		system_message,
		message_len_max, NULL);

	// Display the error message and exit the process
	snprintf(output_message, message_len_max, "%s failed with error %d: %s", func_name, dw, system_message);
	throw std::exception(output_message);
}

void Udp_ports::open()
{
	for (auto p : port_map)
	{
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET)
		{
			std::string errmsg = "Opening socket for port " + p.first;
			throw_win32_sock_error(errmsg.c_str());
		}

		unsigned long mode = 1;
		int result = ioctlsocket(sock, FIONBIO, &mode);
		if (result == SOCKET_ERROR)
		{
			std::string errmsg = "Setting non-blocking mode for for port " + p.first;
			throw_win32_sock_error(errmsg.c_str());
		}
		struct sockaddr_in name;
		memset(&name, 0, sizeof(struct sockaddr_in));
		name.sin_family = AF_INET;
		name.sin_port = htons(p.first);
		name.sin_addr.s_addr = htonl(INADDR_ANY);

		int bound = bind(sock, (struct sockaddr *)&name, sizeof(struct sockaddr_in));
		if (bound == SOCKET_ERROR)
		{
			std::string errmsg = "Binding socket for port " + p.first;
			throw_win32_sock_error(errmsg.c_str());
		}

		// Add the newly opened port to the hash table.
		port_map[p.first] = sock;
	}
}

void Udp_ports::send_packet(udp_packet & P)
{
	// Hopefully, this packet comes from a known port.
	auto search = port_map.find(P.source_port);
	if (search != port_map.end())
	{
		static sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(P.dest_port);
		addr.sin_addr.s_addr = inet_addr(localIP);
		std::vector<uint8_t> bytes = P.to_bytes();
		sendto(search->second, (const char*)bytes.data(), bytes.size(), 0, (const sockaddr *)&addr, sizeof(sockaddr_in));
	}
}


std::vector<uint8_t> Udp_ports::recv_bytes()
{
	char buf[1500];
	struct sockaddr_in inaddr;
	int addrlen = sizeof(inaddr);
	SSIZE_T bytes_received;
	std::vector<uint8_t> output;

	for (auto P : port_map)
	{
		u_long count = 0;

		int ret = ioctlsocket(P.second, FIONREAD, &count);
		if (count > 0)
		{
			bytes_received = recvfrom(P.second, buf, 1500, 0 /* = read */,
				(struct sockaddr *)&inaddr, &addrlen);

			if (bytes_received > 0)
			{
				std::vector<uint8_t> data;
				for (size_t i = 0; i < bytes_received; i++)
				{
					data.push_back(buf[i]);
				}
				std::vector<uint8_t> pkt_bytes = udp_packet(inaddr.sin_port, P.first, data).to_bytes();
				output.insert(output.end(), pkt_bytes.begin(), pkt_bytes.end());

			}
			else if (bytes_received < 0)
				throw_win32_sock_error("Reading from udp port");
		}
	}
	return output;
}
