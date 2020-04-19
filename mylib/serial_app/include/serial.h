#ifndef _SERIAL_H
#define _SERIAL_H

int toBaudrate(int rate);
int serialOpen(const char *dev, int speed);
void serialClose(int fd);
int serialReadWithTimeout(int fd, unsigned char *data, int len, int ms);
int serialWrite(int fd, const unsigned char *data, int len);
int serialRead(int fd, unsigned char *data, int len);


#endif


