#include "modem.h"

int loratun_modem(List *param) {
	printf("Modem: Loop\n");
	sleep(4);
	return 0;
}

char *loratun_modem_error() {
	return "";
}

int loratun_modem_send(char *data, int len) {
	//printf("*** voy a enviar len=%d!\n", len);
	char *s=malloc((len+10)*sizeof(char));
	//sprintf(s, "LEN(%d)",len);
	memcpy(s, "ECHO:", 5);
	memcpy(s+5, data, len);
	s[len+5]=0;
	loratun_modem_recv(s, 3);
	//free(s);
	return 0;
}

int loratun_modem_destroy() {
	printf("Modem: Destroy\n");
	return 0;
}
