#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../lib/config.h"
#include "../lib/serial.h"
#include "../lib/debug.h"

#define MAX_LINE 1024

#define true  1
#define false 0

int fd=0;
int line_buf_pos=0;
int line_buf_len=0;
char line_buf[MAX_LINE];

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

// send
int send(char *command) {
	//con("send(%d) %s\n", (int)strlen(command), command);
	return serial_write(fd, command, strlen(command));
}

// send and change step
int send_step(char *command, int next_step) {
	step=next_step;
	return send(command);
}

// coompare strings and get natural boolean
int equals(char *a, char *b) {
	return (strcmp(a, b)?0:1);
}

// read a complete line from serial port
int line_read(char *line) {
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

	int i;
	int pid_req_retries=0;
	int debug_level=0;
	char *serport=NULL;
	char buffer[MAX_LINE];
	char next_pid_req_buffer[MAX_LINE];
	char *next_pid_req=(char *)&next_pid_req_buffer;
	unsigned char bin[MAX_LINE / 3];
	char *line=(char *)&buffer;
	int reads=0;
	unsigned char engine_load=0;
	int speed  =0, rpm  =0;
	int speed_t=0, rpm_t=0;
	int working=true;

	// destroy
	int destroy() {
		if (debug_level>0) con("Cleaning up\n");
		if (fd) serial_close(fd);
		fd=0;
		return 0;
	}

	// quit handler
	void sig_handler(int signo) {
		con("\n");
		destroy();
		exit(0);
	}

	if (argc<1) {
		con("Required serial port.");
		exit(1);
	}

	// setup config
	config_init();
	//config_add("at", "AT@1");
	//config_add("at", "ATSP3");
	config_add("at", "ATSP0");

	// usage: prints usage and exits
	void usage(struct _IO_FILE *to) {
		fprintf(to, "Usage:\n");
		fprintf(to, "%s <serialport> [-a <atcmd>] [-d <level>]\n", argv[0]);
		fprintf(to, "%s -h\n", argv[0]);
		fprintf(to, "\n");
		fprintf(to, "-a <atcmd>   send an AT command at startup\n");
		fprintf(to, "-d <level>   outputs debug information while running\n");
		fprintf(to, "-h           prints this help text\n");
		exit(to==stdout?0:1);
	}

	// check command line options
	int option;
	while ((option = getopt(argc, argv, "a:d:h")) > 0) {
		switch (option) {
		case 'h':
			usage(stdout);
			break;
		case 'd':
			debug_level=1;
			break;
		case 'a':
			config_add("at", optarg);
			break;
		default:
			break;
		}
	}

	if (argc - optind < 1) {
		err("Required serial port not specified.\n");
		usage(stderr);
	}

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
		if (pid_req_retries<3) {
			pid_req_retries++;
			send_step("0100\r", STEP_PID_FIRST);
		} else {
			working=false;
		}
		return working;
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

		send_step("ATZ\r", STEP_RESET);
		
		// wait for modem reset and discard every byte received in the next 1000ms
		for (i=0; i<10; i++) {
			serial_aread(fd, line, MAX_LINE);
			if (debug_level>2) con("%s", line);
			usleep(100000);
		}

		send("ATD\r");

		// wait for modem reset and discard every byte received in the next 1000ms
		for (i=0; i<10; i++) {
			serial_aread(fd, line, MAX_LINE);
			if (debug_level>2) con("[%s]", line);
			if (strpos(line, "OK") >= 0) {
				if (debug_level>0) con("ATD is OK\n");
				break;
			}
			usleep(100000);
		}

		// set blocking enabled
		serial_set_blocking(fd, 1);

//while(true) { line_read(line); con("LINEA [%s]\n", line); }

		if (debug_level>0) con("Disable echo\n");

		// disable echo
		send_step("ATE0\r", STEP_INIT);

		// initialize first config
		Node *n=config->first;

		// main working loop
		while (working) {

			// read lines
			int nread=line_read(line);

			// ignore empty lines, clear console trailing, prepare bin buffer
			if (nread<=0 || equals(line, "")) continue;
			if (line[0]=='>') line++; // ignore first character if it's a prompt
			int nbin=line2bin();
			//debug((unsigned char *)&bin, nread/3);
			//con("ELM327 "); debug(line, nread);
			if (debug_level>0) con("ELM327 %s\n", line);

			// state machine
			switch (step) {

			// initialize
			case STEP_INIT:
				if (equals(line, "E0") || equals(line, "OK") || equals(line, "BUS INIT: OK")) {
					if (n) {
						do {
							Config *c=(Config *)n->e;
							if (equals("at", c->key)) {
								if (debug_level>0) con("Startup config %s\n", c->value);
								send(c->value);
								send("\r");
								n=n->sig;
								break;
							}
							n=n->sig;
						} while (n);
					} else {
						usleep(100000); // para que no se agobie ;-)
						if (debug_level>0) con("Sending first pid\n");
						retry_first_pid();
						send_step("0100\r", STEP_PID_FIRST);
					}
				} else {
					// errors here
				}
				break;	

			// first PID request
			case STEP_PID_FIRST:
				if (equals(line, "BUS INIT: OK")) {
					if (debug_level>0) con("first pid OK\n");
					next_pid_req="0104\r";
					send_step(next_pid_req, STEP_PID_REQ);
				} else if (equals(line, "BUS INIT: ERROR")) {
					// retry
					if (debug_level>0) con("%s\n", line);
					retry_first_pid();
				} else if (equals(line, "SEARCHING...")) {
					if (debug_level>0) con("searching protocol\n");
					step=STEP_PID_REQ;
				}
				break;

			// PID request state
			case STEP_PID_REQ:
				if (equals(line, "UNABLE TO CONNECT")) {
					retry_first_pid();
				} else {
					// service received
					if (nbin>1 && bin[0]==0x41) {
						next_pid_req="";
						// select by service PID
						switch (bin[1]) {
						case 0x04:
							next_pid_req="010C\r";
							engine_load=(float)bin[2]/2.55;
							break;
						case 0x0C:
							next_pid_req="010D\r";
							rpm=(bin[2]*256+bin[3])/4;
							break;
						case 0x0D:
							next_pid_req="0104\r";
							speed=bin[2];
							reads++;
							speed_t+=speed;
							rpm_t  +=rpm;
							int mspeed=speed_t/reads, mrpm=rpm_t/reads;
							con(
								"DATA(%d) speed=%d\trpm=%d\tmspeed=%d\tmrpm=%d\tload=%d%%\n",
								reads, speed, rpm, mspeed, mrpm, engine_load
							);
							usleep(150000);
							break;
						default:
							con("Unsupported service PID %x\n", bin[1]);
						}
						if (next_pid_req[0]) send(next_pid_req);
					} else {
						// maybe NO DATA, retry
						if (next_pid_req[0]) send(next_pid_req);
					}
				}
				break;

			}

		}

	}

	// finish
	return destroy();

}
