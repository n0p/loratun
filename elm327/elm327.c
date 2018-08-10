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
	int i, n;
	while (1) {
		n=serial_read(fd, (char *)&line_buf + line_buf_pos, MAX_LINE - line_buf_pos);
		//con("LINE_READ "); debug((char *)&line_buf + line_buf_pos, n);
		if (n<0) return n;
		for (i=0; i<n; i++)
			if (line_buf[line_buf_pos+i] == '\r') {
				int aux=line_buf_pos+i;
				memcpy(line, line_buf, aux);
				line[aux]=0;
				line_buf_pos=0;
				return aux;
			}
		line_buf_pos+=n;
		if (line_buf_pos >= MAX_LINE) return 0;
	}
	return n;
}

// destroy
int destroy() {
	con("cleaning up\n");
	if (fd) serial_close(fd);
	fd=0;
	return 0;
}

// main application
int main() {

	// quit handler
	void sig_handler(int signo) {
		con("\n");
		destroy();
		exit(0);
	}

	// setup config
	config_init();
	config_add("serialport", "/dev/rfcomm2");
	//config_add("at", "AT@1");
	//config_add("at", "ATSP3");
	config_add("at", "ATSP0");

	//uint8_t testpkt [] = {0x07, 0x40, 0xD4, 0x6C, 0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00};

	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	int i;
	int pid_req_retries=0;
	char *serport=config_get("serialport");
	char buffer[MAX_LINE];
	unsigned char bin[MAX_LINE / 3];
	char *line=(char *)&buffer;
	int reads=0;
	unsigned char engine_load=0;
	int speed  =0, rpm  =0;
	int speed_t=0, rpm_t=0;
	int working=true;

	// aux: hex byte to unsigned char
	unsigned char hex2int(char c) {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		return 0;
	}

	// aux: line to binary
	void line2bin() {
		int i, j;
		memset(&bin, 0, MAX_LINE / 3);
		for (i=0, j=0;i<strlen(line); i+=3, j++)
			bin[j]=(hex2int(line[i])*16) + hex2int(line[i+1]);
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

 	fd=serial_open(serport);
	if (fd) {
		
		con("init\n");

		// set speed, no parity, no blocking
		//serial_set_interface_attribs(fd, B9600, 0);

		// set blocking disabled
		serial_set_blocking(fd, 0);

		send_step("ATZ\r", STEP_RESET);
		
		// wait for modem reset and discard every byte received in the next 1000ms
		for (i=0; i<10; i++) {
			serial_aread(fd, line, MAX_LINE);
			//con("%s", line);
			usleep(100000);
		}

		send("ATD\r");

		// wait for modem reset and discard every byte received in the next 1000ms
		for (i=0; i<10; i++) {
			serial_aread(fd, line, MAX_LINE);
			//con("%s", line);
			if (strpos(line, "OK") >= 0) {
				con("ATD is OK\n");
				break;
			}
			usleep(100000);
		}

		// set blocking enabled
		serial_set_blocking(fd, 1);

		con("disable echo\n");

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
			if (line[0]=='>') line++;
			line2bin();
			//debug((unsigned char *)&bin, nread/3);
			//con("ELM327 "); debug(line, nread);
			//con("ELM327 %s\n", line);

			// state machine
			switch (step) {

			// we are initializing
			case STEP_INIT:
				if (equals(line, "E0") || equals(line, "OK") || equals(line, "BUS INIT: OK")) {
					if (n) {
						do {
							Config *c=(Config *)n->e;
							if (equals("at", c->key)) {
								con("config %s\n", c->value);
								send(c->value);
								send("\r");
								n=n->sig;
								break;
							}
							n=n->sig;
						} while (n);
					} else {
						usleep(100000); // para que no se agobie ;-)
						con("Sending first pid\n");
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
					con("first pid OK\n");
					send_step("0104", STEP_PID_REQ);
				} else if (equals(line, "BUS INIT: ERROR")) {
					// retry
					con("%s\n", line);
					retry_first_pid();
				} else if (equals(line, "SEARCHING...")) {
					con("searching protocol\n");
					step=STEP_PID_REQ;
				}
				break;

			// PID request
			case STEP_PID_REQ:
				if (equals(line, "UNABLE TO CONNECT")) {
					retry_first_pid();
				} else {
					if (bin[0]==0x41) {
						engine_load=(float)bin[2]/2.55;
						send_step("010C\r", STEP_PID_RPM);
					}
				}
				break;

			// RPM request
			case STEP_PID_RPM:
				if (bin[0]==0x41) {
					rpm=(bin[2]*256+bin[3])/4;
					send_step("010D\r", STEP_PID_SPEED);
				} else {
					// retry
					send_step("010C\r", STEP_PID_RPM);
				}
				break;

			// Speed request
			case STEP_PID_SPEED:
				if (bin[0]==0x41) {
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
					send_step("0104\r", STEP_PID_REQ);
				} else {
					send_step("010D\r", STEP_PID_SPEED);
				}
				break;

			}

		}

	}

	// finish
	return destroy();

}
