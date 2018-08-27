#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MESSAGE "HOL@!"
#define ADDRESS "fd73:ab44:93dd:6b18::1"
#define PORT 43690

int main(void) {

	int sock;
	struct sockaddr_in6 server_addr;

	// IPv6/UDP protocol
	sock = socket(PF_INET6, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("[client] creating socket");
		exit(1);
	}

	// prepare destination address and port in network byte order
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	inet_pton(AF_INET6, ADDRESS, &server_addr.sin6_addr);
	server_addr.sin6_port = htons(PORT);

	// now send the datagram
	if (sendto(sock, MESSAGE, sizeof(MESSAGE), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("[client] sendto failed");
		exit(2);
	}
	printf("[client] '%s' sent to '%s'\n", MESSAGE, ADDRESS);

	// close socket
	close(sock);

	// all fine!
	return 0;

}
