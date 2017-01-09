#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "engopt.h"
#include "eng_pcclient.h"
#include "eng_cmd4linuxhdlr.h"

static eng_dev_info_t* s_dev_info;
static int at_mux_fd = -1;
static int pc_fd=-1;

#ifdef CONFIG_BQBTEST
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <cutils/str_parms.h>
#include <cutils/sockets.h>

#define BQB_CTRL_PATH "/data/misc/.bqb_ctrl"
#define ROUTE_BQB   0
#define ROUTE_AT    1
#define ENABLE_BQB_TEST "AT+SPBQBTEST=1"
#define DISABLE_BQB_TEST "AT+SPBQBTEST=0"
#define TRIGGER_BQB_TEST "AT+SPBQBTEST=?"
#define NOTIFY_BQB_ENABLE "\r\n+SPBQBTEST OK: ENABLED\r\n"
#define NOTIFY_BQB_DISABLE "\r\n+SPBQBTEST OK: DISABLE\r\n"
#define TRIGGER_BQB_ENABLE "\r\n+SPBQBTEST OK: BQB\r\n"
#define TRIGGER_BQB_DISABLE "\r\n+SPBQBTEST OK: AT\r\n"
#define UNKNOW_COMMAND "\r\n+SPBQBTEST ERROR: UNKNOW COMMAND\r\n"

static current_route = ROUTE_AT;

static inline int create_server_socket(const char* name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s < 0) {
		ENG_LOG("socket(AF_LOCAL, SOCK_STREAM, 0) failed \n");
		return -1;
	}
    ENG_LOG("bqb covert name to android abstract name:%s", name);
    if (socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0) {
        if (listen(s, 5) == 0) {
            ENG_LOG("bqb listen to local socket:%s, fd:%d", name, s);
            return s;
        } else {
            ENG_LOG("bqb listen to local socket:%s, fd:%d failed, errno:%d", name, s, errno);
        }
    } else {
        ENG_LOG("bqb create local socket:%s fd:%d, failed, errno:%d", name, s, errno);
    }
    close(s);
    return -1;
}

static int accept_server_socket(int sfd)
{
    struct sockaddr_un remote;
    struct pollfd pfd;
    int fd;
    socklen_t len = sizeof(struct sockaddr_un);

    //ENG_LOG("accept fd %d", sfd);

    /* make sure there is data to process */
    pfd.fd = sfd;
    pfd.events = POLLIN;

    if (poll(&pfd, 1, 0) == 0) {
        ENG_LOG("accept poll timeout");
        return -1;
    }

    //ENG_LOG("poll revents 0x%x", pfd.revents);

    if ((fd = accept(sfd, (struct sockaddr *)&remote, &len)) == -1) {
         ENG_LOG("sock accept failed (%s)", strerror(errno));
         return -1;
    }

    //ENG_LOG("new fd %d", fd);

    return fd;
}

int eng_controller2tester(char * controller_buf, unsigned int data_len)
{
    int len = 0;
    len = write(pc_fd,controller_buf,data_len);
    ENG_LOG("bqb test eng_controller2tester %d", len);
    return len;
}

static void bqb_service_enable(int pc_fd) {
	struct termios ser_settings;
    int ret = 0;
    ENG_LOG("bqb bqb_service_enable");
    tcgetattr(pc_fd, &ser_settings);
    cfmakeraw(&ser_settings);
    ser_settings.c_lflag = 0;
    tcsetattr(pc_fd, TCSANOW, &ser_settings);
    ret = eng_controller_bqb_start();
}

static void bqb_service_disable(int pc_fd) {
	struct termios ser_settings;
    int ret = 0;
    ENG_LOG("bqb bqb_service_disable");
    tcgetattr(pc_fd, &ser_settings);
    cfmakeraw(&ser_settings);
    ser_settings.c_lflag |= (ECHO | ECHONL);
    ser_settings.c_lflag &= ~ECHOCTL;
    tcsetattr(pc_fd, TCSANOW, &ser_settings);
    ret = eng_controller_bqb_stop();
}
#endif

static int start_gser(char* ser_path)
{
    struct termios ser_settings;

    if (pc_fd>=0){
        ENG_LOG("%s ERROR : %s\n", __FUNCTION__, strerror(errno));
        close(pc_fd);
    }

    ENG_LOG("open serial\n");
    pc_fd = open(ser_path,O_RDWR);
    if(pc_fd < 0) {
        ENG_LOG("cannot open vendor serial\n");
        return -1;
    }

    tcgetattr(pc_fd, &ser_settings);
    cfmakeraw(&ser_settings);

#ifdef CONFIG_BQBTEST
    if (current_route == ROUTE_BQB) {
        ser_settings.c_lflag = 0;
    } else {
        ser_settings.c_lflag |= (ECHO | ECHONL);
        ser_settings.c_lflag &= ~ECHOCTL;
    }
#else
    ser_settings.c_lflag |= (ECHO | ECHONL);
    ser_settings.c_lflag &= ~ECHOCTL;
#endif

    tcsetattr(pc_fd, TCSANOW, &ser_settings);

    return 0;
}


static void *eng_readpcat_thread(void *par)
{
    int len;
    int written;
    int cur;
    char engbuf[ENG_BUFFER_SIZE];
    char databuf[ENG_BUFFER_SIZE];
    int i, offset_read, length_read, status;
    eng_dev_info_t* dev_info = (eng_dev_info_t*)par;
#ifdef CONFIG_BQBTEST
    int bs_fd, ret;
    int max_fd = pc_fd;
    fd_set read_set;

    /* init bqb server socket */
    bs_fd = create_server_socket(BQB_CTRL_PATH);

    if (bs_fd < 0) {
        ENG_LOG("creat bqb server socket error(%s)", strerror(errno));
    } else {
        max_fd = bs_fd > pc_fd ? bs_fd : pc_fd;
    }

    for (;;) {
        ENG_LOG("wait for command / byte stream");
        FD_ZERO(&read_set);
        if (bs_fd > 0) {
            FD_SET(bs_fd, &read_set);
        }

        if (pc_fd > 0) {
            FD_SET(pc_fd, &read_set);
        } else {
            sleep(1);
            start_gser(dev_info->host_int.dev_at);
            continue;
        }

        ret = select(max_fd+1, &read_set, NULL, NULL, NULL);
        if (ret == 0) {
            ENG_LOG("select timeout");
            continue;
        } else if (ret < 0) {
            ENG_LOG("select failed %s", strerror(errno));
            continue;
        }
        if (FD_ISSET(bs_fd, &read_set)) {
            ENG_LOG("bs_fd got");
            int fd = accept_server_socket(bs_fd);
            if (fd < 0) {
                ENG_LOG("bqb get service socket fail");
                sleep(1);
                continue;
            }
            memset(engbuf, 0, ENG_BUFFER_SIZE);
            len = read(fd, engbuf, ENG_BUFFER_SIZE);
            ENG_LOG("bqb control: %s: len: %d", engbuf, len);
            if (strstr(engbuf, ENABLE_BQB_TEST)) {
                if (current_route == ROUTE_AT) {
                    bqb_service_enable(pc_fd);
                    write(fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
                    current_route = ROUTE_BQB;
                } else if (current_route == ROUTE_BQB) {
                    write(fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
                }
            } else if (strstr(engbuf, DISABLE_BQB_TEST)) {
                if (current_route == ROUTE_BQB) {
                    bqb_service_disable(pc_fd);
                    write(fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
                    current_route = ROUTE_AT;
                } else if (current_route == ROUTE_AT) {
                    write(fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
                }
            } else if (strstr(engbuf, TRIGGER_BQB_TEST)) {
                if (current_route == ROUTE_BQB) {
                    write(fd, TRIGGER_BQB_ENABLE, strlen(TRIGGER_BQB_ENABLE));
                } else if (current_route == ROUTE_AT) {
                    write(fd, TRIGGER_BQB_DISABLE, strlen(TRIGGER_BQB_DISABLE));
                }
            } else {
                    write(fd, UNKNOW_COMMAND, strlen(UNKNOW_COMMAND));
            }
            close(fd);
            continue;
        } else if (FD_ISSET(pc_fd, &read_set)) {
            memset(engbuf, 0, ENG_BUFFER_SIZE);
            len = read(pc_fd, engbuf, ENG_BUFFER_SIZE);
            if (len <= 0) {
                ENG_LOG("%s: read pc_fd buffer error %s",__FUNCTION__,strerror(errno));
                sleep(1);
                start_gser(dev_info->host_int.dev_at);
                continue;
            }

            ENG_LOG("pc got: %s: %d", engbuf, len);
            if (strstr(engbuf, ENABLE_BQB_TEST)) {
                if (current_route == ROUTE_AT) {
                    bqb_service_enable(pc_fd);
                    write(pc_fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
                    current_route = ROUTE_BQB;
                } else if (current_route == ROUTE_BQB) {
                    write(pc_fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
                }
                continue;
            } else if (strstr(engbuf, DISABLE_BQB_TEST)) {
                if (current_route == ROUTE_BQB) {
                    bqb_service_disable(pc_fd);
                    write(pc_fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
                    current_route = ROUTE_AT;
                } else if (current_route == ROUTE_AT) {
                    write(pc_fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
                }
                continue;
            } else if (strstr(engbuf, TRIGGER_BQB_TEST)) {
                if (current_route == ROUTE_BQB) {
                    write(pc_fd, TRIGGER_BQB_ENABLE, strlen(TRIGGER_BQB_ENABLE));
                } else if (current_route == ROUTE_AT) {
                    write(pc_fd, TRIGGER_BQB_DISABLE, strlen(TRIGGER_BQB_DISABLE));
                }
            }
        } else {
            ENG_LOG("warning !!!");
        }

        if (current_route == ROUTE_BQB) {
            eng_send_data(engbuf, len);
        }

        if (current_route == ROUTE_AT) {
            if(at_mux_fd >= 0) {
                cur = 0;
                while(cur < len) {
                    do {
                        written = write(at_mux_fd, engbuf + cur, len -cur);
                        ENG_LOG("muxfd=%d, written=%d\n", at_mux_fd, written);
                    }while(written < 0 && errno == EINTR);

                    if(written < 0) {
                        ENG_LOG("%s: write length error %s\n", __FUNCTION__, strerror(errno));
                        break;
                    }
                    cur += written;
                }
            }else {
                ENG_LOG("muxfd fail?");
            }
        }
    }
#else
    for(;;){
        ENG_LOG("%s: wait pcfd=%d\n",__func__,pc_fd);

read_again:
        memset(engbuf, 0, ENG_BUFFER_SIZE);
        if (pc_fd >= 0){
            len = read(pc_fd, engbuf, ENG_BUFFER_SIZE);
            ENG_LOG("%s: wait pcfd=%d buf=%s len=%d",__func__,pc_fd,engbuf,len);

            if (len <= 0) {
                ENG_LOG("%s: read length error %s",__FUNCTION__,strerror(errno));
                sleep(1);
                start_gser(dev_info->host_int.dev_at);
                goto read_again;
            }else{
                ENG_LOG("bqb write to at_mux_fd...., engbuf: %s", engbuf);
                // Just send to modem transparently.
                if(at_mux_fd >= 0) {
                    cur = 0;
                    while(cur < len) {
                        do {
                            written = write(at_mux_fd, engbuf + cur, len -cur);
                            ENG_LOG("muxfd=%d, written=%d\n", at_mux_fd, written);
                        }while(written < 0 && errno == EINTR);

                        if(written < 0) {
                            ENG_LOG("%s: write length error %s\n", __FUNCTION__, strerror(errno));
                            break;
                        }
                        cur += written;
                    }
                }else {
                    ENG_LOG("muxfd fail?");
                }
            }
        }else{
            sleep(1);
            start_gser(dev_info->host_int.dev_at);
        }
    }
#endif
    return NULL;
}

static void *eng_readmodemat_thread(void *par)
{
    int ret;
    int len;
    char engbuf[ENG_BUFFER_SIZE];
    eng_dev_info_t* dev_info = (eng_dev_info_t*)par;

    for(;;){
        ENG_LOG("%s: wait pcfd=%d\n",__func__,pc_fd);
        memset(engbuf, 0, ENG_BUFFER_SIZE);
        len = read(at_mux_fd, engbuf, ENG_BUFFER_SIZE);
        ENG_LOG("muxfd =%d buf=%s,len=%d\n",at_mux_fd,engbuf,len);
        if (len <= 0) {
            ENG_LOG("%s: read length error %s\n",__FUNCTION__,strerror(errno));
            sleep(1);
            continue;
        }else{
write_again:
            if (pc_fd>=0){
                ret = write(pc_fd,engbuf,len);
                if (ret <= 0) {
                    ENG_LOG("%s: write length error %s\n",__FUNCTION__,strerror(errno));
                    sleep(1);
                    start_gser(dev_info->host_int.dev_at);
                    goto write_again;
                }
            }else{
                sleep(1);
            }
        }
    }
    return NULL;
}

int eng_at_pcmodem(eng_dev_info_t* dev_info)
{
    eng_thread_t t1,t2;

    ENG_LOG("%s ",__func__);

    start_gser(dev_info->host_int.dev_at);

    at_mux_fd = open(dev_info->modem_int.at_chan, O_RDWR);
    if(at_mux_fd < 0){
        ENG_LOG("%s: open %s fail [%s]\n",__FUNCTION__, dev_info->modem_int.at_chan,strerror(errno));
        return -1;
    }

    if (0 != eng_thread_create( &t1, eng_readpcat_thread, (void*)dev_info)){
        ENG_LOG("read pcat thread start error");
    }

    if (0 != eng_thread_create( &t2, eng_readmodemat_thread, (void*)dev_info)){
        ENG_LOG("read modemat thread start error");
    }
    return 0;
}
