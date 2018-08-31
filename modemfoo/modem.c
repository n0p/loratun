#include <unistd.h>
#include <stdbool.h>

#include "../lib/modem.h"

bool running=false;
int copybuf_len;
uint8_t *copybuf=NULL;

// aux: free copy buffer
void copybuf_free() {
	if (copybuf) {
		free(copybuf);
		copybuf=NULL;
	}
}

// modem loop
int modemtun_modem(List *param) {
	con("[MODEM] started (sample key=%s)\n", modem_config_get("key"));
	running=true;
	while (running) {
		usleep(100000);
		pthread_mutex_lock(&tun_mutex);
		if (copybuf) {
			modemtun_modem_recv(copybuf, copybuf_len);
			copybuf_free();
		}
		pthread_mutex_unlock(&tun_mutex);
	}
	con("[MODEM] finished\n");
	return 0;
}

// send data from interface to modem
int modemtun_modem_send(uint8_t *data, int len) {
	if (!running) while (!running) usleep(100000); // wait for modem initialization
	pthread_mutex_lock(&tun_mutex);
	con("[MODEM] received %d bytes from the interface, sending them back.\n", len);
	copybuf_len=len;
	if (copybuf) free(copybuf);
	copybuf=malloc(len);
	memcpy(copybuf, data, len);
	pthread_mutex_unlock(&tun_mutex);
	return 0;
}

// modem cleanup
int modemtun_modem_destroy() {
	con("[MODEM] destroy\n");
	pthread_mutex_lock(&tun_mutex);
	copybuf_free();
	pthread_mutex_unlock(&tun_mutex);
	running=false;
	return 0;
}
