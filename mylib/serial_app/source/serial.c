#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>    
#include <sys/ioctl.h>
#include <termios.h>   /*终端控制函数定义*/
#include <sys/select.h>
#include <sys/time.h>

static void set_baudrate (struct termios *opt, unsigned int baudrate)
{
	cfsetispeed(opt, baudrate);
	cfsetospeed(opt, baudrate);
}

static void set_data_bit (struct termios *opt, unsigned int databit)
{
    opt->c_cflag &= ~CSIZE;
    switch (databit) {
    case 8:
        opt->c_cflag |= CS8;
        break;
    case 7:
        opt->c_cflag |= CS7;
        break;
    case 6:
        opt->c_cflag |= CS6;
        break;
    case 5:
        opt->c_cflag |= CS5;
        break;
    default:
        opt->c_cflag |= CS8;
        break;
    }
}



static void set_parity (struct termios *opt, char parity)
{
    switch (parity) {
    case 'N':                  /* no parity check */
        opt->c_cflag &= ~PARENB;
        break;
    case 'E':                  /* even */
        opt->c_cflag |= PARENB;
        opt->c_cflag &= ~PARODD;
        break;
    case 'O':                  /* odd */
        opt->c_cflag |= PARENB;
        opt->c_cflag |= ~PARODD;
        break;
    default:                   /* no parity check */
        opt->c_cflag &= ~PARENB;
        break;
    }
}


static void set_stopbit (struct termios *opt, const char *stopbit)
{
    if (0 == strcmp (stopbit, "1")) {
        opt->c_cflag &= ~CSTOPB; 	/* 1 stop bit */
    }	else if (0 == strcmp (stopbit, "1")) {
        opt->c_cflag &= ~CSTOPB; 	/* 1.5 stop bit */
    }   else if (0 == strcmp (stopbit, "2")) {
        opt->c_cflag |= CSTOPB;  /* 2 stop bits */
    } else {
        opt->c_cflag &= ~CSTOPB; 	/* 1 stop bit */
    }
}


int serialInit(int fd, int speed)
{
	struct termios set;

	/* get terminal parameter */
    if (tcgetattr(fd, &set) != 0)
    {
        printf("***tcgetattr(fd=%d) failed, errno=%d\n", fd, errno);
        return -1;
    }
 
    set_baudrate(&set, speed);
    /* set data bits */

    set.c_cflag |= (CLOCAL | CREAD);        //设置控制模式状态，本地连接，接收使能  
    set.c_cflag &= ~CRTSCTS;                //无硬件流控  
	set_data_bit(&set, 8);
	set_parity(&set, 'N');
	set_stopbit(&set, "1");
    
    set.c_oflag = 0;                        //输出模式  
    set.c_lflag = 0;                        //不激活终端模式  
    set.c_cc[VMIN] = 1;    
    set.c_cc[VTIME] = 0;    

    if (tcsetattr(fd, TCSANOW, &set) != 0)
    {
        printf("***tcsetattr(fd=%d) failed, errno=%d\n", fd, errno);
        return -1;
    }
	return 0;
}


int serialOpen(const char *dev, int speed)
{
    int fd;
    fd = open(dev, O_NONBLOCK | O_RDWR | O_NOCTTY);

    if (fd == -1)
    {
        printf("***open(%s) failed, errno=%d\n", dev, errno);
        return -1;
    }

    if (serialInit(fd, speed) < 0)
    {
        close(fd);
        return -1;
    }
	
    return fd;
}

int toBaudrate(int rate)
{
    switch(rate)
    {
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;  
        case 115200:
            return B115200;  
    }

    return -1;
}

void serialClose(int fd)
{
    close(fd);
}

int serialWrite(int fd, const unsigned char *data, int len)
{
    return write(fd, data, len);
}

int serialRead(int fd, unsigned char *data, int len)
{
    return read(fd, data, len);
}

int waitReadEvent(int ms, int fd)
{
    struct timeval tv;
    fd_set fds;
    int maxfd, ret;

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    maxfd = -1;
     
    FD_ZERO(&fds);

    /* add signal */
    if (fd > maxfd)
    {
        maxfd = fd;
    }
    
    FD_SET(fd, &fds);

    ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0) 
    {
        if (FD_ISSET(fd, &fds))
        {
            return 1;
        }         
    }

    return 0;
}

int serialReadWithTimeout(int fd, unsigned char *data, int len, int ms)
{
    int ret;
    
    if (waitReadEvent(ms, fd))
    {
        ret = read(fd, data, len);
    
        if (ret > 0)
        {
            return ret;
        }
        else if (ret < 0)
        {
            return 0;
        }
    }
    /* timeout */
    return 0;
}
