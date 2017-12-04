#include <stdexcept>
#include <cstring>
#include "ip.h"
#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Winsock2.h"
#else
#include <arpa/inet.h>
#endif

// Return true if bv appears valid
bool ip_bytevector_validate(std::vector<uint8_t>& bv)
{
	if (bv.size() < sizeof(ip_hdr))
		return false;
	if (!ip_hdr_valid((ip_hdr*) bv.data()))
		return false;
	return true;
}

// Return true if IH appears to be a valid IPv4 header
bool ip_hdr_valid(struct ip_hdr *ih)
{
	return true;
}

// Return the total length of the IPv4 header
size_t ip_hdr_len(struct ip_hdr *ih)
{
	return (size_t)(ih->ihl) * 4U;
}

bool ip_is_udp(struct ip_hdr *ih)
{
	return ih->protocol == 0x11;
}

// Compute a IP-style checksum over a list of 16-bit integers
static uint16_t ip_cksum(uint8_t *u8, size_t len)
{
	uint16_t *u16 = (uint16_t *)u8;
	size_t u16len = len / 2;
	if (len % 2 == 1)
		u16len++;
	uint32_t sum = 0;

	for (size_t i = 0; i < u16len; i++)
		sum += u16[i];

	return ~(uint16_t)((sum & 0xFFFF) + ((sum & 0xFFFF0000) >> 8));
}

// Update IPv4 checksum entry in the IPv4 header.
void ip_hdr_cksum_set(struct ip_hdr *ih)
{
	ih->cksum = 0;
	ip_cksum((uint8_t *)ih, ip_hdr_len(ih));
}



// Compute a IP-style checksum over a list of 16-bit integers
uint16_t ip_cksum_multi(int n, uint8_t **parts, size_t *lengths)
{
	uint32_t sum = 0;
	uint8_t *u8;
	size_t len;

	for (int i = 0; i < n; i++)
	{
		u8 = parts[i];
		len = lengths[i];
		uint16_t *u16 = (uint16_t *)u8;
		size_t u16len = len / 2;
		if (len % 2 == 1)
			u16len++;
		for (size_t i = 0; i < u16len; i++)
			sum += u16[i];
	}

	return ~(uint16_t)((sum & 0xFFFF) + ((sum & 0xFFFF0000) >> 8));
}

// Compute the checksum that goes in the UDP part of an IPv4 UDP packet
uint16_t udp_hdr_cksum_set(uint32BE_t saddr, uint32BE_t daddr, udp_hdr *hdr, uint8_t *data, size_t data_len)
{
	struct udp_cksum_pseudohdr phdr;
	phdr.saddr = saddr;
	phdr.daddr = daddr;
	phdr.zero = 0u;
	phdr.protocol = 0x11;
	phdr.udplen = sizeof(struct udp_hdr) + data_len;
	phdr.sport = hdr->source;
	phdr.dport = hdr->dest;
	phdr.len = hdr->len;
	phdr.cksum = 0;
	uint8_t *parts[2] = { (uint8_t *)&phdr, data };
	size_t lengths[2] = { sizeof(phdr), data_len };
	hdr->cksum = ip_cksum_multi(2, parts, lengths);
	if (hdr->cksum == 0x0)
		hdr->cksum = 0xFFFF;
	return hdr->cksum;
}

// Compute the checksum that goes in the TCP part of an IPv4 TCP packet
uint16_t tcp_hdr_cksum_set(uint32BE_t saddr, uint32BE_t daddr, tcp_hdr *hdr, uint8_t *data, size_t data_len)
{
	struct tcp_cksum_pseudohdr phdr;
	phdr.saddr = saddr;
	phdr.daddr = daddr;
	phdr.zero = 0u;
	phdr.protocol = 0x06;
	phdr.tcp_segment_length = sizeof(struct tcp_hdr) + data_len;
	uint8_t *parts[2] = { (uint8_t *)&phdr, data };
	size_t lengths[2] = { sizeof(phdr), data_len };
	hdr->checksum = ip_cksum_multi(2, parts, lengths);
	if (hdr->checksum == 0x0)
		hdr->checksum = 0xFFFF;
	return hdr->checksum;
}


// Fill in default values into an IPv4 packet header
void ip_headers_default_set(struct ip_hdr *hdr, uint8_t protocol, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len)
{
	static uint16_t id = 0;

	// IPv4
	hdr->version = 0x4;
	// Minimal IP header size with no optional fields
	hdr->ihl = 0x5;
	// Type of service
	// zero means routine precedence, normal delay, normal throughput, normal reliability
	hdr->type_of_service = 0x00;
	// Length of the datagram: header + data, up to 64k bytes
	if (data_len + sizeof(struct ip_hdr) > UINT16_MAX)
		throw std::runtime_error("oversize payload for IPv4 packet");
	hdr->total_length = sizeof(struct ip_udp_hdr) + data_len;
	// Identifying value assigned by the sender to help reconstructing
	// fragmented packets.
	hdr->identification = htons(id++);
	// Fragment offset.  We aren't handling fragmented packets here.
	hdr->frag_off = htons(0x0000);
	// Time-to-live aka max number of hops.
	hdr->time_to_live = 64;
	// Protocol 0x11 is UDP.
	hdr->protocol = protocol;
	hdr->cksum = htons(0x0000);
	hdr->saddr = saddr;
	hdr->daddr = daddr;
	ip_hdr_cksum_set(hdr);
}

// Fill in default values into an IPv4 + UDP packet header.
// We do the bare minimum to make it convincing.
void ip_udp_headers_default_set(struct ip_hdr *ip_hdr, struct udp_hdr *udp_hdr, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len)
{
	static uint16_t id = 0;

	// Prepend the UDP packet header to DATA
	memset(udp_hdr, 0, sizeof(struct udp_hdr));
	udp_hdr->source = sport;
	udp_hdr->dest = dport;
	udp_hdr->len = data_len + sizeof(udp_hdr);
	udp_hdr->cksum = 0;
	udp_hdr_cksum_set(saddr, daddr, udp_hdr, data, data_len);

	uint8_t *data2 = (uint8_t*)malloc(data_len + sizeof(udp_hdr));
	memcpy(data2, udp_hdr, sizeof(struct udp_hdr));
	memcpy(data2 + sizeof(udp_hdr), data, data_len);
	ip_headers_default_set(ip_hdr, IPV4_PROTOCOL_UDP, saddr, sport, daddr, dport, data2, data_len + sizeof(udp_hdr));
	free(data2);
}

// Fill in default values into an IPv4 + TCP packet header.
// We do the bare minimum to make it convincing.
void ip_tcp_headers_default_set(struct ip_hdr *ip_hdr, struct tcp_hdr *tcp_hdr, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len)
{
	// Prepend the TCP packet header to DATA

	memset(tcp_hdr, 0, sizeof(struct tcp_hdr));
	tcp_hdr->source_port = sport;
	tcp_hdr->destination_port = dport;

	tcp_hdr->sequence_number = 0;
	tcp_hdr->acknowledgement_number = 0;
	tcp_hdr->data_offset = 0;
	tcp_hdr->window = UINT16_MAX;
	tcp_hdr->checksum = 0;
	tcp_hdr->urgent_pointer = 0;
	tcp_hdr_cksum_set(saddr, daddr, tcp_hdr, data, data_len);

	uint8_t *data2 = (uint8_t*)malloc(data_len + sizeof(tcp_hdr));
	memcpy(data2, tcp_hdr, sizeof(struct tcp_hdr));
	memcpy(data2 + sizeof(tcp_hdr), data, data_len);
	ip_headers_default_set(ip_hdr, IPV4_PROTOCOL_TCP, saddr, sport, daddr, dport, data2, data_len + sizeof(udp_hdr));
	free(data2);
}

#if 0
// Test
bool ipv4_test()
{
	// source ip 192.168.1.202
	// dest ip 192.168.1.93
	// protocol TCP
	// length 60
	std::vector<uint8_t> vec = {0x45, 0x00, 0x00, 0x3c, 0x6d, 0x80, 0x40, 0x00,
		0x40, 0x06, 0x48, 0xc4, 0xc0, 0xa8, 0x01, 0xca,
		0xc0, 0xa8, 0x01, 0x5d, 0xab, 0x9a, 0x00, 0x50,
		0x6e, 0x5f, 0x56, 0x15, 0x00, 0x00, 0x00, 0x00, 
		0xa0, 0x02, 0x14, 0x00, 0x89, 0xd3, 0x00, 0x00, 
		0x02, 0x04, 0x01, 0x00 ,0x04, 0x02, 0x08, 0x0a,
		0x5c, 0x65, 0x5d, 0xa4, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x03, 0x03, 0x07 };

	IPv4 header;
	header.from_bytes(vec);
	unsigned long test_long = inet_addr("192.168.1.202");
	in_addr test_addr;
	test_addr.S_un.S_addr = test_long;

	in_addr measured_addr;
	measured_addr.S_un.S_addr = htonl(header.source_ip_packed);
	char *str = inet_ntoa(measured_addr);

	int test1 = strcmp(str, "192.168.1.202");

	//char *str2 = inet_ntoa((in_addr)header.dest_ip_packed);
	return true;
}
#endif
