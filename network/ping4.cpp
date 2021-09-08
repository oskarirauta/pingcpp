#include <thread>
#include <fcntl.h>
#include <netinet/ip_icmp.h>

#include "common.hpp"
#include "ping.hpp"

const uint16_t network::ping_t::cksum4(uint16_t *buf, size_t sz) {

	uint16_t *p = buf;
	int nleft = sz;
	int sum = 0;

	while ( nleft > 1 ) {
		sum += *p;
		p++;
		nleft -= 2;
	}

	if ( nleft == 1 ) {
		uint16_t ans = 0;
		*(uint16_t *)(&ans) = *(unsigned char *)p;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ~sum;
}

const bool network::ping_t::unpack4(char *buf, int sz) {

	int packetsize = ICMP_HEADER_LENGTH + this -> _packetsize();
	if ( sz < packetsize )
		packetsize = sz;

	struct ip *ip_packet = (struct ip *)buf;

	if ( ip_packet -> ip_p == IPPROTO_ICMP ) {

		int ip_header_length = ip_packet -> ip_hl << 2;
		int icmp_packet_length = sz - ip_header_length;

		if ( icmp_packet_length >= ICMP_HEADER_LENGTH ) {

			struct icmp *icmp_packet =
				(struct icmp *)(buf + ip_header_length);

			if ( icmp_packet -> icmp_type == ICMP_ECHOREPLY &&
				icmp_packet -> icmp_id == this -> id ) {

				uint16_t icmp_cksum = (uint16_t)icmp_packet -> icmp_cksum;
				icmp_packet -> icmp_cksum = 0;
				uint16_t cksum = this ->cksum4((unsigned short *)icmp_packet, packetsize);

				this -> result -> bytes = icmp_packet_length;
				this -> result -> seq = icmp_packet -> icmp_seq;
				this -> result -> ttl = (int)ip_packet -> ip_ttl;
				this -> result -> end = common::get_millis();
				this -> result -> cksum_error = cksum == icmp_cksum ? false : true;
				this -> result -> error_code = network::ping_error::PING_NOERROR;
				this -> result -> cksum = cksum;
				this -> result -> icmp_cksum = icmp_cksum;
				this -> result -> icmp_type = (int)icmp_packet -> icmp_type;

				return true;
			}

			this -> result -> icmp_type = (int)icmp_packet -> icmp_type;
		}
	}

	this -> result -> error_code = network::ping_error::PING_MISMATCH;
	return false;
}

const bool network::ping_t::send4(void) {

	int send_result;
	int packetsize = ICMP_HEADER_LENGTH + this -> _packetsize();
	int bufsize = packetsize + 128;
	char data[bufsize];
	if ( packetsize > ICMP_HEADER_LENGTH )
		memset(data + ICMP_HEADER_LENGTH, 0, bufsize - ICMP_HEADER_LENGTH);

	//int ip_header_length = ip_packet -> ip_hl << 2;
	struct icmp *icmp_request = (struct icmp *)data;

	icmp_request -> icmp_type = ICMP_ECHO;
	icmp_request -> icmp_code = 0;
	icmp_request -> icmp_cksum = 0;
	icmp_request -> icmp_id = this -> id;
	icmp_request -> icmp_seq = this -> seq;
	icmp_request -> icmp_data[0] = 0;
	icmp_request -> icmp_data[1] = 0;
	icmp_request -> icmp_cksum = this -> cksum4((unsigned short *)icmp_request,
					packetsize);

	this -> setsockopts4();
	this -> result -> start = common::get_millis();

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

const bool network::ping_t::receive4(void) {

	int packetsize = ICMP_HEADER_LENGTH + this -> _packetsize();
	int bufsize = packetsize + 128;
	char receive_buffer[bufsize];
	char control_buffer[bufsize];
	struct sockaddr receive_address;

	struct iovec iov {
		.iov_base = receive_buffer,
		.iov_len = (size_t)bufsize,
	};

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

		if ( recv_result > 0 && this -> unpack4((char *)&receive_buffer, recv_result)) {
			this -> summary -> received++;
			this -> result -> error_code = network::ping_error::PING_NOERROR;
			break;
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

void network::ping_t::setsockopts4(void) {

	int sockopt;

	// Enable ping broadcasts
	sockopt = 1;
	setsockopt(this -> sockfd, SOL_SOCKET, SO_BROADCAST, &sockopt, sizeof(int));

	// Set recv buffer for ping broadcasts
	sockopt = ( 65 * 1024 ) + 8;
	setsockopt(this -> sockfd, SOL_SOCKET, SO_RCVBUF, &sockopt, sizeof(int));
}

const bool network::ping_t::create_socket4(void) {

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

	this -> setsockopts4();
	return true;
}

const bool::network::ping_t::ping4(void) {

	this -> result -> reset();
	this -> summary -> reset();

	if ( !this -> create_socket4()) return false;

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

		if ( this -> send4() && this -> receive4())
			this -> summary -> succeeded++;
		else {
			this -> summary -> failed++;
			this -> result -> end = common::get_millis();
		}

		if ( this -> should_abort()) {
			this -> summary -> aborted = true;
			this -> abort = true;
		}

		if ( this -> report != NULL )
			this -> report(this);

		if ( this -> should_abort())
			this -> abort = true;
	}

	return this -> abort || this -> summary -> succeeded < min ? false : true;
}
