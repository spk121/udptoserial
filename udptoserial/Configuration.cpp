// Configuration.cpp -- this parses an INI file, throwing an error
// if it doesn't exist or can't be read.

#include "Configuration.h"
#include "ini.h"
#include <cstring>
#include <exception>
#include <stdexcept>

#define CONFIG_UDP_PORT_COUNT_MAX (5)
typedef struct
{
	int baud_rate;
	int throttle_baud_rate;
	const char* serial_port_name;
	int udp_port_count;
	int udp_port[CONFIG_UDP_PORT_COUNT_MAX];
	const char *localIP;
	const char *remoteIP;
} configuration_tmp;

static int handler(void* user, const char* section, const char* name,
	const char* value)
{
	configuration_tmp* pconfig = (configuration_tmp*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("serial port", "baudrate")) {
		pconfig->baud_rate = atoi(value);
	}
	if (MATCH("serial port", "throttle")) {
		pconfig->throttle_baud_rate = atoi(value);
	}
	else if (MATCH("serial port", "name")) {
#ifdef WIN32
		pconfig->serial_port_name = _strdup(value);
#else
		pconfig->serial_port_name = strdup(value);
#endif
	}
	else if (MATCH("network", "local_ip")) {
		pconfig->localIP = strdup(value);
	}
	else if (MATCH("network", "remote_ip")) {
		pconfig->remoteIP = strdup(value);
	}
	// This matches any line that begins with "port"
	else if (strcmp(section, "udp ports") == 0 && strncmp(name, "port", 4) == 0) {
		if (pconfig->udp_port_count < CONFIG_UDP_PORT_COUNT_MAX)
			pconfig->udp_port[pconfig->udp_port_count++] = atoi(value);
	}
	else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}


Configuration::Configuration(const char* filename)
	: serial_port_name{},
	baud_rate{ 9600 },
	throttle_baud_rate{ 0 },
	port_numbers{}
{
	configuration_tmp config;
	memset(&config, 0, sizeof(config));
	if (ini_parse(filename, handler, &config) < 0) {
		std::string err = "Can't load or parse INI file '" + std::string(filename) + "':" + std::string(strerror(errno));
		throw std::runtime_error(err.c_str());
	}

	if (config.serial_port_name != NULL)
		serial_port_name = config.serial_port_name;
	if (config.localIP != NULL)
		local_ip = config.localIP;
	if (config.remoteIP != NULL)
		remote_ip = config.remoteIP;
	baud_rate = config.baud_rate;
	throttle_baud_rate = config.throttle_baud_rate;
	for (int i = 0; i < config.udp_port_count; i++)
		port_numbers.push_back(config.udp_port[i]);

	free((void *)config.serial_port_name);
	free((void *)config.localIP);
	free((void *)config.remoteIP);
}

Configuration::~Configuration()
{
}
