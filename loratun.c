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
#include "lib/modem.h"
#include "schc/schc.h"

//   #include <stddef.h>

#define BUFFER_SIZE 2048 // buffer for reading from tun/tap interface, must be >= 1500

// output to console
void con(char *msg, ...){
	va_list argp;
	va_start(argp, msg);
	vfprintf(stdout, msg, argp);
	va_end(argp);
	fflush(stdout);
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
	memcpy(&(p->ipv6_dev_prefix), &buffer[0x08], 32); // copy IPv6 src+dst address
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


List *modem_config;

void modemConfigInit() {
	modem_config=listNew();
}

void modemConfigAdd(char *k, char *v) {
	ModemConfig *c=malloc(sizeof(ModemConfig));
	strcpy(c->key, k);
	strcpy(c->value, v);
	listNodeAdd(modem_config, (ModemConfig *)c);
}

void modemConfigDel(ModemConfig *c) {
	free(c);
}

void modemConfigList() {
	Node *n=modem_config->first;
	while (n) {
		ModemConfig *c=(ModemConfig *)n->e;
		printf("%s=%s\n", c->key, c->value);
		//if (strcmp(c->key, "key2")==0) printf("*** parámetro me interesa!\n");
		n=n->sig;
	}
}

int strpos(char *haystack, char *needle) {
	char *p=strstr(haystack, needle);
	if (p) return p - haystack;
	return -1;
}


unsigned long int stats_tap_r=0;
unsigned long int stats_tap_w=0;

int tap_fd;

int loratun_modem_recv(char *data, int len) {

	unsigned char buffer[BUFFER_SIZE];

	// decompress

	struct field_values p = {0};

	int ret=schc_decompress((uint8_t *)data, len, (struct field_values *)&p);
	//printf("schc_decompress ret=%d\n", ret);
	//debug((unsigned char *)&p, p.udp_length + offsetof(typeof(p), payload) - 0x08);
	int buffer_len=0;
	if (!ret && !fields2buffer(&p, (unsigned char *)&buffer, (int *)&buffer_len)) {
		printf("UNSC "); debug(buffer, buffer_len);

		stats_tap_w++;

		// now buffer[] contains a full packet or frame, write it into the tun/tap interface
		int nwrite = cwrite(tap_fd, buffer, buffer_len);
		con("NET2TAP %lu: Written %d bytes to the tap interface\n", stats_tap_w, nwrite);

	}

	return 0;
}

// main program
int main(int argc, char *argv[]) {

	//int debug_level=0;
	int option;
	int flags = IFF_TUN;
	char if_name[IFNAMSIZ] = "";
	int nread;
	unsigned char buffer[BUFFER_SIZE];
	//struct sockaddr_in local, remote;
	//char remote_ip[16] = "";            // dotted quad IP string
	//int sock_fd=0;

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

	// inicializar lista de configuraciones de modem
	modemConfigInit();

	// check command line options
	while ((option = getopt(argc, argv, "i:m:uahd")) > 0) {
		switch (option) {
			case 'd':
				//debug_level=1;
				break;
			case 'h':
				usage();
				break;
			case 'i':
				strncpy(if_name, optarg, IFNAMSIZ-1);
				break;
			case 'u':
				flags=IFF_TUN;
				break;
			case 'a':
				flags=IFF_TAP;
				break;
			case 'm':
				{
					int p=strpos(optarg, "=");
					if (p>0) {
						int vlen=strlen(optarg) - p;
						optarg[p]=0;
						modemConfigAdd(optarg, optarg+p+1);
					}
				}
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

/*
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		exit(1);
	}
*/

	// bucle principal
	while (1) {

		int ret;
		fd_set rd_set;

		FD_ZERO(&rd_set);
		FD_SET(tap_fd, &rd_set);

		ret = select(tap_fd + 1, &rd_set, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR) continue;
			perror("select()");
			exit(1);
		}

		// data from tun/tap: just read it and write it to the network
		if (FD_ISSET(tap_fd, &rd_set)) {

			nread = cread(tap_fd, buffer, BUFFER_SIZE);

			stats_tap_r++;
			con("LORATUN %lu: Read %d bytes from the tap interface\n", stats_tap_r, nread);

			printf("READ "); debug(buffer, nread);

			// *** debug: save binary packet to file
			/*
				FILE *fd=fopen("1packet.bin", "w");
				fwrite(buffer, nread, sizeof(unsigned char), fd);
				fclose(fd);
				ret=system("od -Ax -tx1 -v 1packet.bin > 1packet.hex");
			*/

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
						// enviar paquete al modem
						loratun_modem_send(schc_packet, schc_packet_len);
					}
				}

			}

			/*

			pthread_mutex_lock(&mutex);

			*/

		}

	}

	return 0;

}
