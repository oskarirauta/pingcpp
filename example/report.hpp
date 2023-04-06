#include <iostream>
#include <string>
#include "ping_utils.hpp"
#include "ping.hpp"

extern bool failed;

inline void report_error(network::ping_t *ping) {

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
	else if ( ping -> result -> error_code == network::ping_error::PING_PERMISSION )
		std::cout << "Aborting. You need to be root to ping." << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_CONNFAIL )
		std::cout << "Aborting. Socket failed to connect to host " << ping -> result -> addr << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_EBADF )
		std::cout << "Aborting. Socket: bad file descriptor" << std::endl;
	else if ( ping -> result -> error_code == network::ping_error::PING_UNKNOWNPROTOCOL )
		std::cout << "Aborting. Protocol of " << ping -> result -> addr << " is not supported" << std::endl;
}

inline void report(network::ping_t *ping) {

	std::cout << "seq " << ping -> summary -> seq << ": ";
	report_error(ping);

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
