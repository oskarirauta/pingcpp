#include <thread>
#include <fcntl.h>
#include <netinet/icmp6.h>

#include "common.hpp"
#include "ping.hpp"

const bool network::ping_t::unpack6(char *buf, size_t sz, int hoplimit) {

	struct icmp6_hdr *ip_packet = (struct icmp6_hdr *)buf;

	if ( ip_packet -> icmp6_id == this -> id &&
		ip_packet -> icmp6_type == ICMP6_ECHO_REPLY ) {

		this -> result -> bytes = sz;
		this -> result -> seq = ip_packet -> icmp6_seq;
		this -> result -> ttl = hoplimit;
		this -> result -> end = common::get_millis();
		this -> result -> error_code = network::ping_error::PING_NOERROR;
		this -> result -> icmp_type = (int)ip_packet -> icmp6_type;

		return true;
	}

	this -> result -> icmp_type = (int)ip_packet -> icmp6_type;
	this -> result -> error_code = network::ping_error::PING_MISMATCH;
	return false;
}

const bool network::ping_t::send6(void) {

	size_t send_result;
	size_t packetsize = ICMP6_HEADER_LENGTH + this -> _packetsize();
	int bufsize = packetsize + 128;
	char data[bufsize];
	if ( packetsize > ICMP6_HEADER_LENGTH )
		memset(data + ICMP6_HEADER_LENGTH, 0, bufsize - ICMP6_HEADER_LENGTH);

	struct icmp6_hdr *icmp_request = (struct icmp6_hdr *)data;

	icmp_request -> icmp6_type = ICMP6_ECHO_REQUEST;
	icmp_request -> icmp6_code = 0;
	icmp_request -> icmp6_cksum = 0;
	icmp_request -> icmp6_id = this -> id;
	icmp_request -> icmp6_seq = this -> seq;

	this -> result -> start = common::get_millis();
	this -> setsockopts6();

	send_result = sendto(this -> sockfd, data, packetsize, 0,
				this -> ai -> ai_addr, this -> ai -> ai_addrlen);

	if ( send_result < 0 ) {
		this -> result -> error_code = network::ping_error::PING_SENDFAIL;
		return false;
	} else if ( send_result != packetsize ) {
		this -> result -> error_code = network::ping_error::PING_SENDSIZE;
		return false;
	}

	this -> result -> bytes_sent = send_result;
	this -> summary -> sent++;
	return true;
}

const bool network::ping_t::receive6(void) {

	int packetsize = ICMP6_HEADER_LENGTH + this -> _packetsize();
	int bufsize = packetsize + 128;
	char receive_buffer[bufsize];
	char control_buffer[bufsize];
	struct sockaddr_in6 receive_address;

	struct iovec iov {
		.iov_base = receive_buffer,
		.iov_len = (size_t)bufsize,
	};
	iov.iov_base = receive_buffer;
	iov.iov_len = sizeof(receive_buffer);

	struct msghdr msg {
		.msg_name = &receive_address,
		.msg_namelen = sizeof(receive_address),
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = control_buffer,
		.msg_controllen = (socklen_t)bufsize,
	};

	while ( true ) {

		ssize_t recv_result = recvmsg(this -> sockfd, &msg, 0);

		if ( recv_result > 0 ) {

			struct cmsghdr *mp;
			int hoplimit = -1;

			for ( mp = CMSG_FIRSTHDR(&msg); mp; mp = CMSG_NXTHDR(&msg, mp))
				if ( mp -> cmsg_level == SOL_IPV6 &&
					mp -> cmsg_type == IPV6_HOPLIMIT )
					hoplimit = *(int *)CMSG_DATA(mp);

			if ( this -> unpack6((char *)&receive_buffer, recv_result, hoplimit)) {
				this -> summary -> received++;
				this -> result -> error_code = network::ping_error::PING_NOERROR;
				break;
			}

		} else if ( recv_result == 0 ) {
			this -> result -> error_code = network::ping_error::PING_CONNCLOSED;
			break;
		} else if ( recv_result < 0 ) {

			if ( errno == EAGAIN ) {

				std::chrono::milliseconds delay = common::get_millis() - this -> result -> start;
				std::chrono::milliseconds delay_max = std::chrono::duration_cast<std::chrono::milliseconds>(this -> timeout);

				if ( delay >= delay_max ) {
					this -> result -> error_code = network::ping_error::PING_TIMEOUT;
					break;
				}
			} else if ( errno != EINTR ) this -> result -> recvfrom_err++;
		}
	}

	return this -> result -> error_code == network::ping_error::PING_NOERROR ?
		true : false;

}

void network::ping_t::setsockopts6(void) {

	int sockopt;

	// Enable ping broadcasts
        sockopt = 1;
        setsockopt(this -> sockfd, SOL_SOCKET, SO_BROADCAST, &sockopt, sizeof(int));

	// Set recv buffer for ping broadcasts
	sockopt = ( this -> _packetsize() * 2 ) + ICMP6_HEADER_LENGTH * 1024;
	setsockopt(this -> sockfd, SOL_SOCKET, SO_RCVBUF, &sockopt, sizeof(int));

	// Use checksums
	sockopt = 2;
	setsockopt(this -> sockfd, SOL_RAW, IPV6_CHECKSUM, &sockopt, sizeof(int));

	// Enable hoplimit
	sockopt = 1;
	setsockopt(this -> sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &sockopt, sizeof(int));
	setsockopt(this -> sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &sockopt, sizeof(int));
	setsockopt(this -> sockfd, SOL_IPV6, IPV6_HOPLIMIT, &sockopt, sizeof(int));

	// Request TTL
	sockopt = 1;
	setsockopt(this -> sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &sockopt, sizeof(int));
	setsockopt(this -> sockfd, IPPROTO_IP, IP_TTL, &sockopt, sizeof(int));

}


const bool network::ping_t::create_socket6(void) {

	if (( this -> sockfd = socket(this -> ai -> ai_family,
					this -> ai -> ai_socktype,
					this -> ai -> ai_protocol)) < 0 ) {

		if ( errno == EPERM )
			this -> result -> error_code = network::ping_error::PING_PERMISSION;
		else this -> result -> error_code = network::ping_error::PING_CONNFAIL;

		return false;
	}

	if ( fcntl(this -> sockfd, F_SETFL, O_NONBLOCK) == -1 ) {
		this -> result -> error_code = network::ping_error::PING_EBADF;
		return false;
	}

	this -> setsockopts6();
	return true;
}

const bool::network::ping_t::ping6(void) {

	this -> result -> reset();
	this -> summary -> reset();

	if ( !this -> create_socket6())
		return false;

	this -> seq = 0;
	int min = this -> count_min < 0 ? 1 : this -> count_min;
	int max = this -> count_max <= 0 ? 999 : (
		this -> count_max + 1 < this -> count_min ? this -> count_min + 1 : this -> count_max);

	if ( min == 0 ) {
		min = max;
		max++;
	}

	for ( this -> seq = 0; !this -> abort && this -> seq < max && this -> summary -> succeeded < min; this -> seq++ ) {

		this -> result -> reset();
		this -> summary -> seq++;

		if ( this -> seq != 0 && this -> delay.count() > 0 )
			std::this_thread::sleep_for(this -> delay);

		if ( this -> send6() && this -> receive6())
			this -> summary -> succeeded++;
		else this -> summary -> failed++;

		if ( this -> should_abort()) {
			this -> summary -> aborted = true;
			this -> abort = true;
		}

		if ( this -> report != NULL )
			this -> report(this);
	}

	return this -> abort || this -> summary -> succeeded < min ? false : true;
}
