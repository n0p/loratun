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

//   #include <stddef.h>

#define BUFFER_SIZE 2048 // buffer for reading from tun/tap interface, must be >= 1500

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

// convert a IPv6 packet to the struct field_values used by schc_compress
int buffer2fields(unsigned char *buffer, int *len, struct field_values *p) {

	if (buffer[0] & 0xF0 != 0x60) return 1; // no IPv6 packet
	if (*len > BUFFER_SIZE) return 2; // prevent buffer overrun

	int payload_length=*len - 0x30;

	p->ipv6_version      =6;
	p->ipv6_traffic_class=(uint16_t)buffer[0x01]>>4;
	p->ipv6_flow_label   =((buffer[0x01] & 0x0F)<<16) + (buffer[0x02]<<8) + buffer[0x03];
	p->ipv6_next_header  =(uint8_t)buffer[0x06]; // 17 for UDP
	p->ipv6_hop_limit    =(uint8_t)buffer[0x07];
	p->udp_dev_port      =(uint8_t)buffer[0x28]*256 + (uint8_t)buffer[0x29];
	p->udp_app_port      =(uint8_t)buffer[0x2A]*256 + (uint8_t)buffer[0x2B];
	p->udp_length        =(uint8_t)buffer[0x2C]*256 + (uint8_t)buffer[0x2D];
	p->udp_checksum      =(uint8_t)buffer[0x2E]*256 + (uint8_t)buffer[0x2F];
	memcpy(&(p->ipv6_dev_prefix), &buffer[0x08], 16); // copy IPv6 src address
	memcpy(&(p->ipv6_app_prefix), &buffer[0x18], 16); // copy IPv6 dst address
	memcpy(&(p->payload),         &buffer[0x30], payload_length);

	/*
	printf("*ipv6/udp:p class=%x flow=%x next=%x hop=%x udp(sport=%d dport=%d len=%d checksum=%d)\n",
		p->ipv6_traffic_class, p->ipv6_flow_label,
		p->ipv6_next_header, p->ipv6_hop_limit,
		p->udp_dev_port, p->udp_app_port,
		(unsigned int)p->udp_length, (unsigned int)p->udp_checksum
	);
	debug((unsigned char *)p, offsetof(typeof(*p), payload) + payload_length);
	*/

	return 0;

}

// convert the struct field_values passed by schc_decompress to a IPv6 packet
int fields2buffer(struct field_values *p, char *buffer, int *len) {

	*len=(int)p->udp_length + 0x28;
	if (*len > BUFFER_SIZE) return 1; // prevent buffer overflow
	//memset(buffer, 0, *len); // not needed because all bytes are overwritten
	buffer[0x00]=0x60;
	buffer[0x01]=(p->ipv6_traffic_class*0x10) + ((p->ipv6_flow_label & 0xF0000)>>16);
	buffer[0x02]=(p->ipv6_flow_label & 0xFF00)>>8;
	buffer[0x03]=(p->ipv6_flow_label & 0xFF);
	buffer[0x04]=(p->ipv6_payload_length & 0xFF00)>>8;
	buffer[0x05]=(p->ipv6_payload_length & 0xFF);
	buffer[0x06]=p->ipv6_next_header;
	buffer[0x07]=p->ipv6_hop_limit;
	memcpy(&buffer[0x08], &(p->ipv6_dev_prefix), 32); // copy IPv6 src+dst address
	buffer[0x28]=(p->udp_dev_port & 0xFF00)>>8;
	buffer[0x29]=(p->udp_dev_port & 0xFF);
	buffer[0x2A]=(p->udp_app_port & 0xFF00)>>8;
	buffer[0x2B]=(p->udp_app_port & 0xFF);
	buffer[0x2C]=(p->udp_length & 0xFF00)>>8;
	buffer[0x2D]=(p->udp_length & 0xFF);
	buffer[0x2E]=(p->udp_checksum & 0xFF00)>>8;
	buffer[0x2F]=(p->udp_checksum & 0xFF);
	if ((0x30 + p->ipv6_payload_length - SIZE_UDP) > BUFFER_SIZE) return 2; // prevent buffer overflow
	memcpy(&buffer[0x30], &(p->payload), p->ipv6_payload_length - SIZE_UDP);

	return 0;

}

// main program
int main(int argc, char *argv[]) {

	//int debug_level=0;
	int tap_fd, option;
	int flags = IFF_TUN;
	char if_name[IFNAMSIZ] = "";
	int maxfd;
	int nread;
	unsigned char buffer[BUFFER_SIZE];
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

			nread = cread(tap_fd, buffer, BUFFER_SIZE);

			stats_tap++;
			con("LORATUN %lu: Read %d bytes from the tap interface\n", stats_tap, nread);

			printf("READ "); debug(buffer, nread);

			// *** debug: save binary packet to file
			FILE *fd=fopen("1packet.bin", "w");
			fwrite(buffer, nread, sizeof(unsigned char), fd);
			fclose(fd);
			ret=system("od -Ax -tx1 -v 1packet.bin > 1packet.hex");

			uint8_t schc_packet[SIZE_MTU_IPV6];
			size_t  schc_packet_len;

			// compress
			{

				struct field_values p = {0};

				if (!buffer2fields((unsigned char *)&buffer, &nread, &p)) {
					int ret=schc_compress(&p, (uint8_t *)&schc_packet, (size_t *)&schc_packet_len);
					//printf("schc_compress ret=%d newlen=%d\n", ret, (int) schc_packet_len);
					if (!ret) {
						printf("SCHC "); debug(schc_packet, schc_packet_len);
					}
				}

			}

			// decompress
			{

				struct field_values p = {0};

				int len=0;
				int ret=schc_decompress((uint8_t *)&schc_packet, schc_packet_len, (struct field_values *)&p);
				//printf("schc_decompress ret=%d\n", ret);
				//debug((unsigned char *)&p, p.udp_length + offsetof(typeof(p), payload) - 0x08);
				if (!ret && !fields2buffer(&p, (unsigned char *)&buffer, (int *)&len)) {
					printf("UNSC "); debug(buffer, len);
				}

			}

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
