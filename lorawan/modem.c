#include "../lib/modem.h"
#include "../lib/serial.h"
//#include "../lib/debug.h"

#define MAX_LINE 1024

int fd=0;
int line_buf_pos=0;
char line_buf[MAX_LINE];
char scrapbuf[MAX_LINE];

/*
 * Are we up and running?
 */
int get_init_status() {
	if (fd == 0) { 
		err("Uninitialized serial port\n"); 
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
	
	char * line; // incoming line
	char * mline; // original buffer position to free()
	char * tokpos; // final data position to remove trailing \r\n
	line = malloc(MAX_LINE+1);
	int n=line_read(line);
	mline = line;
	
//	con("resp_read() - ");
//	debug(line,n);
	
	// is the buffer empty?
	if (n<=1) return resp_read(response);
	
	// remove leading \r\n
	while ((strncmp("\r", line, 1) == 0 ) || (strncmp("\n", line, 1) == 0 ))
	{
		if (strlen(line) == 0 ) return 0;
//		con("Moving forward -- ");
//		debug(line,strlen(line));
		line++;
	}
	line[n]=0;
//	con("SERIAL DEBUG: GOT %d bytes: %s \n", n, line);
	
	if (strncmp("OK", line, 2) == 0 ) {
		free (mline);
		return 1;
	}
	
	if (strncmp("AT_ERROR", line, 8) == 0 ) {
		err ("Modem resp_read(): generic error\n");
		free (mline);
		return -1;
	}
	if (strncmp("AT_PARAM_ERROR", line, 14) == 0 ) {
		err ("Modem resp_read(): AT parameter error\n");
		free (mline);
		return -2;
	}
	if (strncmp("AT_BUSY_ERROR", line, 13) == 0 ) {
		err ("Modem resp_read(): busy\n");
		free (mline);
		return -3;
	}
	if (strncmp("AT_TEST_PARAM_OVERFLOW", line, 22) == 0 ) {
		err ("Modem resp_read(): command parameter too long\n");
		free (mline);
		return -4;
	}
	if (strncmp("AT_NO_NETWORK_JOINED", line, 20) == 0 ) {
		err ("Modem resp_read(): not joined\n");
		free (mline);
		return -5;
	}
	if (strncmp("AT_RX_ERROR", line, 11) == 0 ) {
		err ("Modem resp_read(): error during RX\n");
		free (mline);
		return -6;
	}
	
	tokpos = strtok(line, "\r\n"); // how far is our \r\n?
	strncpy (response, line, line-tokpos+1);
//	con("line-tokpos=%d\ntokpos-",line-tokpos+1);
//	debug(tokpos,strlen(tokpos));
//	con("line-");
//	debug(line,strlen(line));

	free (mline);
	return 1; // If the modem returns data, there is no error according to the datasheet
}

/*
 * Ask the modem if we already joined the network
 */
int modemtun_modem_check_joined()
{
	if ( get_init_status() < 0 ) return -1;
	
	serial_write(fd, "AT+NJS=?\n", 9);
	usleep(100000);
	resp_read(scrapbuf);
	if (scrapbuf[0] == '0') { 
		con("modemtun_modem_check_joined(): We are OFFline\n");
		return 0;
	}
	else if (scrapbuf[0] == '1') { 
		con("modemtun_modem_check_joined(): We are ONline\n");
		return 1;
	}
	err("modemtun_modem_check_joined(): Unknown response to Network Join Status: %s\n",scrapbuf);
	return -1;
}

/*
 * Try again to join the network
 */
int modemtun_modem_retry_join()
{
	if ( get_init_status() < 0 ) return -1;
	con("modemtun_modem_retry_join(): Sending JOIN request\n");
	serial_write(fd, "AT+JOIN\n", 8);
	usleep(100000);
	resp_read(scrapbuf);
	return 0;
}

/*
 * Open the serial FD, parse the parameters, 
 * configure the modem and join the network
 */
int modemtun_modem_init(List *param) {
	uint8_t equals = '=';
	uint8_t newline = '\n';
	uint8_t* serport = NULL;

	Node *n=param->first;
	while (n){ // Iterate and parse all config values
		ModemConfig *c=(ModemConfig *)n->e;
		if ( (strncmp("SerialPort", c->key, 10) == 0 ) )
		{ // Got our serial port
			con("modemtun_modem_init(): Serial port = %s\n", c->value);
			serport = malloc(strlen(c->value)+1);
			sprintf(serport, "%s", c->value);
			break;
		}
		n=n->sig;
	}

	// is serial port defined?
	if (!serport) {
		perror("SerialPort configuration not defined.");
		return -1;
	}

	// open serial port
 	fd=serial_open(serport);
	if (fd) {
		serial_set_interface_attribs(fd, B9600, 0); // set speed, no parity
		serial_set_blocking(fd, 0); // set blocking
		
		con("modemtun_modem_init(): Serial opened; sending ATZ reset cmd\n");
		serial_write(fd, "ATZ\n", 4); // reset the LoRaWAN modem
		sleep(1); // wait for the modem
		
		serial_write(fd, "AT\n", 3);
		resp_read(scrapbuf);
		
		Node *n=param->first;
		while (n){ // Iterate and parse all config values
			ModemConfig *c=(ModemConfig *)n->e;
			if (strncmp("AT+", c->key, 3) == 0) { // Got a RAW AT config command
				con("modemtun_modem_init(): AT config send %s=%s\n", c->key, c->value);
				serial_write(fd, c->key, strlen(c->key));
				serial_write(fd, &equals, 1);
				serial_write(fd, c->value, strlen(c->value));
				serial_write(fd, &newline, 1);
				usleep(100000);
				resp_read(scrapbuf);
			} else con("modemtun_modem_init(): Config Key %s is not an AT command\n", c->key);
			n=n->sig;
		}
		
		con("modemtun_modem_init(): Sending JOIN request\n");
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
int check_modemtun_modem_recv(uint8_t *data) {
	if ( get_init_status() < 0 ) return -1;
	uint8_t* cleanbuf;
	cleanbuf = malloc(2048);
	
	serial_write(fd, "AT+RECVB=?\n", 11);
	int code = resp_read(cleanbuf);
	con ("check_modemtun_modem_recv(): response: %s\n", cleanbuf);

	switch (code) {
		case 1: // command succeeded
//			con ("pos:%d sz:%d\n", strcspn(cleanbuf, ":"), strlen(cleanbuf));
			if (strcspn(cleanbuf, ":") > strlen(cleanbuf) ) { // Do we have something after portnumber?
				con ("check_modemtun_modem_recv(): Got a packet!\n");
				strncpy (data, cleanbuf, strlen(cleanbuf));
				free(cleanbuf);
				return 1;
			} else {
				free(cleanbuf);
				return 0;
			}
		case -5: // we aren't joined
			err("check_modemtun_modem_recv(): Not joined to any network\n");
			free(cleanbuf);
			return -5;
		default: // some other error
			err("check_modemtun_modem_recv(): Unexpected RX error\n");
			free(cleanbuf);
			return -1;
	}
	free(cleanbuf);
	return 0;
}

/*
 * Send a packet to the network
 */
int modemtun_modem_send(uint8_t *data, int len) {
	if ( get_init_status() < 0 ) return -1;
	uint8_t* cleanbuf;
	cleanbuf = malloc(2048);
	
	if (!modemtun_modem_check_joined()) {
		con ("modemtun_modem_send(): ERROR: Not joined - Not trying\n");
		return -1;
	}


	usleep(200000);
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
	
	con("modemtun_modem_send(): sending: %s", cleanbuf);
	
	usleep(100000);
	serial_write(fd, cleanbuf, strlen(cleanbuf));
	usleep(100000);
	
	int code = resp_read(cleanbuf);
	switch (code) {
		case -1:
			err("modemtun_modem_send(): ERROR: Generic: Modem might be busy\n");
			break;
		case -3: // modem busy
			err("modemtun_modem_send(): ERROR: Modem is busy!\n");
			break;
		case -2: // param error
			err("modemtun_modem_send(): ERROR: Parameter error!\n");
			break;
		case -5: // not joined
			err("modemtun_modem_send(): ERROR: Disconnected from the network!\n");
			break;
		case 1:
		case 0:
			con("modemtun_modem_send(): Ok\n");
			break;
		default:
			err("modemtun_modem_send(): Unexpected output from modem: %d\n", code);
	}
	return code;
}

/*
 * Deinit the serial and free()
 */
int modemtun_modem_destroy() {
	if ( get_init_status() < 0 ) return -1;
	
	con("modemtun_modem_destroy(): Destroy\n");
	if (fd) serial_close(fd);
	fd = 0;
	con("modemtun_modem_destroy(): Closed serial port\n");
	return 0;
}

/*
 * Modem management loop
 * TODO: Implement async behaviour
 */
int modemtun_modem(List *param){

//	pthread_mutex_lock(&send_mutex);

	int abort = 0;
	int errcount = 0;
	uint8_t* pktbuf;
	pktbuf = malloc(2048);
	int initerror = modemtun_modem_init(param);
	
	if (initerror < 0) {
		perror ("modem init failed!!");
		return initerror;
	}
	
	con("modemtun_modem(): Checking joined\n");
	modemtun_modem_check_joined();
	
	con("Modem: Loop()\n");
	while (!abort){
		switch (check_modemtun_modem_recv(pktbuf)) {
			case 0:
				con("Loop(): RECV got no data\n");
				errcount = 0;
				break;
			case 1:
				con("Loop(): RECV got data!\n");
				errcount = 0;
				modemtun_modem_recv(pktbuf, strlen(pktbuf));
				break;
			case -5:
				perror("Loop(): Could not RECV: Not joined");
				modemtun_modem_retry_join();
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
		modemtun_modem_send(testpkt, sizeof(testpkt));
		sleep(1);
		
		if (errcount > 5) {
			perror("Loop() aborting: too many errors");
			abort = 1;
			modemtun_modem_destroy();
			free(pktbuf);
			return -1;
		}
	}
	free(pktbuf);
	return 0;
}
