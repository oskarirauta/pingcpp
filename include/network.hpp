#pragma once

#include <string>
#include <vector>
#include <map>
#include <netinet/ip_icmp.h>

namespace network {

	typedef enum {
		IPV4 = 0,
#ifdef __IPV6__
		IPV6,
#endif
		ANY
	} protocol;

	const std::map<int, std::string> icmp_type_names = {
		{ ICMP_ECHOREPLY, "Echo Reply" },
		{ ICMP_DEST_UNREACH, "Destination Unreachable" },
		{ ICMP_SOURCE_QUENCH, "Source Quench" },
		{ ICMP_REDIRECT, "Redirect (change route)" },
		{ ICMP_ECHO, "Echo Request" },
		{ ICMP_TIME_EXCEEDED, "Time Exceeded" },
		{ ICMP_PARAMETERPROB, "Parameter Problem" },
		{ ICMP_TIMESTAMP, "Timestamp Request" },
		{ ICMP_TIMESTAMPREPLY, "Timestamp Reply" },
		{ ICMP_INFO_REQUEST, "Information Request" },
		{ ICMP_INFO_REPLY, "Information Reply" },
		{ ICMP_ADDRESS, "Address Mask Request" },
		{ ICMP_ADDRESSREPLY, "Address Mask Reply" },
	};

	const int protocolOf(const std::string ipaddr, network::protocol &family);
	const int get_dns_records(const std::string address, std::vector<std::string> &ipaddrs, const network::protocol family = network::protocol::ANY);
	const int dns_lookup(const std::string address, std::string &ipaddr, const network::protocol family = network::protocol::ANY);
	const int reverse_dns_lookup(const std::string ipaddr, std::string &address);
	inline const std::string icmp_type_name(int icmp_type) {

		if ( network::icmp_type_names.find(icmp_type) == network::icmp_type_names.end())
			return("Unknown ICMP Type");
		return network::icmp_type_names.at(icmp_type);
	}
}
