#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>

class Configuration
{
public:
	Configuration(const char* filename);
	~Configuration();

	std::string serial_port_name;
	uint32_t baud_rate;
	uint32_t throttle_baud_rate;
	std::vector<uint16_t> port_numbers;
	std::string local_ip;
	std::string remote_ip;
};

