#include <serial.h>
#include <stdio.h>
#include <errno.h>

int main()
{
	const char* dev = "/dev/ttyS0";
	int rate, fd;
	rate = toBaudrate(115200);
	unsigned char readBuf[13];
	int readNum;

	fd = serialOpen(dev, rate);
    if (fd < 0)
    {
        perror(dev);
        return -1;
    }
	while(1)
	{
		readNum = serialReadWithTimeout(fd, readBuf, sizeof(readBuf), 500);
		if(readNum)
		{
			printf("read data is: %s \n", readBuf);
		}
	}
	return 0;
}
