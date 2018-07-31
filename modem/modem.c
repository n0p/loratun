#include "modem.h"

int fd=0;
int line_buf_pos=0;
char line_buf[MAX_LINE];

/*
 * Read a line from serial fd
 */
int line_read(int fd, char *line) {
	int i;
	int n;
	while (1) {
		n=serial_read(fd, (char *)&line_buf+line_buf_pos, MAX_LINE - line_buf_pos);
		if (n<0) return n;
		for (i=0;i<n;i++)
			if (line_buf[line_buf_pos+i]=='\n') {
				int aux=line_buf_pos+n;
				memcpy(line, line_buf, aux);
				line_buf_pos=0;
				return aux;
			}
		line_buf_pos+=n;
		if (line_buf_pos>=MAX_LINE) {
			return 0;
		}
		//debug(line_buf, line_buf_pos);
	}
	return n;
}

/*
 * Get and parse a response from the modem
 */ 
int resp_read(int fd)
{
		char line[MAX_LINE+1];
		{
			int n=line_read(fd, (char *)&line);
			if (n!=0) { // is the buffer empty?
				if (strncmp("\r\n", line, 2) != 0 ) { // is it an empty line?
					line[n]=0;
					printf("GOT %d bytes: %s \n", n, line);
				}
			}
		}
}

int loratun_modem_check_joined()
{
	
	
}


/*
 * Open the serial FD, parse the parameters, 
 * configure the modem and join the network
 */
int loratun_modem_init(List *param) {
	char equals = '=';
	char newline = '\n';
	char* serport;
	
	Node *n=param->first;
	while (n){ // Iterate and parse all config values
		Config *c=(Config *)n->e;
		if ( ( strlen(c->key) == 10 ) && 
				 (strncmp("SerialPort", c->key, 10) == 0 ) )
		{ // Got our serial port
			printf("Serial port = %s\n", c->value);
			serport = malloc(strlen(c->value)+1);
			sprintf(serport, "%s", c->value);
			break;
		}
		n=n->sig;
	}
	
 	fd=serial_open(serport);
	if (fd) {
		serial_set_interface_attribs(fd, B9600, 0); // set speed, no parity
		serial_set_blocking(fd, 0); // set blocking
		printf("Serial opened; sending ATZ reset cmd\n");
		serial_write(fd, "ATZ\n", 4); // reset the LoRaWAN modem
		sleep(1); // wait for the modem
		
		serial_write(fd, "AT\n", 3);
		
		resp_read(fd);
		
		Node *n=param->first;
		while (n){ // Iterate and parse all config values
			Config *c=(Config *)n->e;
			if (strncmp("AT+", c->key, 3) == 0) { // Got a RAW AT config command
				printf("AT config send %s=%s\n", c->key, c->value);
				serial_write(fd, c->key, strlen(c->key));
				serial_write(fd, &equals, 1);
				serial_write(fd, c->value, strlen(c->value));
			} else printf("Config Key %s is not an AT command\n", c->key);
			n=n->sig;
		}
		
		printf("Sending JOIN request");
		serial_write(fd, "AT+JOIN\n", 8);
		resp_read(fd);
		
	} else {
		perror("Can't open serial port.");
	}

	return 0;

}

/*
 * Try to rx a packet from the network
 */
int loratun_modem_recv(char *data) {
	return 0;
}

/*
 * Send a packet to the network
 */
int loratun_modem_send(char *data, int len) {
	return 0;
}

/*
 * Deinit the serial and free()
 */
int loratun_modem_destroy() {
	printf("Modem: Destroy\n");
	if (fd) serial_close(fd);
	printf("Modem: Closed serial port\n");
	return 0;
}
