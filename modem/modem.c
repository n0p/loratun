#include "modem.h"

int fd=0;
int line_buf_pos=0;
char line_buf[MAX_LINE];
char scrapbuf[MAX_LINE];

/*
 * Are we up and running?
 */
int get_init_status()
{
	if (fd == 0) { 
		perror ("Uninitialized serial port"); 
		return -1;
	}
	return 0;
}

/*
 * Read a line from serial fd
 */
int line_read(char *line) {
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
int resp_read(char* response)
{
	if ( get_init_status() < 0 ) return -1;
	
	char line[MAX_LINE+1];
	int n=line_read((char *)&line);
	if (n<=1) return resp_read(response); // is the buffer empty?
	if (strncmp("\r\n", line, 2) == 0 ) return resp_read(response); // is it an empty line?
	line[n]=0;
	printf("SERIAL DEBUG: GOT %d bytes: %s \n", n, line);
	
	if (strncmp("OK", line, 2) == 0 ) return 1;
	
	if (strncmp("AT_ERROR", line, 8) == 0 ) {
		perror ("Modem: generic error");
		return -1;
	}
	if (strncmp("AT_PARAM_ERROR", line, 14) == 0 ) {
		perror ("Modem: AT parameter error");
		return -2;
	}
	if (strncmp("AT_BUSY_ERROR", line, 13) == 0 ) {
		perror ("Modem: busy");
		return -3;
	}
	if (strncmp("AT_TEST_PARAM_OVERFLOW", line, 22) == 0 ) {
		perror ("Modem: command parameter too long");
		return -4;
	}
	if (strncmp("AT_NO_NETWORK_JOINED", line, 20) == 0 ) {
		perror ("Modem: not joined");
		return -5;
	}
	if (strncmp("AT_RX_ERROR", line, 11) == 0 ) {
		perror ("Modem: error during RX");
		return -6;
	}
	strncpy (response, line, strlen(line)-2); // -2 to avoid copying \r\n
	return 1;
}

/*
 * Ask the modem if we already joined the network
 */
int loratun_modem_check_joined()
{
	if ( get_init_status() < 0 ) return -1;
	
	printf("Checking JOIN status\n");
	serial_write(fd, "AT+NJS=?\n", 8);
	resp_read(scrapbuf);
	printf("Got response: %s",scrapbuf);
	return -1;
}

/*
 * Try again to join the network
 */
int loratun_modem_retry_join()
{
	if ( get_init_status() < 0 ) return -1;
	return -1;
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
		resp_read(scrapbuf);
		
		Node *n=param->first;
		while (n){ // Iterate and parse all config values
			Config *c=(Config *)n->e;
			if (strncmp("AT+", c->key, 3) == 0) { // Got a RAW AT config command
				printf("AT config send %s=%s\n", c->key, c->value);
				serial_write(fd, c->key, strlen(c->key));
				serial_write(fd, &equals, 1);
				serial_write(fd, c->value, strlen(c->value));
				serial_write(fd, &newline, 1);
				resp_read(scrapbuf);
			} else printf("Config Key %s is not an AT command\n", c->key);
			n=n->sig;
		}
		
		printf("Sending JOIN request\n");
		serial_write(fd, "AT+JOIN\n", 8);
		resp_read(scrapbuf);
		
	} else {
		perror("Can't open serial port.");
	}

	return 0;

}

/*
 * Try to rx a packet from the network
 */
int loratun_modem_recv(char *data) {
	if ( get_init_status() < 0 ) return -1;
	
	return 0;
}

/*
 * Send a packet to the network
 */
int loratun_modem_send(char *data, int len) {
	if ( get_init_status() < 0 ) return -1;
	
	return 0;
}

/*
 * Deinit the serial and free()
 */
int loratun_modem_destroy() {
	if ( get_init_status() < 0 ) return -1;
	
	printf("Modem: Destroy\n");
	if (fd) serial_close(fd);
	fd = 0;
	printf("Modem: Closed serial port\n");
	return 0;
}
