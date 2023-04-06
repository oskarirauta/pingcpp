#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "ping.hpp"

network::ping_t::ping_t(std::string address, network::protocol protocol) {

	this -> host = address;
	this -> host_addr = "";
	this -> sockfd = -1;
	this -> id = getpid() & 0xFFFF;
	this -> protocol = protocol;
	this -> ai = NULL;
	this -> abort = false;

	this -> connection = new network::ping_connection;
	this -> connection -> target = address;

	int res;

	setuid(getuid());

	if (( res = network::dns_lookup(address, this -> host_addr, this -> protocol)) != 0 ) {
		this -> host_addr = address;
		this -> connection -> ipaddr = address;
		this -> connection -> domain = "";
		std::string domain;
		if ( network::reverse_dns_lookup(address, domain) == 0 )
			this -> connection -> domain = domain;
	} else {
		this -> connection -> domain = address;
		this -> connection -> ipaddr = this -> host_addr;
	}

	if (( res = network::protocolOf(this -> host_addr, this -> protocol)) != 0 ||
		this -> protocol == network::protocol::ANY )
		this -> protocol = network::protocol::ANY;

	this -> connection -> protocol = this -> protocol;

	struct addrinfo hints = {
		.ai_flags = AI_NUMERICHOST,
#ifdef __PINGCPP_IPV6__
		.ai_family = this -> protocol == network::protocol::IPV4 ? AF_INET : AF_INET6,
#else
		.ai_family = AF_INET,
#endif
		.ai_socktype = SOCK_RAW,
#ifdef __PINGCPP_IPV6__
		.ai_protocol = this -> protocol == network::protocol::IPV4 ? IPPROTO_ICMP : IPPROTO_ICMPV6,
#else
		.ai_protocol = IPPROTO_ICMP,
#endif
	};

	if (( res = getaddrinfo(this -> host_addr.c_str(), NULL, &hints, &this -> ai)) > 0 )
		this -> ai = NULL;

	this -> result = new network::ping_result;
	this -> result -> addr = this -> host_addr;

	this -> summary = new network::ping_summary;
	this -> summary -> reset();
}

network::ping_t::~ping_t(void) {

	if ( this -> ai != NULL )
		freeaddrinfo(this -> ai);

	free( this -> connection);
	free( this -> result);
	free( this -> summary);
}

const bool network::ping_t::should_abort(void) {

	return ( this -> result -> error_code == network::ping_error::PING_PERMISSION ||
		this -> result -> error_code == network::ping_error::PING_CONNFAIL ||
		this -> result -> error_code == network::ping_error::PING_EBADF ||
		this -> result -> error_code == network::ping_error::PING_UNKNOWNPROTOCOL ) ? true : false;
}

const size_t network::ping_t::_packetsize(void) {

	return this -> packetsize < 0 ? 0 :
		( this -> packetsize > ( 65515 - ICMP_HEADER_LENGTH ) ?
			( 65515 - ICMP_HEADER_LENGTH ) :
			this -> packetsize );
}

const bool network::ping_t::address_error(void) {

	return this -> ai == NULL ? true : false;
}

const bool network::ping_t::execute(void) {

	if ( this -> protocol == network::protocol::ANY ) {

		this -> summary -> reset();
		this -> result -> reset();
		this -> result -> error_code = network::ping_error::PING_UNKNOWNPROTOCOL;
		return false;
	}

#ifdef __PINGCPP_IPV6__
	return this -> protocol == network::protocol::IPV6 ?
			this -> ping6() : this -> ping4();
#else
	return this -> ping4();
#endif

}
