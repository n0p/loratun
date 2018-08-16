#include "serial.h"

// set basic interface attributes
int serial_set_interface_attribs(int fd, int speed, int parity) {

	struct termios tty;

	// get current attributes
	memset (&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		perror("tcgetattr");
		return -1;
	}

	// set input and output speed
	cfsetispeed(&tty, speed);
	cfsetospeed(&tty, speed);

	// read does not block
	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 0;
	//tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

	// set control modes
	tty.c_cflag &= ~CSIZE;             // bitmask
	tty.c_cflag |= CS8;                // raw 8-bit chars
	tty.c_cflag |= (CLOCAL | CREAD);   // enable reading, enable receiver, ignore modem control lines
	tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	// input: disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars
	tty.c_iflag &= ~IGNBRK;                 // disable break processing
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	// raw: no local canonical processing
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	// raw: no output processing
	tty.c_oflag = 0;

	// set attributes
	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

// change blocking mode
int serial_set_blocking(int fd, int should_block) {

	struct termios tty;

	// get current attributes
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		perror("tggetattr");
		return -1;
	}

	// change blocking mode
	tty.c_cc[VMIN]  = (should_block?1:0);
	tty.c_cc[VTIME] = 0; // 0 seconds read timeout

	// set attributes
	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("setting term attributes");
		return -1;
	}

	return 0;

}

// open serial port
int serial_open(char* device) {

	int fd=open(device, O_RDWR | O_NOCTTY | O_SYNC); // O_NONBLOCK O_NDELAY
	if (fd < 0) {
		perror("Serial: Opening");
		return 0;
	}

	return fd;

}

// blocking read if available
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

	int n=read(fd, buf, len);
	return (n?n:-1);

}

// async serial read
int serial_aread(int fd, char *buf, int len) {
	return read(fd, buf, len);
}

// write to serial
int serial_write(int fd, char *buf, int len) {
	return write(fd, buf, len);
}

// close serial port
void serial_close(int fd) {
	close(fd);
}
