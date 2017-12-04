#pragma once
// This is a minimal parser for IPv4 headers

#include <cstdint>
#include <vector>

// Big-endian numbers
typedef uint16_t uint16BE_t;
typedef uint32_t uint32BE_t;

struct ip4_hdr
{
	uint8_t ihl : 4;
	uint8_t version : 4;
	uint8_t type_of_service;
	uint16BE_t total_length;
	uint16BE_t identification;
	uint16BE_t frag_off;
	uint8_t time_to_live;
	uint8_t protocol;
	uint16BE_t cksum;
	uint32BE_t saddr;
	uint32BE_t daddr;
};

struct udp_hdr
{
	uint16BE_t source;
	uint16BE_t dest;
	uint16BE_t len;
	uint16BE_t cksum;
};

struct tcp_hdr
{
	uint16BE_t source_port;
	uint16BE_t destination_port;
	uint32BE_t sequence_number;
	uint32BE_t acknowledgement_number;
	uint16BE_t data_offset;
	uint16BE_t window;
	uint16BE_t checksum;
	uint16BE_t urgent_pointer;
};

struct ip4_udp_hdr
{
	struct ip4_hdr _ip4_hdr;
	struct udp_hdr _udp_hdr;
};

struct udp_cksum_pseudohdr
{
	uint32BE_t saddr;
	uint32BE_t daddr;
	uint8_t zero;
	uint8_t protocol;
	uint16BE_t udplen;
	uint16BE_t sport;
	uint16BE_t dport;
	uint16BE_t len;
	uint16BE_t cksum;
};

struct tcp_cksum_pseudohdr
{
	uint32BE_t saddr;
	uint32BE_t daddr;
	uint8_t zero;
	uint8_t protocol;
	uint16BE_t tcp_segment_length;  // Length off both header and data
};

#define IPV4_PROTOCOL_UDP 17
#define IPV4_PROTOCOL_TCP 6

bool ip4_hdr_valid(struct ip4_hdr *ih);
size_t ip4_hdr_len(struct ip4_hdr *ih);
bool ip4_is_udp(struct ip4_hdr *ih);
void ip4_hdr_cksum_set(struct ip4_hdr *ih);
uint16_t udp4_hdr_cksum_set(uint32BE_t saddr, uint32BE_t daddr, udp_hdr *hdr, uint8_t *data, size_t data_len);
uint16_t tcp4_hdr_cksum_set(uint32BE_t saddr, uint32BE_t daddr, tcp_hdr *hdr, uint8_t *data, size_t data_len);
void ip4_headers_default_set(struct ip4_hdr *hdr, uint8_t protocol, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len);
void ip4_udp_headers_default_set(struct ip4_hdr *ip_hdr, struct udp_hdr *udp_hdr, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len);
void ip4_tcp_headers_default_set(struct ip4_hdr *ip_hdr, struct tcp_hdr *udp_hdr, uint32BE_t saddr, uint16BE_t sport, uint32BE_t daddr, uint16BE_t dport, uint8_t *data, size_t data_len);

class IPv4
{
public:
	IPv4();
	~IPv4();

	bool from_bytes(std::vector<uint8_t>& bytes);
	void to_bytes(std::vector<uint8_t>& bytes);

	uint8_t header_length_words;
	uint16_t length;
	uint8_t protocol;
	uint32_t source_ip_packed;
	uint32_t dest_ip_packed;
	std::vector<uint8_t> payload;
};

bool ipv4_test();
