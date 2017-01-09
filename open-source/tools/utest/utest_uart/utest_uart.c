#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>

struct speed_map
{
  const char *string;      /* ASCII representation. */
  speed_t speed;           /* Internal form. */
  unsigned long int value; /* Numeric value. */
};

static struct speed_map speeds[] =
{
  {"0", B0, 0},
  {"50", B50, 50},
  {"75", B75, 75},
  {"110", B110, 110},
  {"134", B134, 134},
  {"134.5", B134, 134},
  {"150", B150, 150},
  {"200", B200, 200},
  {"300", B300, 300},
  {"600", B600, 600},
  {"1200", B1200, 1200},
  {"1800", B1800, 1800},
  {"2400", B2400, 2400},
  {"4800", B4800, 4800},
  {"9600", B9600, 9600},
  {"19200", B19200, 19200},
  {"38400", B38400, 38400},
  {"exta", B19200, 19200},
  {"extb", B38400, 38400},
#ifdef B57600
  {"57600", B57600, 57600},
#endif
#ifdef B115200
  {"115200", B115200, 115200},
#endif
#ifdef B230400
  {"230400", B230400, 230400},
#endif
#ifdef B460800
  {"460800", B460800, 460800},
#endif
#ifdef B500000
  {"500000", B500000, 500000},
#endif
#ifdef B576000
  {"576000", B576000, 576000},
#endif
#ifdef B921600
  {"921600", B921600, 921600},
#endif
#ifdef B1000000
  {"1000000", B1000000, 1000000},
#endif
#ifdef B1152000
  {"1152000", B1152000, 1152000},
#endif
#ifdef B1500000
  {"1500000", B1500000, 1500000},
#endif
#ifdef B2000000
  {"2000000", B2000000, 2000000},
#endif
#ifdef B2500000
  {"2500000", B2500000, 2500000},
#endif
#ifdef B3000000
  {"3000000", B3000000, 3000000},
#endif
#ifdef B3500000
  {"3500000", B3500000, 3500000},
#endif
#ifdef B4000000
  {"4000000", B4000000, 4000000},
#endif
  {NULL, 0, 0}
};

static char uart_device[56] = {0};

static char const* get_uart_name(int num)
{
    memset(uart_device, 0, sizeof(uart_device));
    sprintf(uart_device, "%s%d", "/dev/ttyS", num);
    return uart_device;
}

static unsigned long int baud_to_value(speed_t speed)
{
    int i;

    for (i = 0; speeds[i].string != NULL; ++i)
        if (speed == speeds[i].speed)
            return speeds[i].value;

    return 0;
}

static int uart_open_port(const char *dev_name)
{
    int fd; /* File descriptor for the port */
    fd = open(dev_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (-1 == fd) {
        printf("Unable to open uart port %s.\n", dev_name);
        return -1;
    }

    if (isatty(fd) == 0) {
        printf("This is not a tty device.\n");
    }

    return fd;
}

static void display_speed(struct termios* _termios)
{
    if (cfgetispeed(_termios) == 0 || cfgetispeed(_termios) == cfgetospeed(_termios))
        printf("speed = %lu baud; \n", baud_to_value(cfgetospeed(_termios)));
    else
        printf("ispeed = %lu baud; ospeed = %lu baud; \n",
                baud_to_value(cfgetispeed(_termios)),
                baud_to_value(cfgetospeed(_termios)));
}

static void display_data_bit(struct termios* _termios)
{
    switch (_termios->c_cflag & CSIZE)
    {
    case CS5:
        printf("data bits = 5\n");
        break;
    case CS6:
        printf("data bits = 6\n");
        break;
    case CS7:
        printf("data bits = 7\n");
        break;
    default:
    case CS8:
        printf("data bits = 8\n");
        break;
    }
}

static void display_parity_bit(struct termios* _termios)
{
    if (_termios->c_cflag & PARENB)
        if (_termios->c_cflag & PARODD)
            printf("parity = odd\n");
        else
            printf("parity = even\n");
    else
        printf("parity = none\n");
}

static void display_stop_bit(struct termios* _termios)
{
    if (_termios->c_cflag & CSTOPB)
        printf("stop bits = 2\n");
    else
        printf("stop bits = 1\n");
}

static void display_hardware_flow_control(struct termios* _termios)
{
    if (_termios->c_cflag & CRTSCTS)
        printf("RTS/CTS is enabled.\n");
    else
        printf("RTS/CTS is disabled.\n");
}

static void display_soft_flow_control(struct termios* _termios)
{
    if ((_termios->c_cflag & IXOFF) || (_termios->c_cflag & IXON)) {
        unsigned char stop_char = _termios->c_cc[VSTOP];
        unsigned char start_char = _termios->c_cc[VSTART];

        if(_termios->c_cflag & IXOFF)
            printf("INBOUND XON/XOFF is enabled, XON = %2x, XOFF = %2x\n", start_char, stop_char);
        else
            printf("INBOUND XON/XOFF is disabled\n");

        if (_termios->c_cflag & IXON)
            printf("OUTBOUND XON/XOFF is enabled. XON = %2x, XOFF = %2x\n", start_char, stop_char);
        else
            printf("OUTBOUND XON/XOFF is disabled\n");
    }
}

static void display_all(struct termios* _termios, char const *device_name)
{
    printf("----------------------------%s---------------------------\n", device_name);
    display_speed(_termios);
    display_data_bit(_termios);
    display_parity_bit(_termios);
    display_stop_bit(_termios);
    display_hardware_flow_control(_termios);
    display_soft_flow_control(_termios);
}

static int uart_set_port(int fd, int baud, int databits, char* parity,
        int stopbits, char* flowcontrol)
{
    struct termios new_ios, old_ios;
    if (tcgetattr(fd, &new_ios) != 0) {
        printf("Save the terminal error.\n");
        return -1;
    }

    bzero(&old_ios, sizeof(struct termios));
    old_ios = new_ios;

    tcflush(fd, TCIOFLUSH);
    new_ios.c_cflag |= CLOCAL | CREAD;
    new_ios.c_cflag &= ~CSIZE;

    if(0 != databits) {
        switch (databits)
        {
        case 5:
            new_ios.c_cflag |= CS5;
            break;
        case 6:
            new_ios.c_cflag |= CS6;
            break;
        case 7:
            new_ios.c_cflag |= CS7;
            break;
        case 8:
            new_ios.c_cflag |= CS8;
            break;
        default:
            printf("databits set invalid option -%d\n", databits);
            break;
        }
       printf("databits set - %d\n", databits);
    }

    if(0 != baud) {
        switch (baud)
         {
        case 2400:
            cfsetispeed(&new_ios, B2400);
            cfsetospeed(&new_ios, B2400);
            break;
        case 4800:
            cfsetispeed(&new_ios, B4800);
            cfsetospeed(&new_ios, B4800);
            break;
        case 9600:
            cfsetispeed(&new_ios, B9600);
            cfsetospeed(&new_ios, B9600);
            break;
        case 19200:
            cfsetispeed(&new_ios, B19200);
            cfsetospeed(&new_ios, B19200);
            break;
        case 115200:
            cfsetispeed(&new_ios, B115200);
            cfsetospeed(&new_ios, B115200);
            break;
        case 460800:
            cfsetispeed(&new_ios, B460800);
            cfsetospeed(&new_ios, B460800);
            break;
        default:
            printf("parity set invalid option -%d\n", baud);
            break;
         }
        printf("speed set - %d\n", baud);
    }

    if(NULL != parity) {
        switch (*parity)
         {
        case 'o':
        case 'O':
            new_ios.c_cflag |= PARENB;
            new_ios.c_cflag |= PARODD;
            new_ios.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':
            new_ios.c_iflag |= (INPCK | ISTRIP);
            new_ios.c_cflag |= PARENB;
            new_ios.c_cflag &= ~PARODD;
            break;
        case 'n':
        case 'N':
            new_ios.c_cflag &= ~PARENB;
            new_ios.c_iflag &= ~INPCK;
            break;
        default:
            printf("parity set invalid option -%c\n", *parity);
            break;
         }
        printf("parity set - %c\n", *parity);
    }

    if(NULL != flowcontrol) {
        switch(*flowcontrol){
        case 'y':
            new_ios.c_iflag = IXON | IXOFF | IXANY;
            break;
        case 'n':
           new_ios.c_iflag &= ~(IXON | IXOFF | IXANY);
            break;
        default:
            printf("flow control set invalid option -%c\n", *flowcontrol);
            break;
         }
        printf("flow control set - %c\n", *flowcontrol);
    }

    if (stopbits == 1)
        new_ios.c_cflag &= ~CSTOPB;
    else if (stopbits == 2)
        new_ios.c_cflag |= CSTOPB;

    /*No hardware control*/
    new_ios.c_cflag &= ~CRTSCTS;

    /*delay time set */
    new_ios.c_cc[VTIME] = 0;
    new_ios.c_cc[VMIN] = 0;

    /*raw model*/
    new_ios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    new_ios.c_oflag &= ~OPOST;

    new_ios.c_iflag &= ~(INLCR | IGNCR | ICRNL);
    new_ios.c_iflag &= ~(ONLCR | OCRNL);

    new_ios.c_oflag &= ~(INLCR | IGNCR | ICRNL);
    new_ios.c_oflag &= ~(ONLCR | OCRNL);

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &new_ios) != 0) {
        printf("Set the terminal error.\n");
        tcsetattr(fd, TCSANOW, &old_ios);
        return -1;
    }

    return 0;
}

static int do_list(int argc, char **argv)
{
    struct termios mode;
    int fd, uartnum = -1;
    int i, opt, nums = 3;

    if ((opt = getopt(argc, argv, "n:")) != -1)
        uartnum = atoi(optarg);

    if (-1 != uartnum)
        nums = 1;
    else
        uartnum = 0;

    for (i = 0; i < nums; i++) {
        const char* device_name = get_uart_name(uartnum);
        if (0 == strcmp(device_name, "ukn")) {
            printf("This is not a tty device.\n");
            continue;
        }

        fd = uart_open_port(device_name);
        if (-1 == fd)
            continue;

        if (tcgetattr(fd, &mode) != 0) {
            printf("Save the terminal error.\n");
            close(fd);

            continue;
        }

        display_all(&mode, device_name);
        uartnum++;

        close(fd);
    }

    return 0;
}

static int do_set(int argc, char **argv)
{
    int opt, err, fd;
    int uartnum = -1, baud = 0, databits = 0, stopbits = 0;
    char* parity = NULL, *flowcontrol = NULL;

    while ((opt = getopt(argc, argv, "n:b:p:d:s:f:")) != -1) {
        switch (opt)
         {
        case 'n':
            uartnum = atoi(optarg);
            break;
        case 'b':
            baud = atoi(optarg);
            break;
        case 'p':
            parity = optarg;
            break;
        case 'd':
            databits = atoi(optarg);
            break;
        case 's':
            stopbits = atoi(optarg);
            break;
        case 'f':
            flowcontrol = optarg;
            break;
        default:
            printf("utest_uart set invalid option -%c\n", opt);
            return -EINVAL;
         }
    }

    if(-1 == uartnum)
        return -EINVAL;

    const char* device_name = get_uart_name(uartnum);
    if (0 == strcmp(device_name, "ukn")) {
        printf("This is not a tty device!\n");
        return -EINVAL;
    }

    fd = uart_open_port(device_name);
    if(-1 == fd)
        return -1;

    err = uart_set_port(fd, baud, databits, parity, stopbits, flowcontrol);
    if (0 != err) {
         printf("uart_set_port error!\n");
        close(fd);
        return -EINVAL;
    }

    close(fd);
    return 0;
}

static void usage(void)
{
    printf("Usage:\n");
    printf("utest_uart list [-n uarnum]\n");
    printf("utest_uart set -n uartnum [-b baud][-p parity][-d databits][-s stopbits][-f flowcontrol]\n");
}

int main(int argc, char **argv)
{
    char *cmd;
    int rval = -EINVAL;
    printf("----------------------------utest_uart begin-----------------------\n");
    if (argc < 2) {
        usage();
        return rval;
    }

    cmd = argv[1];
    argc--;
    argv++;

    printf("utest_uart -- %s \n", cmd);
    if (strcmp(cmd, "list") == 0)
        rval = do_list(argc, argv);
    else if (strcmp(cmd, "set") == 0)
        rval = do_set(argc, argv);
    else
        usage();

    if (rval == -EINVAL)
        usage();

    printf("----------------------------utest_uart end-------------------------\n");
    return rval;
}



