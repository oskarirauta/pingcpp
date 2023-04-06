#pragma once

#include <string>
#include <chrono>
#include <netdb.h>

#include "ping_utils.hpp"

#ifndef ICMP_HEADER_LENGTH
# define ICMP_HEADER_LENGTH sizeof(struct icmp *)
#endif

#ifdef __PINGCPP_IPV6__
# ifndef ICMP6_HEADER_LENGTH
#  define ICMP6_HEADER_LENGTH ICMP_HEADER_LENGTH
# endif
# ifdef IPV6_2292HOPLIMIT
#  undef IPV6_HOPLIMIT
#  define IPV6_HOPLIMIT IPV6_2292HOPLIMIT
# endif
#endif

namespace network {

	typedef enum {
		PING_NOERROR = 0,
		PING_MISMATCH, /* icmp type or id not matching */
		PING_SENDFAIL,
		PING_SENDSIZE, /* received less than sent */
		PING_CONNCLOSED, /* disconnected */
		PING_TIMEOUT, /* no reply received before time out */
		PING_PERMISSION, /* socket failure, no root priviledge */
		PING_CONNFAIL, /* socket failed to connect */
		PING_EBADF, /* socket: bad file descriptor */
		PING_NOREPLY, /* no reply for other reason */
		PING_UNKNOWNPROTOCOL, /* protocol of address not supported, ipv6??? */
	} ping_error;

	struct ping_result {

		public:
			std::string addr = "";
			int bytes_sent = 0;
			int bytes = 0;
			int seq = 0;
			int ttl = 0;
			int recvfrom_err = 0;
			bool cksum_error = false;
			uint16_t cksum = 0, icmp_cksum = 0;
			int icmp_type = -1;
			std::chrono::milliseconds start, end;
			network::ping_error error_code = network::ping_error::PING_NOERROR;

			inline const bool succeeded() {
				return this -> error_code == 0 ? true : false;
			}

			inline const std::chrono::milliseconds time() {
				return this -> end - this -> start;
			}

			inline void reset() {
				this -> bytes_sent = 0;
				this -> bytes = 0;
				this -> seq = 0;
				this -> ttl = 0;
				this -> recvfrom_err = 0;
				this -> cksum_error = false;
				this -> cksum = 0;
				this -> icmp_cksum = 0;
				this -> icmp_type = -1;
				this -> error_code = network::ping_error::PING_NOERROR;
			}
	};

	struct ping_connection {

		public:
			std::string target = "";
			std::string ipaddr = "";
			std::string domain = "";
			network::protocol protocol = network::protocol::ANY;
	};

	struct ping_summary {

		public:
			int seq = 0;
			int sent = 0;
			int received = 0;
			int failed = 0;
			int succeeded = 0;
			bool aborted = false;

			inline void reset(void) {
				this -> seq = 0;
				this -> sent = 0;
				this -> received = 0;
				this -> failed = 0;
				this -> succeeded = 0;
				this -> aborted = false;
			}
	};

	class ping_t {

		private:
			std::string host;
			std::string host_addr;
			int sockfd;
			int id;
			int seq;
			struct addrinfo *ai;
			network::protocol protocol = network::protocol::IPV4;

			const size_t _packetsize(void);

			const bool should_abort(void);
			const uint16_t cksum4(uint16_t *buf, size_t sz);
			const bool unpack4(char *buf, size_t sz);
			void setsockopts4(void);
			const bool create_socket4(void);
			const bool send4(void);
			const bool receive4(void);
			const bool ping4(void);

#ifdef __PINGCPP_IPV6__
			const bool unpack6(char *buf, size_t sz, int hoplimit);
			void setsockopts6(void);
			const bool create_socket6(void);
			const bool send6(void);
			const bool receive6(void);
			const bool ping6(void);
#endif

		public:
			std::chrono::milliseconds timeout = std::chrono::milliseconds(5000);
			std::chrono::milliseconds delay = std::chrono::milliseconds(500);
			size_t packetsize = 56;
			int count_min = 1;
			int count_max = 3;
			bool abort = false;

			struct network::ping_connection* connection;
			struct network::ping_result* result;
			struct network::ping_summary* summary;

			void (*report)(network::ping_t *) = NULL;

			ping_t(std::string addr, network::protocol protocol = network::protocol::IPV4);
			~ping_t(void);
			const bool address_error(void);
			const bool execute(void);
	};

}
