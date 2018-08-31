#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>

#include "../lib/modem.h"

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
	received++;
	return 0;
}

// modem thread wrapper
int *modem_thread(void *p) {
	modemtun_modem(modem_config);
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
	modem_config_add("key", "value");

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
	modemtun_modem_send((uint8_t *)"ABC\0", 4);

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
