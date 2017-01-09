#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#define max(a,b) ((a)>(b)?(a):(b))
#define SRV_PORT 10000
#define BLZ_DEFAULT_PORT 0

/* make sure direction BT device, we have enough buffer space */
#define SRV_SOCK_RX_BUF_SIZE    0x4000
/* to avoid rx flooding limit on BT stack side the send buffer space */
#define SRV_SOCK_TX_BUF_SIZE    0x0800

/* emulate better UART behaviour by only reading a bit more then max ACL size off 1021 */
#define XFER_CHUNK_SIZE     0x0500

struct sockaddr_in si_other_clnt;

pthread_t client_thread_id;
pthread_t server_thread_id;

int s_clnt = -1;
int bt_fd = -1;

int slen_clnt;
int slen_srv;
int s_srv = -1;
int listenfd = -1;

fd_set read_set;
fd_set active_set;

char *devname = "/dev/ttyUSB0";
int initial_baud = 115200;
char ip_dest[20];
char *reverse_conn = NULL;
int mode_raw = 0;
int default_port = SRV_PORT;
int discard_bt_tx = 0;
int dump = 1;
char *vhci_server_ip = "127.0.0.1";

#ifndef MSG_DONTWAIT
#define CYGWIN_SET_NONBLOCKING(fd) set_nb(fd, 1)
#define recv(fd, buf, len, flag)  read(fd, buf, len)
#define MSG_DONTWAIT 0x40
#else
#define CYGWIN_SET_NONBLOCKING(fd)
#endif

int bthw_client_open(char *ip_addr, int port, int baudrate);

// TODO -- hookup protocol trace parsing
static void hex_dump(char *msg, void *data, int size, int trunc)
{
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    char whitespace[100];

    if (dump == 0)
        return;
    else if (dump == 1)
    {
         printf("%s %d bytes\n", msg, size);
         return;
    }

    for (n=0;n<strlen(msg);n++)
       whitespace[n] = ' ';

    whitespace[n]=0;

    //printf("%s [%03d]  ", msg, size);

    /* truncate */
    //if(trunc && (size>32))
    //    size = 32;

    for (n = 1; n <= size; n++)
    {
        if (n % 16 == 1)
        {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x", ((unsigned int) p - (unsigned int) data));
        }

        c = *p;
        if (isalnum(c) == 0)
        {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if (n % 16 == 0)
        {
            /* line completed */
            printf("%s [%4.4s]   %-50.50s  %s\n", whitespace, addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        }
        else if (n % 8 == 0)
        {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0)
    {
        /* print rest of buffer if not empty */
        printf("%s [%4.4s]   %-50.50s  %s\n", msg, addrstr, hexstr, charstr);
    }
}

static inline int read_n(int fd, char *buf, int len)
{
    int t = 0, w;

    while (len > 0)
    {
        if ((w = read(fd, buf, len)) < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        if (!w)
            return 0;
        len -= w;
        buf += w;
        t += w;
    }
    return t;
}

static inline int write_n(int fd, char *buf, int len)
{
    int t = 0, w;

    if (fd < 0) {
        return -1;
    }
    while (len > 0)
    {
        if ((w = write(fd, buf, len)) < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                perror("write::");
                continue;
            }
            return -1;
        }
        if (!w)
        {
            perror("ERR: write_n() 000000\n");
            printf("write_n 000000 \n");
            return 0;
        }
        len -= w;
        buf += w;
        t += w;
        if (len > 0)
            printf("write_n segmentation ");
    }
    return t;
}

int wsock_connect(char *ip_str, int port)
{
    int                 sockfd, n = -1;
    struct sockaddr_in  servaddr;

    if ( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_str);
    servaddr.sin_port        = htons(port);

    while(n<0)
    {
       printf("Connecting to [%s:%d]\n",ip_str, port);

       n = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

       if (n<0)
       {
          perror("connect");
          sleep(1);
          //return -1;
       }
    }

    printf("Connected (%d)\n", sockfd);

    return sockfd;
}

int wsock_listen(int port)
{
    socklen_t           clilen;
    struct sockaddr_in  cliaddr, servaddr;
    int s, srvlen;
    int n = 1; /* rl debug */
    int size_n;
    int result = 0;

    printf("Starting listener\n");

    if (listenfd != -1)
    {
        printf("Listener already created\n");
        goto accept_incoming;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0)
    {
        printf("Listener not created: listen fd %d\n", listenfd);
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    srvlen = sizeof(servaddr);

    /* allow reuse of sock addr upon bind */
    result = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

    if (result<0)
    {
        perror("setsockopt");
    }

    /* rl start debug */
    /* RCVBUF */
    size_n = sizeof(n);
    result = getsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (void *) &n, (socklen_t *) &size_n);
    printf("listen sock %d: SO_RCVBUF: %d, res: %d, size_n: %d\n", listenfd, n, result, size_n);

    printf("setsockopt(SOL_SOCKET,SO_RCVBUF): %d\n", SRV_SOCK_RX_BUF_SIZE);
    result = setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (void *) &n,
            (socklen_t) SRV_SOCK_RX_BUF_SIZE);
    if (0 > result)
    {
        perror("setsockopt(SOL_SOCKET,SO_RCVBUF)");
    }

    /* SNDBUF */
    size_n = sizeof(n);
    result = getsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (void *) &n, (socklen_t *) &size_n);
    printf("listen sock %d: SO_SNDBUF: %d, res: %d, size_n: %d\n", listenfd, n, result, size_n);
    if (0 > result)
    {
        perror("getsockopt::SOL_SOCKET::SO_SNDBUF:");
    }

    printf("setsockopt(SOL_SOCKET,SO_SNDBUF): %d\n", SRV_SOCK_TX_BUF_SIZE);
    result = setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (void *) &n,
            (socklen_t) SRV_SOCK_TX_BUF_SIZE);
    if (0 > result)
    {
        perror("setsockopt(SOL_SOCKET,SO_SNDBUF)");
    }
    /* debug end */

    result = bind(listenfd, (struct sockaddr *) &servaddr, srvlen);

    if (result<0)
        perror("bind");

    result = listen(listenfd, 1);
    //printf("Wait accept\n");

    if (result<0)
        perror("listen");

accept_incoming:

    clilen = sizeof(struct sockaddr_in);

    s = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

    if (s<0)
    {
        perror("accept");
        return -1;
    }
    printf("Connected (%d)\n", s);

    return s;
}

///////////////////////////////////////////////////////////////////////////////////////
// TUNNEL SERVER


#ifdef NATIVE

void start_bluez(void);
void stop_bluez(void);
void btserver_main(void);

typedef enum {
    e_serial, e_bluez, e_vhci
} t_runmode;

int runmode = e_serial;

int translate_speed(int spd)
{
   speed_t speed_c = B921600;

    switch (spd)
    {

   case 2400:
      speed_c = B2400;
      break;
   case 4800:
      speed_c = B4800;
      break;
   case 9600:
      speed_c = B9600;
      break;
   case 19200:
      speed_c = B19200;
      break;
   case 38400:
      speed_c = B38400;
      break;
   case 57600:
      speed_c = B57600;
      break;
   case 115200:
      speed_c = B115200;
      break;
   case 230400:
      speed_c = B230400;
      break;
   case 460800:
      speed_c = B460800;
      break;
   case 921600:
      speed_c = B921600;
      break;
#if !defined(__CYGWIN32__) && !defined(__CYGWIN__)
   case 2000000:
       speed_c = B2000000;
       break;
   case 3000000:
       speed_c = B3000000;
       break;
   case 4000000:
       speed_c = B4000000;
       break;
#endif
   default:
      printf("Bad baudrate %d.\n", spd);
      break;
   }
   return speed_c;
}

void dev_setup(int Fd, int Speed, int Flow)
{
  struct termios t;

  if (Fd < 0)
  {
    perror("FdSetup");
    exit(1);
  }

  if (tcgetattr(Fd, &t) < 0)
  {
    perror("tcgetattr");
    exit(1);
  }

  cfmakeraw(&t);

  t.c_cflag &= ~CBAUD;
  t.c_cflag |= translate_speed(Speed) | CS8 | CLOCAL;
  t.c_oflag = 0; /* turn off output processing */
  t.c_lflag = 0; /* no local modes */

  if (Flow)
    t.c_cflag |= CRTSCTS;
  else
    t.c_cflag &= ~CRTSCTS;

  if (tcsetattr(Fd, TCSANOW, &t) < 0)
  {
    perror("FdSetup : tcsetattr");
    exit(1);
  }
  return;
}

static int open_com_port(char *DevName, int BaudRate)
{
  int Fd;

  printf("Opening dev %s at %d baud\n", DevName, BaudRate);

  if ((Fd = open(DevName, O_RDWR | O_NOCTTY)) < 0)
  {
    perror("open failed");
    exit(1);
  }

  dev_setup(Fd, BaudRate, 1); // always use flow control

  tcflush(Fd, TCIOFLUSH);

  return Fd;
}

void client_test(void)
{
    int fd = bthw_client_open(reverse_conn, 10000, 115200);
    sleep(3);
    printf("close %d\n", fd);
    close(fd);
    sleep(3);
    printf("exit\n");
    exit(0);
}

void start_serial(int baudrate)
{
    bt_fd = open_com_port(devname, baudrate);
}

void btserver_close(void)
{
    if (runmode == e_bluez)
        stop_bluez();
    else
        close(bt_fd);
}

static int set_nb(int fd, int nb)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
    {
        printf("Can't get fd flags with fcntl(): %s (%d)", strerror(errno), errno);
        return -1;
    }
    flags &= ~O_NONBLOCK;

    if (nb)
        flags |= O_NONBLOCK;

    int status = fcntl(fd, F_SETFL, flags);

    if (status < 0)
    {
        printf("Can't set socket to nonblocking mode with fcntl(): %s (%d)", strerror(errno), errno);
        return -1;
    }

    return 0;
}

void reset_device(int fd)
{
    int n_read = 0;
    char reset[4] = {1,3,0xc, 0};
    char reset_cfm[7] = {4, 0xe, 4, 1, 3, 0xc, 0};
    char rsp[32];
    int got_response = 0;

    set_nb(bt_fd, 1);

    while (!got_response)
    {
        fflush(stdout);

        printf("\nSending hci reset...");

        write(bt_fd, reset, 4);
        n_read = read(bt_fd, rsp, 32);
        if (n_read == 7)
        {
            if (memcmp(reset_cfm, rsp, 7)==0)
            {
                got_response = 1;
                continue;
            }
        }
        printf("waiting... (please reset board)...");
        fflush(stdout);
        sleep(1);
    }

    set_nb(bt_fd, 0);

    printf("got response !\n\n");
}

static void server_thread(void *p)
{
    /* make sure we can talk to the device at the initial baudrate */
    if (runmode == e_serial)
    {
        printf("Start at initial baudrate %d\n", initial_baud);
        start_serial(initial_baud);
        reset_device(bt_fd);
        close(bt_fd);
    }

    if (runmode == e_bluez)
    {
        start_bluez();
        reset_device(bt_fd);
        close(bt_fd);
    }

    if (runmode == e_vhci)
    {
        //start_vhci();
        //reset_device(bt_fd);
        //printf("closing bt fd %d\n", bt_fd);
        //close(bt_fd);
        //sleep(1);

    }

    /* main loop */
    while (1)
    {
        btserver_main();
        btserver_close();
        sleep(1);
    }
}

int bt_send(char *msg, char *p, int len)
{
    int n;
    hex_dump(msg, p, len, 1);
    n = write_n(bt_fd, p, len);
    return n;
}

int server_send(char *msg, char *p, int len)
{
    int n;
    hex_dump(msg, p, len, 1);
    n = write_n(s_srv, p, len);
    if (0 > tcflow(bt_fd, TCOON))
    {
        perror("tcflow::TCOON:");
    }
    return n;
}

int server_recv(void)
{
    char buf[XFER_CHUNK_SIZE];
    int rx_len;

    rx_len = recv(s_srv, buf, XFER_CHUNK_SIZE, MSG_DONTWAIT);

    if (rx_len == 0)
    {
        close(s_srv);
        printf("Disconnected\n");
        return -1;
    }

    if (rx_len<0)
    {
         perror("read()");
         close(s_srv);
         printf("Disconnected\n");
    }

    if (!discard_bt_tx)
       bt_send("[bt-tx] <---", buf, rx_len);
    else
        printf("discard %d bytes\n", rx_len);

    return rx_len;
}

void bt_recv(int fd)
{
    char buf[XFER_CHUNK_SIZE];
    int len=0;

    if (runmode == e_bluez)
        len = recv(fd, buf, XFER_CHUNK_SIZE, MSG_DONTWAIT);
    else
        len = read(fd, buf, XFER_CHUNK_SIZE);

    if (len == 0)
        return;

    server_send("[bt-rx] --->", buf, len);
}

#ifdef ENABLE_BLZ

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

// fixme -- doesn't work with bt usb due to kernel limitation
void setup_raw(int dev, int enable)
{
    int dd = hci_open_dev(dev);

    if (ioctl(dd, HCISETRAW, enable) < 0)
    {
        fprintf(stderr, "Can't set raw mode on hci%d: %s (%d)\n",
                    dev, strerror(errno), errno);
        hci_close_dev(dd);
        exit(1);
    }

    hci_close_dev(dd);
}

static int bt_open_socket(int dev, unsigned long flags)
{
    struct sockaddr_hci addr;
    struct hci_filter flt;

    int sk, opt;

    /* Create HCI socket */
    sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (sk < 0)
    {
        perror("Can't create raw socket");
        return -1;
    }

    opt = 1;
    if (setsockopt(sk, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0)
    {
        perror("Can't enable data direction info");
        return -1;
    }

    opt = 1;
    if (setsockopt(sk, SOL_HCI, HCI_TIME_STAMP, &opt, sizeof(opt)) < 0)
    {
        perror("Can't enable time stamp");
        return -1;
    }

    /* Setup filter */
    hci_filter_clear(&flt);

    /* only enable acl, sco and evts */
    hci_filter_set_ptype(HCI_SCODATA_PKT, &flt);
    hci_filter_all_ptypes(&flt);
    hci_filter_all_events(&flt);

    if (setsockopt(sk, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0)
    {
        perror("Can't set filter");
        return -1;
    }

    /* Bind socket to the HCI device */
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = dev;
    if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        printf("Can't attach to device hci%d. %s(%d)\n",
                    dev, strerror(errno), errno);
        return -1;
    }

    return sk;
}

void start_bluez(void)
{
    int opt;
    struct hci_dev_info di;

    printf("Starting up bluez socket\n");

    bt_fd = bt_open_socket(BLZ_DEFAULT_PORT, 0);

    if (mode_raw)
       setup_raw(BLZ_DEFAULT_PORT, 1);

    if (bt_fd < 0)
    {
        perror("Can't open device");
        return;
    }

    ioctl(bt_fd, HCIDEVUP, BLZ_DEFAULT_PORT);

    printf("HCI device opened (fd %d)\n", bt_fd);

    if (bt_fd==0)
        exit(1);

#if 1
    if (hci_devinfo(BLZ_DEFAULT_PORT, &di) < 0)
    {
        perror("Can't get device info");
        return;
    }

    opt = hci_test_bit(HCI_RAW, &di.flags);

    if (ioctl(bt_fd, HCISETRAW, opt) < 0)
    {
        if (errno == EACCES)
        {
            perror("Can't access device");
            return;
        }
    }
#endif
}

void stop_bluez(void)
{
    printf("Shutting down bluez socket\n");
    //ioctl(bt_fd, HCIDEVDOWN, BLZ_DEFAULT_PORT);
    close(bt_fd);
}

#endif

int start_vhci(void)
{
    int vhci_port = 1800;

    printf("start_vhci :: (%s:%d)\n", vhci_server_ip, vhci_port);

    bt_fd = wsock_connect(vhci_server_ip, vhci_port);

    printf("vhci started\n");

    return bt_fd;
}

void stop_vhci(void)
{

}

void btserver_main(void)
{
     int max_fd;
     int result;
     int baudrate;
     struct timeval tv;

     printf("BT HW Tunnel Starting (port %d)\n", default_port);

     if (runmode == e_bluez)
        start_bluez();

     if (runmode == e_vhci)
        start_vhci();

     if (reverse_conn)
        s_srv = wsock_connect(reverse_conn, default_port);
     else
        s_srv = wsock_listen(default_port);

     if (s_srv<0)
        exit(0);

     CYGWIN_SET_NONBLOCKING(s_srv);

     /* read baudrate */
     result = read_n(s_srv, (char *)&baudrate, 4);

     if (runmode == e_serial)
        printf("Client requested to open serial port @ %d baud\n", baudrate);

     if (result != 4)
     {
        printf("failed to read baudrate (%d), exit\n", result);
        close(s_srv);
        sleep(1);
        return;
     }

     if (runmode == e_serial)
        start_serial(baudrate);

     FD_ZERO(&read_set);
     FD_ZERO(&active_set);
     FD_SET(s_srv, &active_set);
     FD_SET(bt_fd, &active_set);

     max_fd = max(s_srv, bt_fd);

     while (1)
     {
         read_set = active_set;

         tv.tv_sec = 0;
         tv.tv_usec = 200000;

         /* wait for a connection or socket data, blocking call
                    use timeout to make sure we update read set properly */
        result = select(max_fd + 1, &read_set, NULL, NULL, NULL /*&tv */);

         //printf("result %d\n", result);

         if (result == 0)
         {
             //printf("select timeout\n");
         }
         else if (result < 0 && errno != EINTR)
         {
             perror("select");
         }
         else
         {
            /* TODO: improve placement of flow off, this simply works but degrades throughput */
            if (0 > tcflow(bt_fd, TCOOFF))
            {
                perror("tcflow::TCOOFF:");
            }
            if (FD_ISSET(s_srv, &read_set))
            {
                if (server_recv()<0)
                {
                    FD_CLR(s_srv, &active_set);
                    return;
                }
            }
            if (FD_ISSET(bt_fd, &read_set))
            {
                bt_recv(bt_fd);
            }
            /* TODO: improve placement of flow off. this simply works but degrades perf */
            if (0 > tcflow(bt_fd, TCOON))
            {
                perror("tcflow::TCOON:");
            }
        }
    }
}

void process_cmd(char *cmd)
{
    if (!strncmp(cmd, "discard", 7))
    {
        printf("discard all bt tx data...\n");
        discard_bt_tx = 1;
    }
    else if (!strncmp(cmd, "nodiscard", 9))
    {
        printf("set back to no discard...\n");
        discard_bt_tx = 0;
    }
    else if (!strncmp(cmd, "quit", 4))
    {
        exit(0);
    }
}

const char * const help_menu[] = { "-b <default baud>  : Default baudrate upon board reset",
    "-d <device>        : Set device name e.g. /dev/ttyUSB0>",
    "-p <server port>   : Set alternate tcp port",
    "-r <server addr>   : Reverse connection",
    "-R                 : Setup bluez RAW hci mode",
    "-D <mode>          : Hexdump mode (0=disabled, 1=light, 2=verbose)",
    "-z                 : Use bluez as hw interface",
                "-V                 : Connect VHCI server (hci emul)", NULL };

void dump_help(void)
{
    int i = 0;
    printf("\n");
    while (help_menu[i])
        printf("\r%s\n", help_menu[i++]);
    printf("\n\n");
}
int main(int argc, char** argv)
{
    int opt;
    int console = 0;

    runmode = e_serial;

    while ((opt = getopt(argc, argv, "b:cd:hD:i:tp:r:RzV:")) != -1)
    {
        switch(opt)
        {
            case 'b':
                initial_baud = atoi(optarg);
                printf("Initial baudrate\n");
            break;

            case 'c':
                console = 1;
                break;

            case 'd':
                devname = optarg;
                printf("Serial mode configured\n");
            break;

            case 'p':
                default_port = atoi(optarg);
                printf("Using port %d\n", default_port);
                break;

            case 'z':
                runmode = e_bluez;
                printf("Bluez mode configured\n");
                break;

            case 't':
                client_test();
                break;

            case 'r':
                reverse_conn = ip_dest;
                strncpy(reverse_conn, optarg, sizeof(ip_dest));
            break;

            case 'R':
                mode_raw = 1;
                printf("Bluez raw mode enabled\n");
                break;

            case 'D':
                dump = atoi(optarg);
                printf("Hexdump enable : %d\n", dump);
                break;

            case 'V':
                printf("VHCI Mode");
                runmode = e_vhci;
                if (optarg)
                    vhci_server_ip = optarg;
                break;

            case 'h':
                dump_help();
                exit(0);
                break;

            default:
                dump_help();
                exit(0);
                break;
        }
    }

    if (pthread_create(&server_thread_id, NULL, (void*) server_thread, (void*) s_srv) != 0)
         perror("pthread_create");

    sleep(3);

    while(console)
    {
        char line[128];

        // command prompt
        printf( ">" );

        fflush(stdout);

        fgets ( line, 128, stdin );

        //add_history(line);

        // write for each new command
        //write_history(HISTORY_FILE);

        if (line[0]!= '\0')
        {
            process_cmd(line);
            memset(line, '\0', 128);
        }
    }

    /* wait for thread to terminate */
    pthread_join(server_thread_id, NULL);
    return 0;
}

#endif // NATIVE

///////////////////////////////////////////////////////////////////////////////////////
// CLIENT

// if using 0.0.0.0 as ip address we set it up as passive listener
int bthw_client_open(char *ip_addr, int port, int baudrate)
{
    int s;
    printf("bthw_client_open :: (%s:%d) @ %d baud\n", ip_addr, port, baudrate);

    if (strcmp(ip_addr, "0.0.0.0") == 0)
       s = wsock_listen(port);
    else
       s = wsock_connect(ip_addr, port);

    /* send baudrate in first 4 bytes */
    write_n(s, (char *)&baudrate, 4);

    s_clnt = s;

    return s;
}

#if 0
int  bthw_client_send(char *p, int len)
{
    int tx_len;

    hex_dump("tx <--", p, len, 0);

    tx_len = write_n(s_clnt, p, len);

    if (tx_len==-1)
      perror("sendto()");

   if (len!=tx_len)
      printf("client sent %d bytes out of %d\n", tx_len, len);

   return tx_len;
}

int bthw_client_recv(char *p, int len)
{
    int rx_len;

    rx_len = read(s_clnt, p, len);//, MSG_DONTWAIT);

    hex_dump("rx -->", p, rx_len, 0);

    if (rx_len==-1)
      perror("recv");

    if (rx_len == 0)
    {
        printf("Disconnected tcp link\n");
        return -1;
    }

   return rx_len;
}

int bthw_client_get_fd(void)
{
    return s_clnt;
}

#endif

