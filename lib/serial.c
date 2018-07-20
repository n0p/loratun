#include "serial.h"

int serial_set_interface_attribs(int fd, int speed, int parity) {

	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		perror("tcgetattr");
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	//tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

int serial_set_blocking(int fd, int should_block) {
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		perror("tggetattr");
		return -1;
	}
	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("setting term attributes");
		return -1;
	}
	return 0;
}

int serial_open(char* device) {
	int fd=open(device, O_RDWR | O_NOCTTY | O_SYNC); // O_NONBLOCK O_NDELAY
	if (fd < 0) {
		perror("Serial: Opening");
		return 0;
	}
	return fd;
}

int serial_read(int fd, char *buf, int len) {

	fd_set rd_set;

	FD_ZERO(&rd_set);
	FD_SET(fd, &rd_set);
	int ret = select(fd+1, &rd_set, NULL, NULL, NULL);
	if (ret < 0 && errno == EINTR) return -1;
	else if (ret < 0) {
		perror("select()");
		return -1;
	}

	//if (FD_ISSET(tap_fd, &rd_set)) {
	int n=read(fd, buf, len);
	return (n?n:-1);

}

int serial_write(int fd, char *buf, int len) {
	return write(fd, buf, len);
}

void serial_close(int fd) {
	close(fd);
}
