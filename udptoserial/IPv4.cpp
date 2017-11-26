#include "IPv4.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Winsock2.h"

struct ip4_hdr
{
	uint8_t ihl : 4;
	uint8_t version : 4;
	uint8_t tos;
	uint16BE_t tot_len;
	uint16BE_t id;
	uint16BE_t frag_off;
	uint8_t ttl;
	uint8_t protocol;
	uint16BE_t cksum;
	uint32BE_t saddr;
	uint32BE_t daddr;
};

// Return true if IH appears to be a valid IPv4 header
bool ip4_hdr_valid(struct ip4_hdr *ih)
{
	return true;
}

// Return the total length of the IPv4 header
size_t ip4_hdr_len(struct ip4_hdr *ih)
{
	return (size_t)(ih->ihl) * 4U;
}

bool ip4_is_udp(struct ip4_hdr *ih)
{
	return ih->protocol == 0x11;
}

// Update IPv4 checksum entry in the IPv4 header.
void ip4_hdr_cksum_set(struct ip4_hdr *ih)
{
	ih->cksum = 0;
	ip_cksum((uint8_t *)ih, ip4_hdr_len(ih));
}

// Compute a IP-style checksum over a list of 16-bit integers
uint16_t ip_cksum(uint8_t *u8, size_t len)
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

// Fill in default values into an IPv4 UDP packet header
void ip4_udp_headers_default_set(struct ip4_udp_hdr *hdr, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len)
{
	static uint16_t id = 0;

	// IPv4
	hdr->_ip4_hdr.version = 0x4;
	// Minimal IP header size with no optional fields
	hdr->_ip4_hdr.ihl = 0x5;
	// Type of service ?
	hdr->_ip4_hdr.tos = 0x00;
	hdr->_ip4_hdr.tot_len = sizeof(struct ip4_udp_hdr) + data_len;
	hdr->_ip4_hdr.id = htons(id++);
	// Fragment offset.  We aren't handling fragmented packets here.
	hdr->_ip4_hdr.frag_off = htons(0x0000);
	// Time-to-live aka max number of hops.
	hdr->_ip4_hdr.ttl = 64;
	// Protocol 0x11 is UDP.
	hdr->_ip4_hdr.protocol = 0x11;
	hdr->_ip4_hdr.cksum = htons(0x0000);
	hdr->_ip4_hdr.saddr = saddr;
	hdr->_ip4_hdr.daddr = daddr;
	ip4_hdr_cksum_set(&(hdr->_ip4_hdr));

	hdr->_udp_hdr.source = sport;
	hdr->_udp_hdr.dest = dport;
	hdr->_udp_hdr.len = 8 + data_len;
	udp_hdr_cksum_set(saddr, daddr, &hdr->_udp_hdr, data, data_len);
}


static int udp_send()
{
	struct udp_hdr *uh;

	uh->source = inet_sport;
	uh->dest = inet_dport;
	uh->len = htons(len);
	uh->check = 0;

	if (udp_op->no_checksum_tx)
		ucksum = CHECKSUM_NONE;
	else
		ucksum = udp_cksum();

	/* add protocol-dependent pseudo-header */
	uh->check = csum_tcpudp_magic(fl4->saddr, fl4->daddr, len,
		sk->sk_protocol, csum);
}

IPv4::IPv4()
{
}


IPv4::~IPv4()
{
}

void IPv4::to_bytes(std::vector<uint8_t>& output)
{
	// IP version 4 with minimum header
	auto start_pos = output.end();
	output.push_back(0x45);
	// DSCP and EN
	output.push_back(0x00);
	// length
	output.push_back((length & 0xFF00) >> 8);
	output.push_back(length & 0xFF);
	// Identification, and unique ID for fragmented packets.
	output.push_back(rand());
	output.push_back(rand());
	// Flags and fragment offset
	output.push_back(0x00);
	output.push_back(0x00);
	// Time-To-Live
	output.push_back(0x64);
	// Protocol is always UDP
	output.push_back(0x11);
	// Checksum -- set it to zero, then fill it in after.
	auto checksum_pos = output.end();
	output.push_back(0x00);
	output.push_back(0x00);
	// Source Address
	output.push_back((source_ip_packed & 0xFF000000) >> 24);
	output.push_back((source_ip_packed & 0xFF0000) >> 16);
	output.push_back((source_ip_packed & 0xFF00) >> 8);
	output.push_back((source_ip_packed & 0xFF));
	// Destination address
	output.push_back((dest_ip_packed & 0xFF000000) >> 24);
	output.push_back((dest_ip_packed & 0xFF0000) >> 16);
	output.push_back((dest_ip_packed & 0xFF00) >> 8);
	output.push_back((dest_ip_packed & 0xFF));

	// Now fill in the checksum.
	uint32_t sum = 0;
	for (auto i = start_pos; i != output.end(); i += 2)
	{
		sum += i[0] | (i[1] << 8);
	}
	uint16_t cksum = (sum & 0xFFFF) + ((sum & 0xFFFF0000) >> 16);
	cksum = ~cksum;
	checksum_pos[0] = (cksum & 0xFF00) >> 8;
	checksum_pos[0] = (cksum & 0xFF);

}

bool IPv4::from_bytes(std::vector<uint8_t>& bytes)
{
	if (bytes.size() < 20)
	{
		printf("IPv4 packet is too short\n");
		return false;
	}

	// Check that packet is IPv4
	if ((bytes[0] & 0xF0) != 0x40)
	{
		printf("IPv4 packet isn't version 4\n");
		return false;
	}

	header_length_words = bytes[0] & 0xFF;
	if (header_length_words < 5)
	{
		printf("IPv4 stated header length (%d words) is too short\n", header_length_words);
	}

	// Check that the length makes sense
	length = bytes[3] | (bytes[2] << 8);
	if (length != bytes.size())
	{
		printf("IPv4 packet length mismatch %d != %d\n", length, bytes.size());
		return false;
	}

	protocol = bytes[9];
	source_ip_packed = bytes[15] | (bytes[14] << 8) | (bytes[13] << 16) | (bytes[12] << 24);
	dest_ip_packed = bytes[19] | (bytes[18] << 8) | (bytes[17] << 16) | (bytes[16] << 24);

	return true;
}


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
