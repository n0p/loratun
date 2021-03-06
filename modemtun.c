#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
#include <stdbool.h>
#include <pthread.h>

#include "lib/debug.h"
#include "lib/modem.h"
#include "schc/schc.h"
#include "schc/schc_fields.h"

#define BUFFER_SIZE 2048 // buffer for reading from tun interface, must be >= 1500

int tun_fd;
int debug_level=0;
bool modem_exit=false;
int modem_ret=0;

pthread_mutex_t recv_mutex;
pthread_mutex_t tun_mutex;

unsigned long int stats_tun_r=0;
unsigned long int stats_tun_w=0;

// aux: search position of a needle in a haystack
int strpos(char *haystack, char *needle) {
	char *p=strstr(haystack, needle);
	if (p) return p - haystack;
	return -1;
}

// output to console
void con(char *msg, ...) {
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

// allocates or reconnects to a tun device. The caller must reserve enough space in *dev.
int tun_alloc(char *dev, int if_flags) {

	struct ifreq ifr;
	int fd, err;
	char *clonedev = "/dev/net/tun";

	if ((fd = open(clonedev , O_RDWR)) < 0) {
		perror("Opening /dev/net/tun");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = if_flags;

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
int cread(int fd, uint8_t *buf, int n) {
	int nread;
	if ((nread=read(fd, buf, n)) < 0) {
		perror("Reading data");
		exit(1);
	}
	return nread;
}

// write routine that checks for errors and exits if an error is returned
int cwrite(int fd, uint8_t *buf, int n) {
	int nwrite;
	if ((nwrite=write(fd, buf, n)) < 0) {
		perror("Writing data");
		exit(1);
	}
	return nwrite;
}

// ensures we read exactly n bytes, and puts them into "buf" unless EOF
int read_n(int fd, uint8_t *buf, int n) {
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

// modem thread wrapper
void *modem_thread(void *p) {
	modem_ret=modemtun_modem(modem_config);
	modem_exit=true;
	return NULL;
}

// data received from modem 
int modemtun_modem_recv(uint8_t *data, int len) {

	uint8_t buffer[BUFFER_SIZE];

	struct field_values p = {0};

	pthread_mutex_lock(&recv_mutex);

	if (debug_level>0) con("[TUN] send %d bytes from modem to the interface\n", len);

	int ret=schc_decompress((uint8_t *)data, len, (struct field_values *)&p);
	//con("[TUN] schc_decompress ret=%d\n", ret);
	//debug((uint8_t *)&p, p.udp_length + offsetof(typeof(p), payload) - 0x08);
	int buffer_len=0;
	if (!ret && !fields2buffer(&p, (uint8_t *)&buffer, (int *)&buffer_len)) {
		if (debug_level>1) {
			con("[TUN] UNSC ");
			debug(buffer, buffer_len);
		}

		stats_tun_w++;

		// now buffer[] contains a full packet or frame, write it into the tun interface
		int nwrite = cwrite(tun_fd, buffer, buffer_len);
		if (debug_level>1) con("[TUN] %lu: written %d bytes to the tun interface\n", stats_tun_w, nwrite);

	}

	pthread_mutex_unlock(&recv_mutex);

	return 0;

}

// main program
int main(int argc, char *argv[]) {

	int if_flags=IFF_TUN;
	char if_name[IFNAMSIZ] = "";
	uint8_t buffer[BUFFER_SIZE];
	int nread;

	// usage: prints usage and exits
	void usage(struct _IO_FILE *to) {
		fprintf(to, "Usage:\n");
		fprintf(to, "%s -i <ifacename> [-d <level>] [-m <modemconfig>...]\n", argv[0]);
		fprintf(to, "%s -h\n", argv[0]);
		fprintf(to, "\n");
		fprintf(to, "-d <level>         outputs debug information while running\n");
		fprintf(to, "-i <ifacename>     name of interface to use (mandatory)\n");
		fprintf(to, "-m <modemconfig>   set a configuration to the modem (key=value)\n");
		fprintf(to, "-h                 prints this help text\n");
		exit(to==stdout?0:1);
	}

	// initialize list of modem configurations
	modem_config_init();

	// check command line options
	int option;
	while ((option = getopt(argc, argv, "i:m:hd:")) > 0) {
		switch (option) {
		case 'h':
			usage(stdout);
			break;
		case 'i':
			strncpy(if_name, optarg, IFNAMSIZ-1);
			break;
		case 'm':
			{
				int p=strpos(optarg, "=");
				if (p>0) {
					optarg[p]=0;
					modem_config_add(optarg, optarg+p+1);
				}
			}
			break;
		case 'd':
			debug_level=atoi(optarg);
			break;
		default:
			err("[TUN] Unknown option %c\n", option);
			usage(stderr);
		}
	}

	// check excess options
	if (argc - optind > 0) {
		err("[TUN] Too many options!\n");
		usage(stderr);
	}

	// check interface name
	if (*if_name == '\0') {
		err("[TUN] Must specify interface name!\n");
		usage(stderr);
	}

	// initialize tun interface
	if ((tun_fd = tun_alloc(if_name, if_flags | IFF_NO_PI)) < 0) {
		err("[TUN] Error connecting to tun interface %s!\n", if_name);
		exit(1);
	}

	if (debug_level>0) con("[TUN] Interface '%s' enabled\n", if_name);

	// modem thread declaration
	pthread_t modem_thread_t;

	// quit handler
	void sig_handler(int signo) {

		// request modem quit
		modemtun_modem_destroy();

		/// wait for the thread to finish
		if (pthread_join(modem_thread_t, NULL)) {
			perror("Error joining thread");
			exit(2);
		}

		// free config list
		modem_config_destroy();

		// everything is OK, lets quit!
		if (debug_level>0) {
			if (modem_ret) err("[TUN] Finished with modem error=%d.\n", modem_ret);
			else con("[TUN] Finished with no errors.\n");
		}
		exit(modem_ret);

	}

	// capture signals
	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	// initialize mutex
	if (pthread_mutex_init(&recv_mutex, NULL) != 0) {
		perror("recv_mutex");
		return 1;
	}
	if (pthread_mutex_init(&tun_mutex, NULL) != 0) {
		perror("send_mutex");
		return 1;
	}

	// start thread
	if (pthread_create(&modem_thread_t, NULL, (void *)modem_thread, 0)) {
		perror("Error creating thread");
		return 1;
	}

	// bucle principal
	while (!modem_exit) {

		int ret;
		fd_set rd_set;

		FD_ZERO(&rd_set);
		FD_SET(tun_fd, &rd_set);

		ret = select(tun_fd + 1, &rd_set, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR) continue;
			perror("select()");
			break;
		}

		// data from tun: just read it and write it to the network
		if (FD_ISSET(tun_fd, &rd_set)) {

			nread = cread(tun_fd, buffer, BUFFER_SIZE);

			stats_tun_r++;
			if (debug_level>0) con("[TUN] %lu: Read %d bytes from the tun interface\n", stats_tun_r, nread);

			if (debug_level>1) {
				con("[TUN] READ ");
				debug(buffer, nread);
			}

			// *** debug: save binary packet to file
			/*
				FILE *fd=fopen("1packet.bin", "w");
				fwrite(buffer, nread, sizeof(uint8_t), fd);
				fclose(fd);
				ret=system("od -Ax -tx1 -v 1packet.bin > 1packet.hex");
			*/

			uint8_t schc_packet[SIZE_MTU_IPV6];
			size_t  schc_packet_len;

			// compress
			{

				struct field_values p = {0};

				// prepare SCHC fields
				if (!buffer2fields((uint8_t *)&buffer, &nread, &p)) {
					// SCHC compress
					int ret=schc_compress(&p, (uint8_t *)&schc_packet, (size_t *)&schc_packet_len);
					//printf("schc_compress ret=%d newlen=%d\n", ret, (int) schc_packet_len);
					if (!ret) {
						if (debug_level>1) {
							con("[TUN] SCHC ");
							debug(schc_packet, schc_packet_len);
						}
						// send compressed packet to modem
						modemtun_modem_send(schc_packet, schc_packet_len);
					}
				}

			}

		}

	}

	// request a quit
	sig_handler(SIGINT);

	// fallback: this will never happen
	return 255;

}
