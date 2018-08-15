#include "isend.h"

//#define ADDRESS "fd73:ab44:93dd:6b18::1" // fd73:ab44:93dd:6b18::/64

int sock=0;
uint8_t *packet_buffer=NULL;
uint16_t packet_buffer_len=0;
struct sockaddr_in6 server_addr;

// initialize internet send
int isend_init(uint8_t *dst_addr, uint16_t dst_port) {

	// prepare socket for IPv6/UDP protocol
	sock=socket(PF_INET6, SOCK_DGRAM, 0);
	if (sock < 0) {
		sock=0;
		return 1;
	}

	// prepare destination address and port in network byte order
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family=AF_INET6;
	inet_pton(AF_INET6, dst_addr, &server_addr.sin6_addr);
	server_addr.sin6_port=htons(dst_port);

	return 0;

}

// clear buffer
void isend_clear() {
	if (packet_buffer) {
		free(packet_buffer);
		packet_buffer=NULL;
		packet_buffer_len=0;
	}
}

// clean packet
void isend_begin() {
	isend_clear();
	packet_buffer=malloc(0);
	packet_buffer_len=0;
}

// add data to the packet
void isend_add(uint8_t *data, uint16_t len) {
	packet_buffer=realloc(packet_buffer, packet_buffer_len+len);
	memcpy(packet_buffer+packet_buffer_len, data, len);
	packet_buffer_len+=len;
}

// send the packet
void isend_commit() {
	if (sendto(sock, packet_buffer, packet_buffer_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("[client] sendto failed");
		exit(2);
	}
	isend_clear();
}

// finish
void isend_destroy() {
	// close socket
	if (sock) {
		close(sock);
		sock=0;
	}
}
