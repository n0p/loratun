#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER 1024
#define MESSAGE "HOL@!"
//#define ADDRESS "::1"
#define ADDRESS "fd73:ab44:93dd:6b18::1" // fd73:ab44:93dd:6b18::/64
#define PORT 43690 // 0xAAAA;
//#define ADDRESS "fe80:0000:0000:0000:0800:27ff:fe00:0001"
//#define PORT 10001

int main(void)
{

	int sock;
	struct sockaddr_in6 server_addr;

	/* IPv6/UDP protocol */
	sock = socket(PF_INET6, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("[client] creating socket");
		exit(1);
	}

	/* prepare destination address and port in network byte order */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	inet_pton(AF_INET6, ADDRESS, &server_addr.sin6_addr);
	server_addr.sin6_port = htons(PORT);

	/* now send a datagram */
	if (sendto(sock, MESSAGE, sizeof(MESSAGE), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("[client] sendto failed");
		exit(2);
	}

	printf("[client] '%s' sent to '%s'\n", MESSAGE, ADDRESS);

/*

	socklen_t client_len;
	char addrbuf[INET6_ADDRSTRLEN];
	char buffer[BUFFER];
	struct sockaddr_in6 client_addr;

	// wait for a reply
	printf("[client] '%s' sent, waiting for a reply\n", MESSAGE);
	client_len = sizeof(client_addr);
	if (recvfrom(sock, buffer, BUFFER, 0, (struct sockaddr *)&client_addr, &client_len) < 0) {
		perror("[client] recvfrom failed");
		exit(3);
	}

	// show received message
	printf("[client] received '%s' from %s\n",
		buffer,
		inet_ntop(AF_INET6, &client_addr.sin6_addr, addrbuf, INET6_ADDRSTRLEN)
	);
*/

	/* close socket */
	close(sock);

	/* all fine! */
	return 0;

}
