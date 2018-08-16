#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "../lib/config.h"
#include "../lib/serial.h"
#include "../lib/debug.h"

#include "isend.h"

#define MAX_LINE 1024

int debug_level=0;
int fd=0;

enum {
	STEP_NONE,
	STEP_RESET,
	STEP_INIT,
	STEP_PID_FIRST,
	STEP_PID_REQ,
	STEP_PID_RPM,
	STEP_PID_SPEED
};

int step=STEP_NONE;

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

// aux: search position of a needle in a haystack
int strpos(char *haystack, char *needle) {
	char *p=strstr(haystack, needle);
	if (p) return p - haystack;
	return -1;
}

// send command
int cmd_send(char *command) {
	if (debug_level>2) con("> %s\n", command);
	return serial_write(fd, command, strlen(command));
}

// send command and change step
int cmd_send_step(char *command, int next_step) {
	step=next_step;
	return cmd_send(command);
}

// coompare strings and get natural boolean
bool equals(char *a, char *b) {
	return (strcmp(a, b)?false:true);
}

// read a complete line from serial port
int line_read(char *line) {
	static int line_buf_pos=0;
	static int line_buf_len=0;
	static char line_buf[MAX_LINE];
	int i, n=0;
	while (1) {
		// first, read the buffer for pending lines
		for (i=line_buf_pos; i<line_buf_len; i++) if (line_buf[i] == '\r') {
			memcpy(line, line_buf, i);
			line[i]=0; // replace \r with \0
			int nrest=line_buf_len-i-1;
			memcpy(line_buf, line_buf+i+1, nrest);
			line_buf_len-=i+1;
			//con("NBUFFER rest=%d pos=%d len=%d ", nrest, line_buf_pos, line_buf_len); debug((char *)&line_buf, line_buf_len);
			line_buf_pos=0;
			return i;
		}
		line_buf_pos+=n;
		if (line_buf_pos >= MAX_LINE) return -2; // buffer limit reached
		n=serial_read(fd, (char *)&line_buf + line_buf_len, MAX_LINE - line_buf_len);
		line_buf_len+=n;
		//con("BUFFER read=%d pos=%d len=%d ", n, line_buf_pos, line_buf_len); debug((char *)&line_buf, line_buf_len);
		if (n<0) break;
	}
	return n;
}

// main application
int main(int argc, char **argv) {

	char *serport=NULL;
	char next_pid_req_b[MAX_LINE];
	char *next_pid_req=(char *)&next_pid_req_b;
	char dst_addr_b[40]="";
	char *dst_addr=(char *)dst_addr_b;
	uint16_t dst_port=43690;
	unsigned char bin[MAX_LINE / 3];
	char buffer[MAX_LINE];
	char *line=(char *)&buffer;
	bool working=true;
	uint8_t  engine_load=0, speed=0;
	uint16_t rpm=0;
	uint32_t speed_t=0, rpm_t=0, engine_load_t=0;
	uint32_t reads=0;

	// main destroy
	int destroy() {
		if (debug_level>0) con("Cleaning up\n");
		if (fd) {
			serial_close(fd);
			fd=0;
		}
		if (dst_addr) isend_destroy();
		return 0;
	}

	// quit handler
	void sig_handler(int signo) {
		con("\n");
		destroy();
		exit(0);
	}

	// setup config
	config_init();
	//config_add("at", "AT@1");
	//config_add("at", "ATSP3");
	config_add("at", "ATSP0"); // by default, automatic protocol search

	// usage: prints usage and exits
	void usage(struct _IO_FILE *to) {
		fprintf(to, "Usage:\n");
		fprintf(to, "%s <serialport> [-a <atcmd>] [-d <level>]\n", argv[0]);
		fprintf(to, "%s -h\n", argv[0]);
		fprintf(to, "\n");
		fprintf(to, "-a <atcmd>      send an AT command at startup\n");
		fprintf(to, "-s <ipv6addr>   send data over IPv6/UDP\n");
		fprintf(to, "-p <ipv6port>   specify port (actual: %d)\n", dst_port);
		fprintf(to, "-d <level>      outputs debug information while running\n");
		fprintf(to, "-h              prints this help text\n");
		exit(to==stdout?0:1);
	}

	// check command line options
	int option;
	while ((option = getopt(argc, argv, "ha:d:s:p:")) > 0) {
		switch (option) {
		case 'h':
			usage(stdout);
			break;
		case 'a':
			config_add("at", optarg);
			break;
		case 'd':
			debug_level=1;
			break;
		case 's':
			strcpy(dst_addr, optarg);
			break;
		case 'p':
			dst_port=atoi(optarg);
			break;
		default:
			break;
		}
	}

	// require serial port
	if (argc - optind < 1) {
		err("Required serial port not specified.\n");
		usage(stderr);
	}

	// excess options
	if (argc - optind > 1) {
		err("Too many options!\n");
		usage(stderr);
	}

	// get serial port
	if (argv[argc-1]) {
		config_add("serialport", argv[argc-1]);
		serport=config_get("serialport");
	} else {
		err("No serial port specified!\n");
		usage(stderr);
	}

	//uint8_t testpkt [] = {0x07, 0x40, 0xD4, 0x6C, 0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00};

	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	// aux: hex byte to unsigned char
	unsigned char hex2int(char c) {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		return 0;
	}

	// aux: line to binary
	int line2bin() {
		int i, nbin;
		memset(&bin, 0, MAX_LINE / 3);
		for (i=0, nbin=0; i<strlen(line); i+=3, nbin++)
			bin[nbin]=(hex2int(line[i])*16) + hex2int(line[i+1]);
		return nbin;
	}

	// retry
	int retry_first_pid() {
		static int pid_req_retries=0;
		if (pid_req_retries<3) {
			pid_req_retries++;
			cmd_send_step("0100\r", STEP_PID_FIRST);
		} else {
			working=false;
		}
		return working;
	}

	// send string over internet
	void send_string(char *s) {
		isend_add((uint8_t *)s, strlen(s));
	}

	// send op over internet
	void send_op(uint8_t op) {
		isend_add((uint8_t *)&op, 1);
	}

	// send op + 1 byte over internet
	void send_op1(uint8_t op, uint8_t data) {
		struct Packet {
			uint8_t op;
			uint8_t data;
		} __attribute__((packed)) p;
		uint8_t *b=(uint8_t *)&p;
		p.op=op;
		p.data=data;
		isend_add(b, 2);
	}

	// send op + 2 bytes over internet
	void send_op2(uint8_t op, uint16_t data) {
		struct Packet {
			uint8_t op;
			uint16_t data;
		} __attribute__((packed)) p;
		uint8_t *b=(uint8_t *)&p;
		p.op=op;
		p.data=htons(data);
		isend_add(b, 3);
	}

	// if destination declared, initialize internet socket
	if (dst_addr)
		if (isend_init(dst_addr, dst_port) != 0) {
			perror("[client] creating socket");
			exit(11);
		}

	// start connection
	con("Connecting to %s\n", serport);
 	fd=serial_open(serport);
	if (fd) {
		
		if (debug_level>0) con("Serial Initialization\n");

		// set speed, no parity, no blocking
		serial_set_interface_attribs(fd, B9600, 0);

		// set blocking disabled
		serial_set_blocking(fd, 0);

		// send reset, wait for it and discard every byte received in the next 1000ms
		cmd_send_step("ATZ\r", STEP_RESET);
		{
			int i;
			for (i=0; i<10; i++) {
				serial_aread(fd, line, MAX_LINE);
				if (debug_level>2) con("%s", line);
				usleep(100000);
			}
		}

		// clean, wait for it and discard every byte received in the next 1000ms or until OK is received
		cmd_send("ATD\r");
		{
			int i;
			for (i=0; i<10; i++) {
				serial_aread(fd, line, MAX_LINE);
				if (debug_level>2) con("[%s]", line);
				if (strpos(line, "OK") >= 0) {
					if (debug_level>0) con("ATD is OK\n");
					break;
				}
				usleep(100000);
			}
		}

		// set blocking enabled
		serial_set_blocking(fd, 1);

		//while(true) { line_read(line); con("LINE[%s]\n", line); } // debug

		// disable echo
		if (debug_level>0) con("Disable echo\n");
		cmd_send_step("ATE0\r", STEP_INIT);

		// point to first config
		Node *config_node=config->first;

		// main working loop
		while (working) {

			// read next line
			int nread=line_read(line);
			if (nread <= 0 || equals(line, "")) continue; // ignore empty lines
			if (line[0] == '>') line++; // clear console prompt
			if (debug_level>1) con("< %s\n", line);

			// state machine
			switch (step) {

			// initialize
			case STEP_INIT:
				if (equals(line, "E0") || equals(line, "OK") || equals(line, "BUS INIT: OK")) {
					if (config_node) {
						do {
							Config *c=(Config *)config_node->e;
							config_node=config_node->sig;
							if (equals("at", c->key)) {
								if (debug_level>0) con("Startup config %s\n", c->value);
								cmd_send(c->value);
								cmd_send("\r");
								break;
							}
						} while (config_node);
					} else {
						usleep(100000); // 100ms wait to prevent ELM327 CPU blocking
						if (debug_level>0) con("Sending first pid\n");
						retry_first_pid();
					}
				} else {
					// errors here
				}
				break;	

			// first PID request
			case STEP_PID_FIRST:
				// if bus initialization is successfull, try first PID
				if (equals(line, "BUS INIT: OK")) {

					if (debug_level>0) con("first pid OK\n");
					next_pid_req="0104\r";
					cmd_send_step(next_pid_req, STEP_PID_REQ);

				// if initialization is not successfull, try again a few times
				} else if (equals(line, "BUS INIT: ERROR")) {

					if (debug_level>0) con("%s\n", line);
					retry_first_pid();

				// if bus initialization is searching, go next step
				} else if (equals(line, "SEARCHING...")) {

					if (debug_level>0) con("searching protocol\n");
					step=STEP_PID_REQ;

				}
				break;

			// PID request state
			case STEP_PID_REQ:

				// unable to connect, retry first pid
				if (equals(line, "UNABLE TO CONNECT")) {

					retry_first_pid();

				// else, we have a response
				} else {

					// convert line to binary
					int nbin=line2bin(); //debug((unsigned char *)&bin, nread/3);

					// check service received
					if (nbin>1 && bin[0]==0x41) {

						// default first PID request
						next_pid_req="0104\r";

						// select by service PID
						switch (bin[1]) {

						case 0x04: // Engine Load
							next_pid_req="010C\r";
							engine_load=(float)bin[2]/2.55;
							break;

						case 0x0C: // RPM
							next_pid_req="010D\r";
							rpm=(bin[2]*256+bin[3])/4;
							break;

						case 0x0D: // Speed
							//next_pid_req="0104\r";
							speed=bin[2];
							
							// add num reads
							reads++;

							// total and average speed, rpm and engine load
							speed_t      +=speed;
							rpm_t        +=rpm;
							engine_load_t+=engine_load;
							int
								mspeed      =speed_t      /reads,
								mrpm        =rpm_t        /reads,
								mengine_load=engine_load_t/reads
							;

							// show data
							con(
								"DATA(%d) speed=%d avg=%d rpm=%d avg=%d load=%d%% avg=%d%%\n",
								reads, speed, mspeed, rpm, mrpm, engine_load, mengine_load
							);

							// send data over internet
							if (dst_addr) {
								isend_begin();
								send_string("CAR");
								//send_op(0x01);
								send_op1(0x04, mengine_load);
								send_op2(0x0C, mrpm);
								send_op1(0x0D, mspeed);
								isend_commit();
							}

							// gracefully sleep
							usleep(150000);

							break;

						default:
							con("Unsupported service PID %x\n", bin[1]);

						}

						// ask next
						if (next_pid_req[0]) cmd_send(next_pid_req);

					} else {

						// maybe NO DATA, retry
						if (next_pid_req[0]) cmd_send(next_pid_req);

					}

				}
				break;

			}

		}

	}

	// finish
	return destroy();

}
