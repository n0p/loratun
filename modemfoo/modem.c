#include <unistd.h>

#include "../lib/modem.h"

#define true 1
#define false 0

int running=true;
uint8_t *copybuf=NULL;
int copybuf_len;

// aux: free copy buffer
void copybuf_free() {
	if (copybuf) {
		free(copybuf);
		copybuf=NULL;
	}
}

// modem loop
int loratun_modem(List *param) {
	con("[MODEM] started (sample key=%s)\n", modem_config_get("key"));
	while (running) {
		usleep(100000);
		pthread_mutex_lock(&tun_mutex);
		if (copybuf) {
			loratun_modem_recv(copybuf, copybuf_len);
			copybuf_free();
		}
		pthread_mutex_unlock(&tun_mutex);
	}
	con("[MODEM] finished\n");
	return 0;
}

// send data from interface to modem
int loratun_modem_send(uint8_t *data, int len) {
	con("[MODEM] sending %d bytes!\n", len);
	copybuf_len=len;
	if (copybuf) free(copybuf);
	copybuf=malloc(len);
	memcpy(copybuf, data, len);
	return 0;
}

// modem cleanup
int loratun_modem_destroy() {
	con("[MODEM] destroy\n");
	pthread_mutex_lock(&tun_mutex);
	copybuf_free();
	pthread_mutex_unlock(&tun_mutex);
	running=false;
	return 0;
}
