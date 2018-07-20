#include "modem.h"

int fd=0;
int line_buf_pos=0;
char line_buf[MAX_LINE];

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


int loratun_modem_init(List *param) {

	char line[MAX_LINE+1];
	fd=serial_open("/dev/ttyACM0");
	if (fd) {
		serial_set_interface_attribs(fd, B9600, 0); // set speed, no parity
		serial_set_blocking(fd, 1); // set blocking
		serial_write(fd, "AT\n", 3);
		serial_write(fd, "AT+DEUI=?\n", 10);
		while (1)
		{
			int n=line_read(fd, (char *)&line);
			if (n<0) break;
			line[n]=0;
			printf("LINE(%d)=%s", n, line);
		}
	} else {
		printf("No puedo abrir el puerto serie.\n");
	}

	return 0;

}

int loratun_modem_recv(char *data) {
	return 0;
}

int loratun_modem_send(char *data, int len) {
	return 0;
}

int loratun_modem_destroy() {
	printf("Modem: Destroy\n");
	if (fd) serial_close(fd);
	return 0;
}
