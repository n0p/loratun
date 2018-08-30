#include "../lib/modem.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>

pthread_mutex_t tun_mutex;

// output to console
void con(char *msg, ...) {
	va_list argp;
	va_start(argp, msg);
	vfprintf(stdout, msg, argp);
	va_end(argp);
	fflush(stdout);
}

// data reception
int loratun_modem_recv(uint8_t *data, int len) {
	con("*** RECEIVED len(%d)=%s\n", len, data);
	return 0;
}

int *modem_thread(void *p) {
	loratun_modem(modem_config);
	return NULL;
}

int main() {

	void sig_handler(int signo) {
		con("\n");
		loratun_modem_destroy();
		modem_config_destroy();
		//exit(0);
	}

	// initialize list of modem configurations
	modem_config_init();

	// add needed configurations
	modem_config_add("SerialPort", "/dev/ttyACM0");
	modem_config_add("key", "value");
	modem_config_print();

	if (signal(SIGINT,  sig_handler) == SIG_ERR) perror("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) perror("\ncan't catch SIGQUIT\n");

	// thread
	pthread_t modem_thread_t;

	// initialize mutex
	if (pthread_mutex_init(&tun_mutex, NULL) != 0) {
		perror("send_mutex");
		return 1;
	}

	// start thread
	if (pthread_create(&modem_thread_t, NULL, (void *)modem_thread, 0)) {
		perror("Error creating thread");
		return 1;
	}

	// send some test data
	int i=0;
	for (i=0;i<3;i++) {
		con("SENDING #%d\n", i);
		loratun_modem_send((uint8_t *)"ABC\0", 4);
		sleep(1);
	}

	/// wait for the thread to finish
	con("Press Ctrl+C to end modem test\n");
	if (pthread_join(modem_thread_t, NULL)) {
		perror("Error joining thread");
		return 2;
	}

	// remove handlers
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);

	// all ok
	return 0;

}
