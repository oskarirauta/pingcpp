#include <netdb.h>
#include "ping_utils.hpp"

const int network::protocolOf(const std::string ipaddr, network::protocol &family) {

	int res = 0;
	struct addrinfo *ai, hints = {
		.ai_flags = AI_NUMERICHOST,
		.ai_family = PF_UNSPEC,
	};

	if (( res = getaddrinfo(ipaddr.c_str(), NULL, &hints, &ai)) != 0 )
		return res;

#ifdef __PINGCPP_IPV6__
	family = ai -> ai_family == AF_INET ? network::protocol::IPV4 :
		( ai -> ai_family == AF_INET6 ? network::protocol::IPV6 : family);
#else
	if ( ai -> ai_family != AF_INET )
		res = -1; // IPv6 support not enabled
	else family = network::protocol::IPV4;
#endif

	freeaddrinfo(ai);
	return res;
}

const int network::get_dns_records(const std::string address, std::vector<std::string> &results, network::protocol family) {

	int res;
	struct addrinfo *ai, hints = {
#ifdef __PINGCPP_IPV6__
		.ai_family = family == network::protocol::IPV4 ? AF_INET :
				( family == network::protocol::IPV6 ? AF_INET6 : AF_UNSPEC),
#else
		.ai_family = family == network::protocol::IPV4 ? AF_INET : AF_UNSPEC,
#endif
		.ai_socktype = SOCK_STREAM,
	};

	if (( res = getaddrinfo(address.c_str(), NULL, &hints, &ai)) != 0 )
		return res;

	struct addrinfo *p;
	char ip_address[INET6_ADDRSTRLEN];
	int oldsize = results.size();

	for ( p = ai; p != NULL; p = p -> ai_next )
		if (( res = getnameinfo((struct sockaddr *)p -> ai_addr, p -> ai_addrlen, ip_address, sizeof ip_address,
				NULL, 0, NI_NUMERICHOST)) == 0 )
			results.push_back(std::string(ip_address));

	freeaddrinfo(ai);
	return res == 0 ? 0 : ( results.size() == oldsize ? res : 0 );
}

const int network::dns_lookup(const std::string address, std::string &ipaddr, network::protocol family) {

	std::vector<std::string> records;
	int res;

	if (( res = network::get_dns_records(address, records, family)) == 0 &&
		!records.empty()) ipaddr = records[0];

	return res == 0 && records.empty() ? -1 : res;
}

const int network::reverse_dns_lookup(const std::string ipaddr, std::string &address) {

	int res;
	struct addrinfo *ai, hints = {
		.ai_flags = AI_NUMERICHOST,
		.ai_family = AF_UNSPEC,
	};

	if (( res = getaddrinfo(ipaddr.c_str(), NULL, &hints, &ai)) > 0 )
		return res;

	char node[NI_MAXHOST];

	if (( res = getnameinfo(ai -> ai_addr, ai -> ai_addrlen, node, sizeof node,
				NULL,  0, NI_NAMEREQD)) == 0 ) {
		address = std::string(node);
	}

	freeaddrinfo(ai);
	return res;
}
