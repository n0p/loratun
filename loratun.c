#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

#include "lib/debug.h"
#include "schc/schc.h"

// buffer for reading from tun/tap interface, must be >= 1500
#define BUFSIZE 2048
#define CLIENT 0
#define SERVER 1
#define PORT 10001

#define BUF2NUNIT16(buf, n) (unsigned char)buf[n]*256+(unsigned char)buf[n+1];

// output to console
void con(char *msg, ...){
	va_list argp;
	va_start(argp, msg);
	vfprintf(stdout, msg, argp);
	va_end(argp);
}

// prints custom error messages on stderr
void err(char *msg, ...) {
	va_list argp;
	va_start(argp, msg);
	vfprintf(stderr, msg, argp);
	va_end(argp);
}

// Allocates or reconnects to a tun/tap device. The caller must reserve enough space in *dev.
int tun_alloc(char *dev, int flags) {

	struct ifreq ifr;
	int fd, err;
	char *clonedev = "/dev/net/tun";

	if ((fd = open(clonedev , O_RDWR)) < 0) {
		perror("Opening /dev/net/tun");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = flags;

	if (*dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		perror("ioctl(TUNSETIFF)");
		close(fd);
		return err;
	}

	strcpy(dev, ifr.ifr_name);

	return fd;
}

// read routine that checks for errors and exits if an error is returned
int cread(int fd, unsigned char *buf, int n) {
	int nread;
	if ((nread=read(fd, buf, n)) < 0) {
		perror("Reading data");
		exit(1);
	}
	return nread;
}

// write routine that checks for errors and exits if an error is returned
int cwrite(int fd, char *buf, int n) {
	int nwrite;
	if ((nwrite=write(fd, buf, n)) < 0) {
		perror("Writing data");
		exit(1);
	}
	return nwrite;
}

// ensures we read exactly n bytes, and puts them into "buf" unless EOF
int read_n(int fd, unsigned char *buf, int n) {
	int nread;
	int left = n;
	while (left > 0) {
		if ((nread = cread(fd, buf, left)) == 0) {
			return 0;
		} else {
			left -= nread;
			buf += nread;
		}
	}
	return n;
}

// main program
int main(int argc, char *argv[]) {

	//int debug_level=0;
	int tap_fd, option;
	int flags = IFF_TUN;
	char if_name[IFNAMSIZ] = "";
	int maxfd;
	uint16_t nread;
	unsigned char buffer[BUFSIZE];
	//struct sockaddr_in local, remote;
	//char remote_ip[16] = "";            // dotted quad IP string
	int sock_fd=0, net_fd=0;
	unsigned long int stats_tap=0;

	// usage: prints usage and exits
	void usage() {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s -i <ifacename> [-u|-a] [-d]\n", argv[0]);
		fprintf(stderr, "%s -h\n", argv[0]);
		fprintf(stderr, "\n");
		fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
		fprintf(stderr, "-u|-a: use TUN (-u, default) or TAP (-a)\n");
		fprintf(stderr, "-d: outputs debug information while running\n");
		fprintf(stderr, "-h: prints this help text\n");
		exit(1);
	}

	// check command line options
	while ((option = getopt(argc, argv, "i:uahd")) > 0) {
		switch(option) {
			case 'd':
				//debug_level=1;
				break;
			case 'h':
				usage();
				break;
			case 'i':
				strncpy(if_name,optarg, IFNAMSIZ-1);
				break;
			case 'u':
				flags=IFF_TUN;
				break;
			case 'a':
				flags=IFF_TAP;
				break;
			default:
				err("Unknown option %c\n", option);
				usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc > 0) {
		err("Too many options!\n");
		usage();
	}

	if (*if_name == '\0') {
		err("Must specify interface name!\n");
		usage();
	}

	// initialize tun/tap interface
	if ((tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0) {
		err("Error connecting to tun/tap interface %s!\n", if_name);
		exit(1);
	}

	con("Interface '%s' enabled\n", if_name);

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		exit(1);
	}


/*


	socklen_t remotelen;
	int cliserv=-1;    // must be specified on cmd line
	int optval=1;


	if (cliserv == CLIENT) {
		// Client, try to connect to server

		// assign the destination address
		memset(&remote, 0, sizeof(remote));
		remote.sin_family = AF_INET;
		remote.sin_addr.s_addr = inet_addr(remote_ip);
		remote.sin_port = htons(port);

		// connection request
		if (connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
			perror("connect()");
			exit(1);
		}

		net_fd = sock_fd;
		con("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
		
	} else {

		// Server, wait for connections

		// avoid EADDRINUSE error on bind()
		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
			perror("setsockopt()");
			exit(1);
		}

		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		local.sin_port = htons(port);
		if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
			perror("bind()");
			exit(1);
		}

		if (listen(sock_fd, 5) < 0) {
			perror("listen()");
			exit(1);
		}

		// wait for connection request
		remotelen = sizeof(remote);
		memset(&remote, 0, remotelen);
		if ((net_fd = accept(sock_fd, (struct sockaddr*)&remote, &remotelen)) < 0) {
			perror("accept()");
			exit(1);
		}

		con("SERVER: Client connected from %s\n", inet_ntoa(remote.sin_addr));

	}
*/

	// use select() to handle two descriptors at once
	maxfd = (tap_fd > net_fd?tap_fd:net_fd);

	while (1) {

		int ret;
		fd_set rd_set;

		FD_ZERO(&rd_set);
		FD_SET(tap_fd, &rd_set); FD_SET(net_fd, &rd_set);

		ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

		if (ret < 0 && errno == EINTR) {
			continue;
		}

		if (ret < 0) {
			perror("select()");
			exit(1);
		}

		// data from tun/tap: just read it and write it to the network
		if (FD_ISSET(tap_fd, &rd_set)) {

			nread = cread(tap_fd, buffer, BUFSIZE);

			stats_tap++;
			con("LORATUN %lu: Read %d bytes from the tap interface\n", stats_tap, nread);

			debug(buffer, nread);

			// *** debug: save binary packet to file
			FILE *fd=fopen("1packet.bin", "w");
			fwrite(buffer, nread, sizeof(unsigned char), fd);
			fclose(fd);
			ret=system("od -Ax -tx1 -v 1packet.bin > 1packet.hex");


			/*
			struct field_values {
				uint8_t ipv6_version;
				uint8_t ipv6_traffic_class;
				uint32_t ipv6_flow_label;
				size_t ipv6_payload_length;
				uint8_t ipv6_next_header;
				uint8_t ipv6_hop_limit;
				uint8_t ipv6_dev_prefix[8];
				uint8_t ipv6_dev_iid[8];
				uint8_t ipv6_app_prefix[8];
				uint8_t ipv6_app_iid[8];
				uint16_t udp_dev_port;
				uint16_t udp_app_port;
				size_t udp_length;
				uint16_t udp_checksum;
				uint8_t payload[SIZE_MTU_IPV6]; 
			};
			*/
			struct field_values ipv6packet = {0};


			ipv6packet.ipv6_version=6;
			ipv6packet.ipv6_next_header=17;
			ipv6packet.ipv6_hop_limit=0xFF;
			ipv6packet.udp_dev_port=BUF2NUNIT16(buffer, 40);
			ipv6packet.udp_app_port=BUF2NUNIT16(buffer, 42); // 43690;
			ipv6packet.udp_length=SIZE_UDP+(nread-48);

			memcpy(&(ipv6packet.ipv6_dev_prefix), &buffer[8], 16); // copiar dirección IPv6 origen
			memcpy(&(ipv6packet.ipv6_app_prefix), &buffer[24], 16); // copiar dirección IPv6 destino
			memcpy(&(ipv6packet.payload), &buffer[48], nread-48); // copiar dirección IPv6 destino
			memcpy(&(ipv6packet.udp_checksum), &buffer[46], 1); // copiar checksum

			ipv6packet.udp_length=BUF2NUNIT16(buffer, 44);
			ipv6packet.udp_checksum=BUF2NUNIT16(buffer, 46);

			printf("********** %d %d\n", ipv6packet.udp_dev_port, ipv6packet.udp_app_port);
			printf("********** %x %x\n", ipv6packet.udp_length, ipv6packet.udp_checksum);
			debug((unsigned char *)&ipv6packet, sizeof(ipv6packet));

			uint8_t schc_packet[SIZE_MTU_IPV6];
			size_t  schc_packet_len;
			int ret=schc_compress(&ipv6packet, (uint8_t *)&schc_packet, (size_t *)&schc_packet_len);

			printf("*******ret=%d len=%d\n", ret, (int) schc_packet_len);
			debug(schc_packet, schc_packet_len);


/*

			uint16_t nwrite, plength;

			// write length + packet
			plength = htons(nread);
			nwrite = cwrite(net_fd, (char *)&plength, sizeof(plength));
			nwrite = cwrite(net_fd, buffer, nread);
			
			con("TAP_IN %lu: Written %d bytes to the network\n", stats_tap, nwrite);
*/

		}


/*
		//unsigned long int net2tap=0;

		// data from the network: read it, and write it to the tun/tap interface. 
		// We need to read the length first, and then the packet
		if (FD_ISSET(net_fd, &rd_set)) {


			// Read length
			nread = read_n(net_fd, (char *)&plength, sizeof(plength));
			if (nread == 0) {
				// ctrl-c at the other end
				break;
			}

			net2tap++;

			// read packet
			nread = read_n(net_fd, buffer, ntohs(plength));
			con("NET2TAP %lu: Read %d bytes from the network\n", net2tap, nread);

			// now buffer[] contains a full packet or frame, write it into the tun/tap interface
			nwrite = cwrite(tap_fd, buffer, nread);
			con("NET2TAP %lu: Written %d bytes to the tap interface\n", net2tap, nwrite);

		}
*/

	}

	return 0;

}
