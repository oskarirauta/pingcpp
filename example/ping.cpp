#include <iostream>
#include <string>
#include "network.hpp"
#include "ping.hpp"

bool failed = false;

void report(network::ping_t *ping) {

	if ( ping -> result -> error_code == network::ping_error::PING_MISMATCH )
		std::cout << "icmp type or id mismatch from " << ping -> result -> addr << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_SENDFAIL )
		std::cout << "failed to send icmp ping to " << ping -> result -> addr << std::endl;		
	else if ( ping -> result -> error_code == network::ping_error::PING_SENDSIZE )
		std::cout << "received " << ping -> result -> bytes << " bytes, expected " << ping -> result -> bytes_sent << " bytes" << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_CONNCLOSED )
		std::cout << ping -> result -> addr << " closed connection un-expectedly" << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_TIMEOUT )
		std::cout << "Timed out with " << ping -> result -> time().count() << "ms" << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_NOREPLY )
		std::cout << "No reply from " << ping -> result -> addr << " for unknown reason" << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_PERMISSION ) {
		std::cout << "You need to be root to ping." << std::endl;
		ping -> abort = true;
		failed = true;
	} else if ( ping -> result -> error_code == network::ping_error::PING_CONNFAIL ) {
		std::cout << "Socket failed to connect to host " << ping -> result -> addr << std::endl;
		ping -> abort = true;
		failed = true;
	} else if ( ping -> result -> error_code == network::ping_error::PING_EBADF ) {
		std::cout << "Socket: bad file descriptor" << std::endl;
		ping -> abort = true;
		failed = true;
	} else if ( ping -> result -> error_code == network::ping_error::PING_UNKNOWNPROTOCOL ) {
		std::cout << "Protocol of " << ping -> result -> addr << " is not supported" << std::endl;
		ping -> abort = true;
		failed = true;
	}

	if ( ping -> result -> error_code != network::ping_error::PING_NOERROR )
		return;

	std::cout << ping -> result -> bytes << " bytes from ";
	std::cout << ping -> result -> addr;
	std::cout << " seq=" << ping -> result -> seq;
	std::cout << " ttl=" << ping -> result -> ttl;
	std::cout << " time=" << ping -> result -> time().count() << "ms";
	if ( ping -> result -> cksum_error ) {
		std::cout << " (cksum " <<
			ping -> result -> cksum << " != " <<
			ping -> result -> icmp_cksum << ")";
	}
	std::cout << std::endl;
}

void usage(std::string cmd, std::string msg) {

	std::cout << "ping, by Oskari Rauta\n" << std::endl;

	if ( cmd.empty() && !msg.empty()) {
		std::cout << "Error:\n\t" << msg << "\n" << std::endl;
		return;
	}

	std::cout << "Options:\n";

	std::cout << "\t-? or -h\tThis help\n";
	std::cout << "\t-p X\t\tpacket size in bytes, default: 56\n";
	std::cout << "\t-d X\t\tdelay between pings in milliseconds, default: 500\n";
	std::cout << "\t-t X\t\ttimeout in seconds, default: 15\n";
	std::cout << "\t-1  \t\texit after first succesfull ping\n";
	std::cout << "\t-4  \t\tIPv4\n";
	std::cout << "\t-6  \t\tTry IPv6 and fallback to IPv4\n";
	std::cout << "\t-m X\t\tMaximum of ping attempts, default: 999\n\n";

	std::cout << "Usage:\n" << "\t" << cmd << " [options] host\n" << std::endl;

	if ( !msg.empty())
		std::cout << "Error:\n\t" << msg << "\n" << std::endl;
}

int main(int argc, char *argv[]) {

	std::string cmd = std::string(argv[0]);
	bool paramErr = argc == 1 ? true : false;
	std::string paramMsg = "";
	char *arg;
	int packetsize = 56;
	int delay = 500;
	int timeout = 15;
	int max = 999;
	bool any = true;
	bool ipv6 = false;
	bool exitOnFirstPing = false;

	argc--;
	argv++;

	while ( argc >= 1 && **argv == '-' && !paramErr ) {

		arg = *argv;
		arg++;

		switch ( *arg ) {
			case '?':
				paramMsg = "";
				paramErr = true;
				break;
			case 'h':
				paramMsg = "";
				paramErr = true;
				break;
			case 'p':
				if ( --argc <= 0 ) {
					paramMsg = "packetsize value missing";
					paramErr = true;
					break;
				}
				argv++;
				packetsize = atoi(*argv);
				break;
			case 'd':
				if ( --argc <= 0 ) {
					paramMsg = "delay(ms) value missing";
					paramErr = true;
					break;
				}
				argv++;
				delay = atoi(*argv);
				break;
			case 't':
				if ( --argc <= 0 ) {
					paramMsg = "tieout (seconds) value missing";
					paramErr = true;
					break;
				}
				argv++;
				timeout = atoi(*argv);
				break;
			case '1':
				exitOnFirstPing = true;
				break;
			case '4':
				any = false;
				ipv6 = false;
				break;
			case '6':
				any = false;
				ipv6 = true;
				break;
			case 'm':
				if ( --argc <= 0 ) {
					paramMsg = "Max ping count value missing";
					paramErr = true;
					break;
				}
				argv++;
				max = atoi(*argv);
				break;
			default:
				paramMsg = "Unknown parameter";
				paramErr = true;
		}
		argc--;
		argv++;

	}

	std::string host = paramErr ? "" : std::string(*argv);

	if ( !paramErr && host.empty()) {
		paramErr = true;
		paramMsg = "host missing";
	}


	if ( paramErr ) {
		usage(cmd, paramMsg);
		return 0;
	}

	network::ping_t ping(host, any ? network::protocol::ANY :
		( ipv6 ? network::protocol::IPV6 : network::protocol::IPV4));

	// Check for errors

	if ( ping.connection -> protocol == network::protocol::ANY ) {
		usage("", "Address not detected as either IPv4 or IPv6 address, or hostname does not resolve.");
		return -1;
	}

	if ( ping.address_error()) {
		usage("", "Address info for host not available.");
		return -1;
	}

	// Setup
	ping.packetsize = packetsize;
	ping.delay = std::chrono::milliseconds(delay);
	ping.timeout = std::chrono::seconds(timeout);
	ping.count_min = exitOnFirstPing ? 1 : max;
	ping.count_max = max;
	ping.report = report;

	ping.execute();

	if ( failed ) {
		std::cout << "Exited because of error" << std::endl;
		return -1;
	}

	return 0;
}
