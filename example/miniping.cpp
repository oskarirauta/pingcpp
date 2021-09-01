#include <iostream>
#include <string>
#include "example/report.hpp"
#include "network.hpp"
#include "ping.hpp"

#define PING_HOST "google.com"

bool failed = false;

#ifdef __IPV6__
# define TITLE "miniping(+IPV6), by Oskari Rauta\nMIT License\n"
#else
# define TITLE "miniping, by Oskari Rauta\nMIT License\n"
#endif

int main(int argc, char *argv[]) {

	std::cout << TITLE << std::endl;

	std::string arg = argc > 1 ? std::string(argv[1]) : "";

	network::protocol proto =
		arg == "-6" ? network::protocol::IPV6 :
			( arg == "-4" ? network::protocol::IPV4 :
				network::protocol::ANY );

	std::string host = PING_HOST;

	std::cout << TITLE << std::endl;
	std::cout << "Attempting to ping host " << host << " using ";
	std::cout << ( proto == network::protocol::IPV6 ? "IPv6" : (
			proto == network::protocol::IPV4 ? "IPv4" : "ANY" ));
	std::cout << " protocol" << std::endl;

	network::ping_t ping(host, proto);

	if ( ping.connection -> protocol == network::protocol::ANY ) {
		std::cout << "error: IP address failure, or unsupported protocol for host " << host << std::endl;
		return -1;
	} else if ( ping.address_error()) {
		std::cout << "error: address information for host " << host << " is not available" << std::endl;
		return -1;
	}

	ping.packetsize = 56; // standard icmp packet size
	ping.delay = std::chrono::milliseconds(500);
	ping.timeout = std::chrono::seconds(15);
	ping.count_min = 10;
	ping.count_max = 99;
	ping.report = report;

	std::cout << "Host's ip address is " << ping.connection -> ipaddr << " on ";
	std::cout << ( ping.connection -> protocol == network::protocol::IPV6 ? "IPv6" : "IPv4" );
	std::cout << " network.\nICMP packet size: " << ping.packetsize << " bytes +";
	std::cout << "\n\ticmp header size " << ICMP_HEADER_LENGTH << " bytes = ";
	std::cout << ping.packetsize + ICMP_HEADER_LENGTH << " bytes\n";
	std::cout << std::endl;

	ping.execute();

	std::cout << std::endl;
	if ( failed ) std::cout << "Exited because of error" << std::endl;

	return failed ? -1 : 0;
}
