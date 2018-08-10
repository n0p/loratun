#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h> 

#include <errno.h>
//#include <sys/time.h>
//#include <sys/types.h> 
#include <sys/select.h>

//int ret=system("/bin/stty -F /dev/ttyACM0 9600 cs8 -cstopb -parenb ignbrk -brkint -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts");

int serial_set_interface_attribs(int fd, int speed, int parity);
int serial_set_blocking(int fd, int should_block);
int serial_open(char* device);
int serial_read(int fd, char *buf, int len);
int serial_aread(int fd, char *buf, int len);
int serial_write(int fd, char *buf, int len);
void serial_close(int fd);

#endif
