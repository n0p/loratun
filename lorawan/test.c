#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>

#include "../lib/modem.h"

bool terminated=true;
bool received=false;
pthread_mutex_t tun_mutex;

// output to console
void con(char *msg, ...) {
	va_list argp;
	va_start(argp, msg);
	vfprintf(stdout, msg, argp);
	va_end(argp);
	fflush(stdout);
}

// output to error
void err(char *msg, ...) {
	va_list argp;
	va_start(argp, msg);
	vfprintf(stderr, msg, argp);
	va_end(argp);
	fflush(stderr);
}

// data reception
int modemtun_modem_recv(uint8_t *data, int len) {
	con("[TEST] RECEIVED len(%d)=%s\n", len, data);
	received=true;
	return 0;
}

// modem thread wrapper
int *modem_thread(void *p) {
	modemtun_modem(modem_config);
	terminated=true;
	received=true;
	return NULL;
}

// main application
int main() {

	// thread
	pthread_t modem_thread_t;

	// quit signal handler
	void sig_handler(int signo) {
		con("\n");
		modemtun_modem_destroy();
		modem_config_destroy();
	}

	// initialize list of modem configurations
	modem_config_init();

	// register signals
	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	// add needed configurations
	modem_config_add("SerialPort", "/dev/ttyACM0");
	 // App keys for pleiades gateway
	modem_config_add("AT+APPKEY", "ce:e3:fa:63:e5:ee:e8:ff:28:dd:08:69:ca:48:87:1c");
	modem_config_add("AT+NWSKEY", "cb:c9:a5:4e:de:35:4e:c9:33:61:cf:01:c3:71:e7:e2");
	modem_config_add("AT+DADDR", "06:c6:f5:c0");
	modem_config_add("AT+APPSKEY", "81:3f:2e:ad:c7:ec:ce:26:ae:b2:33:87:a9:b9:cb:45");
	
	/*
	// App keys for TTN test app
	modem_config_add("AT+APPKEY", "35:4D:D6:24:10:56:7E:3C:8F:97:75:2C:EA:F4:05:F1");
	modem_config_add("AT+APPEUI", "70:B3:D5:7E:D0:00:99:FB");
	*/
	
	modem_config_add("AT+ADR", "0");
	modem_config_add("AT+DR", "7");
	modem_config_add("AT+DCS", "0");
	modem_config_add("AT+TXP", "3");
	modem_config_add("AT+CLASS", "C");
	modem_config_add("AT+NJM", "1");

	// start print debug
	con("[TEST] starting with next configurations:\n");
	modem_config_print();

	// initialize mutex
	if (pthread_mutex_init(&tun_mutex, NULL) != 0) {
		perror("[TEST] tun_mutex init");
		return 1;
	}

	// start thread
	if (pthread_create(&modem_thread_t, NULL, (void *)modem_thread, 0)) {
		perror("[TEST] error creating thread");
		return 1;
	}

	// send some test data
	con("[TEST] send test\n");
	uint8_t testpkt [] = {0x07, 0x40, 0xD4, 0x6C, 0x48, 0x4F, 0x4C, 0x40, 0x21, 0x00};
	modemtun_modem_send(testpkt, sizeof(testpkt));

	// wait until we receive data back
	while (!received) usleep(100000);

	// request termination
	con("[TEST] Requesting termination");
	sig_handler(SIGQUIT);

	/// wait for the thread to finish
	if (pthread_join(modem_thread_t, NULL)) {
		perror("[TEST] Error joining thread");
		return 2;
	}

	// remove handlers
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);

	// all ok
	return 0;

}
