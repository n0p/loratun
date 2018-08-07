#include "modem.h"
#include "../lib/serial.h"
#include "../lib/debug.h"

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
		debug(line_buf,strlen(line_buf));
		for (i=0;i<n;i++)
			if ((line_buf[line_buf_pos+i]=='\n')||(line_buf[line_buf_pos+i]=='\r')) {
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
	int n=line_read(line);
	
	printf("resp_read() - ");
	debug(line,n);
	
	if (n<=1) return resp_read(response); // is the buffer empty?
	if (strncmp("\r\n", line, 2) == 0 ) return resp_read(response); // is it an empty line?
	if (strncmp("\r", line, 1) == 0 ) return resp_read(response); // is it an empty line?
	if (strncmp("\n", line, 1) == 0 ) return resp_read(response); // is it an empty line?
	line[n]=0;
	printf("SERIAL DEBUG: GOT %d bytes: %s \n", n, line);
	
	if (strncmp("OK", line, 2) == 0 ) return 1;
	
	if (strncmp("AT_ERROR", line, 8) == 0 ) {
		printf ("Modem: generic error\n");
		return -1;
	}
	if (strncmp("AT_PARAM_ERROR", line, 14) == 0 ) {
		printf ("Modem: AT parameter error\n");
		return -2;
	}
	if (strncmp("AT_BUSY_ERROR", line, 13) == 0 ) {
		printf ("Modem: busy\n");
		return -3;
	}
	if (strncmp("AT_TEST_PARAM_OVERFLOW", line, 22) == 0 ) {
		printf ("Modem: command parameter too long\n");
		return -4;
	}
	if (strncmp("AT_NO_NETWORK_JOINED", line, 20) == 0 ) {
		printf ("Modem: not joined\n");
		return -5;
	}
	if (strncmp("AT_RX_ERROR", line, 11) == 0 ) {
		printf ("Modem: error during RX\n");
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
	
	usleep(100000);
	printf("Checking JOIN status\n");
	serial_write(fd, "AT+NJS=?\n", 9);
	usleep(100000);
	resp_read(scrapbuf);
	if (scrapbuf[0] == '0') { 
		printf("We are OFFline\n");
		return 0;
	}
	else if (scrapbuf[0] == '1') { 
		printf("We are ONline\n");
		return 1;
	}
	printf("Unknown response to Network Join Status: %s\n",scrapbuf);
	return -1;
}

/*
 * Try again to join the network
 */
int loratun_modem_retry_join()
{
	if ( get_init_status() < 0 ) return -1;
	printf("Sending JOIN request\n");
	serial_write(fd, "AT+JOIN\n", 8);
	usleep(100000);
	resp_read(scrapbuf);
	return 0;
}

/*
 * Open the serial FD, parse the parameters, 
 * configure the modem and join the network
 */
int loratun_modem_init(List *param) {
	uint8_t equals = '=';
	uint8_t newline = '\n';
	uint8_t* serport;
	
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
				usleep(100000);
				resp_read(scrapbuf);
			} else printf("Config Key %s is not an AT command\n", c->key);
			n=n->sig;
		}
		
		printf("Sending JOIN request\n");
		serial_write(fd, "AT+JOIN\n", 8);
		resp_read(scrapbuf);
		return 0;
		
	} else {
		perror("Can't open serial port.");
		return -1;
	}

	return -99; // we should never get here

}

/*
 * Try to rx a packet from the network
 */
int check_loratun_modem_recv(uint8_t *data) {
	if ( get_init_status() < 0 ) return -1;
	uint8_t* cleanbuf;
	cleanbuf = malloc(2048);
	
	serial_write(fd, "AT+RECVB=?\n", 11);
	int code = resp_read(cleanbuf);
	printf ("RECV response: %s\n", cleanbuf);

	switch (code) {
		case 1: // command succeeded
			// printf ("pos:%d sz:%d\n", strcspn(cleanbuf, ":"), strlen(cleanbuf));
			if (strcspn(cleanbuf, ":") > strlen(cleanbuf)-1 ) { // Do we have something after portnumber?
				printf ("Got a packet!");
				strncpy (data, cleanbuf, strlen(cleanbuf));
				free(cleanbuf);
				return 1;
			} else {
				free(cleanbuf);
				return 0;
			}
		case -5: // we aren't joined
			perror("Not joined to any network");
			free(cleanbuf);
			return -5;
		default: // some other error
			perror("Unexpected RX error");
			free(cleanbuf);
			return -1;
	}
	free(cleanbuf);
	return 0;
}

/*
 * Send a packet to the network
 */
int loratun_modem_send(uint8_t *data, int len) {
	if ( get_init_status() < 0 ) return -1;
	uint8_t* cleanbuf;
	cleanbuf = malloc(2048);
	
	if (!loratun_modem_check_joined()) {
		printf ("Send error: Not joined\n");
		return -1;
	}
	
	sprintf(cleanbuf, "AT+SENDB=10:\n");
	
	// This is to hex-print a binary array
	const uint8_t * hex = "0123456789abcdef";
	uint8_t * pin = data;
	uint8_t * ptr = cleanbuf + strlen(cleanbuf) -1;
	int i = 0;
	for(; i < len-1; ++i){
		*ptr++ = hex[(*pin>>4)&0xF];
		*ptr++ = hex[(*pin++)&0xF];
		//*ptr++ = ':';
	}
	*ptr++ = hex[(*pin>>4)&0xF];
	*ptr++ = hex[(*pin)&0xF];
	*ptr = '\n';
	
	printf("DEBUG sending: %s\n", cleanbuf);
	
	serial_write(fd, cleanbuf, strlen(cleanbuf));
	int code = resp_read(cleanbuf);
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

/*
 * Modem management loop
 * TODO: Implement async behaviour
 */
int loratun_modem(List *param){

//	pthread_mutex_lock(&send_mutex);

	int abort = 0;
	int errcount = 0;
	uint8_t* pktbuf;
	pktbuf = malloc(2048);
	int initerror = loratun_modem_init(param);
	
	if (initerror < 0) {
		perror ("modem init failed!!");
		return initerror;
	}
	
	printf("Modem: Checking joined\n");
	loratun_modem_check_joined();
	
	printf("Modem: Loop()\n");
	while (!abort){
		switch (check_loratun_modem_recv(pktbuf)) {
			case 0:
				printf("Loop(): RECV got no data\n");
				errcount = 0;
				break;
			case 1:
				printf("Loop(): RECV got data!\n");
				errcount = 0;
				loratun_modem_recv(pktbuf, strlen(pktbuf));
				break;
			case -5:
				perror("Loop(): Could not RECV: Not joined");
				loratun_modem_retry_join();
			default:
				perror("Could not RECV");
				errcount++;
		}

		if (resp_read(scrapbuf) < 0){
			perror("got an error while on loop!!");
			errcount++;
		}

//		pthread_mutex_unlock(&send_mutex);
		sleep(1);
//		pthread_mutex_lock(&send_mutex);
		
		//** FOR TESTING AND DEBUG PURPOSES **//
		uint8_t testpkt [] = {0x07, 0x40, 0xD4, 0x6C, 0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00};
		loratun_modem_send(testpkt, sizeof(testpkt));
		sleep(1);
		
		if (errcount > 5) {
			perror("Loop() aborting: too many errors");
			abort = 1;
			loratun_modem_destroy();
			free(pktbuf);
			return -1;
		}
	}
	free(pktbuf);
	return 0;
}
