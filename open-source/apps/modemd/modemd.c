#define LOG_TAG     "MODEMD"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>
#include "modemd.h"
#include <sys/time.h>
#include <fcntl.h>
#include "modemd_ext.h"

#define  SOCKET_NAME_MODEM_CTL  "control_modem"

int  client_fd[MAX_CLIENT_NUM];
static char ttydev[256];
int  engpc_client_fd[MAX_CLIENT_NUM] ;// engpc_client that notify modemd start/stop  engpc_server
static fd_set engpcFds;//engcontrol_listen thread listen(select) engpc_fd
static int nengfds = 0; //max engpc_fd ;

 int notifypipe[2] = {-1}; //for engcontrol_thread nofity engcontrol_listen that some engpc_cil connect

static int ttydev_fd;

/*
 * Returns 1 if found, 0 otherwise. needle must be null-terminated.
 * strstr might not work because WebBox sends garbage before the first OK read
 *
 */
int findInBuf(unsigned char *buf, int len, char *needle)
{
    int i;
    int needleMatchedPos = 0;

    if (needle[0] == '\0') {
        return 1;
    }

    for (i = 0; i < len; i++) {
        if (needle[needleMatchedPos] == buf[i]) {
            needleMatchedPos++;
            if (needle[needleMatchedPos] == '\0') {
                /* Entire needle was found */
                return 1;
            }
        } else {
            needleMatchedPos = 0;
        }
    }
    return 0;
}

/* helper function to get pid from process name */
static int get_task_pid(char *name)
{
    DIR *d;
    struct dirent *de;
    char cmdline[1024];

    d = opendir("/proc");
    if (d == 0) return -1;

    while ((de = readdir(d)) != 0) {
        if(isdigit(de->d_name[0])) {
            int pid = atoi(de->d_name);
            int fd, ret;
            sprintf(cmdline, "/proc/%d/cmdline", pid);
            fd = open(cmdline, O_RDONLY);
            if (fd < 0) continue;
            ret = read(fd, cmdline, sizeof(cmdline)-1);
            close(fd);
            if (ret < 0) ret = 0;
            cmdline[ret] = 0;
            if (strcmp(name, cmdline) == 0) {
                closedir(d);
                return pid;
            }
        }
    }
    closedir(d);
    return -1;
}

static int stop_nvitemd(char * modem)
{
    char cp_diskserver[20] = { 0 };
    snprintf(cp_diskserver, sizeof(cp_diskserver), "cp_diskserver_%s", modem);
    MODEMD_LOGD("stop %s cp_diskserver!", modem);
    property_set("ctl.stop", cp_diskserver);
    return 0;
}

static int start_nvitemd(char * modem)
{
    char cp_diskserver[20] = { 0 };
    snprintf(cp_diskserver, sizeof(cp_diskserver), "cp_diskserver_%s", modem);
    MODEMD_LOGD("start %s cp_diskserver!", modem);
    property_set("ctl.start", cp_diskserver);
    return 0;
}

static int stop_engservice(char * modem)
{
    char engpcclient[20] = {0};
    if (strcmp(modem, "l")==0) {
        snprintf(engpcclient, sizeof(engpcclient), "engpcclient%ste", modem);
    } else {
        snprintf(engpcclient, sizeof(engpcclient), "engpcclient%s", modem);
    }
    MODEMD_LOGD("stop engservice!");

    property_set("ctl.stop", engpcclient);

    return 0;
}

static int eng_parse_cmdline(void){
    int ret = 0;
    int cmdline_fd = open("/proc/cmdline", O_RDONLY);
    MODEMD_LOGD("eng_parse_cmdline fd = %d,error = %s", cmdline_fd,strerror(errno));
    if (cmdline_fd >=0 ) {
        char *p;
        char cmd[1024] = {0};
        read(cmdline_fd, cmd, sizeof(cmd) - 1);
        MODEMD_LOGD("cmdline:%s", cmd);
        p = strstr(cmd, "autotest=");
        if (p!=NULL) {
            p = p+9;
            if('1' == *p)
                ret = 1;
        }
        close(cmdline_fd);
    }
    MODEMD_LOGD("eng_parse_cmdline ret =%d", ret);
    return ret;
}

static int start_engservice(char * modem)
{
    char prop[PROPERTY_VALUE_MAX];
    MODEMD_LOGD("start engservice!");
    property_get(MODEM_ENGCTRL_PRO,prop, "0");
    if(!strcmp(prop, "1") && !eng_parse_cmdline() ) {
       MODEMD_LOGD("persist.engpc.disable is true return  ");
       return 0;
    }

    char engpcclient[20] = { 0 };
    if (strcmp(modem, "l")==0) {
        snprintf(engpcclient, sizeof(engpcclient), "engpcclient%ste", modem);
    } else {
        snprintf(engpcclient, sizeof(engpcclient), "engpcclient%s", modem);
    }
    property_set("ctl.start", engpcclient);

    return 0;
}

static int stop_phser(char * modem)
{
    MODEMD_LOGD("stop %s phoneserver!", modem);
    property_set("ctl.stop", "phoneserver");
    return 0;
}

static int start_phser(char * modem)
{
    MODEMD_LOGD("start %s phoneserver!", modem);
    property_set("ctl.start", "phoneserver");
    return 0;
}

static int stop_rild(char * modem)
{
    char phoneCount[PROPERTY_VALUE_MAX] = {0};

    snprintf(SP_SIM_NUM_PROP, sizeof(SP_SIM_NUM_PROP), "ro.modem.%s.count", modem);
    property_get(SP_SIM_NUM_PROP, phoneCount, "");
    /* stop rild */
    MODEMD_LOGD("stop %s rild!", modem);
    property_set("ctl.stop", "spril-daemon");
    property_get(SP_SIM_NUM_PROP, phoneCount, "");
    if (!strcmp(phoneCount, "2")) {
        property_set("ctl.stop", "spril-daemon1");
    } else if(!strcmp(phoneCount, "3")) {
        property_set("ctl.stop", "spril-daemon1");
        property_set("ctl.stop", "spril-daemon2");
    }

    return 0;
}

static int start_rild(char * modem)
{
    char phoneCount[PROPERTY_VALUE_MAX] = {0};

    snprintf(SP_SIM_NUM_PROP, sizeof(SP_SIM_NUM_PROP), "ro.modem.%s.count", modem);
    property_get(SP_SIM_NUM_PROP, phoneCount, "");

    /* start rild */
    MODEMD_LOGD("start %s rild!", modem);
    property_set("ctl.start", "spril-daemon");
    if (!strcmp(phoneCount, "2")) {
        property_set("ctl.start", "spril-daemon1");
    } else if (!strcmp(phoneCount, "3")) {
        property_set("ctl.start", "spril-daemon1");
        property_set("ctl.start", "spril-daemon2");
    }

    return 0;
}

int stop_service(char * modem, int is_vlx)
{
    pid_t pid;
    char pid_str[32] = {0};
    char prop[PROPERTY_VALUE_MAX] = "";

    property_get(MODEM_RESET_PROP, prop, "0");
    MODEMD_LOGD("enter stop_service!");

    /* stop eng */
    if(atoi(prop))
        stop_engservice(modem);

    /* stop phoneserver */
    stop_phser(modem);

    /* close ttydev */
    if (is_vlx == 1) {
        if (ttydev_fd >= 0)
            close(ttydev_fd);
    }

    /* stop rild */
    stop_rild(modem);

    stop_nvitemd(modem);

    /* restart com.android.phone */
    pid = get_task_pid(PHONE_APP);
    if (pid > 0) {
        MODEMD_LOGD("restart %s (%d)!", PHONE_APP, pid);
        snprintf(pid_str, sizeof(pid_str), "%d", pid);
        property_set(PHONE_APP_PROP, pid_str);
    }
    MODEMD_LOGD("kill %s phone process", modem);
    property_set("ctl.start", "kill_phone");

    return 0;
}

int send_atcmd(int stty_fd, char *at_str, char *path)
{
    int ret = -1;
    int count = 0, length = 0;
    char buffer[256] = {0};
    fd_set rfds;
    struct timeval timeout;

    MODEMD_LOGD("write %s to %s", at_str, path);

    length = strlen(at_str);

    for (;;) {
        ret = write(stty_fd, at_str, length);
        if (ret != length) {
            MODEMD_LOGE("write error length = %d  ret = %d\n", length, ret);
            close(stty_fd);
            exit_modemd();
        }

WAIT_RESPOSE_DATA:
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        FD_ZERO(&rfds);
        FD_SET(stty_fd, &rfds);

        ret = select(stty_fd + 1, &rfds, NULL, NULL, &timeout);
        if (ret < 0) {
            MODEMD_LOGE("select error: %s", strerror(errno));
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else {
                close(stty_fd);
                exit_modemd();
            }
        } else if (ret == 0) {
            MODEMD_LOGE("select timeout");
            continue;
        } else {
            memset(buffer, 0, sizeof(buffer));
            count = read(stty_fd, buffer, sizeof(buffer)-1);
            if (count <= 0) {
                MODEMD_LOGE("read %d return %d, error: %s", stty_fd, count, strerror(errno));
                continue;
            }
            MODEMD_LOGD("read response %s", buffer);
            if (strstr(buffer, "OK")) {
                ret = 1;
                break;
            } else if (strstr(buffer, "ERROR")) {
                MODEMD_LOGD("wrong modem state, exit!");
                close(stty_fd);
                exit_modemd();
            }
            else
            {
                //go on wait
                MODEMD_LOGD("do nothing, go on wait!");
                goto WAIT_RESPOSE_DATA;
            }
        }
    }

    return ret;
}

/******************/
/* Read imei info */
/******************/
static int read_imei(int index, char *imeiPtr)
{
    int ret = 0;
    int fd = -1;
    char imei_path[100] = {0};

    MODEMD_LOGD("%s: imei index: %d\n", __FUNCTION__, index);

    switch(index){
        case 1:
            strcpy(imei_path, IMEI1_CONFIG_FILE);
            break;
        case 2:
            strcpy(imei_path, IMEI2_CONFIG_FILE);
            break;
        case 3:
            strcpy(imei_path, IMEI3_CONFIG_FILE);
            break;
        case 4:
            strcpy(imei_path, IMEI4_CONFIG_FILE);
            break;
        default:
            return 0;
    }

    fd = open(imei_path, O_RDONLY);
    if (fd >= 0) {
        ret = read(fd, imeiPtr, MAX_IMEI_STR_LENGTH);
        if (ret > 0) {
            MODEMD_LOGD("%s: read imei succ : %s\n", __FUNCTION__, imeiPtr);
            ret = 1;
        } else {
            MODEMD_LOGD("%s: read imei fail\n", __FUNCTION__);
            ret = 0;
        }
        close(fd);
    } else {
        MODEMD_LOGD("%s: open imei file failed\n", __FUNCTION__);
    }

    return ret;
}

/*******************/
/* Send imei to CP */
/*******************/
void send_imei(int stty_fd, char *path)
{
    int i = 0, ret = 0;
    char imeistr[MAX_IMEI_STR_LENGTH+1] = {0};
    char at_str[64] = {0};

    /* Send IMEI */
    for (i = 1; i < IMEI_NUM + 1; i ++) {
        memset(imeistr, 0, sizeof(imeistr));
        ret = read_imei(i, imeistr);
        if (ret <= 0) continue;
        memset(at_str, 0, sizeof(at_str));
        sprintf(at_str, "AT+SPIMEI=%d,\"%s\"\r", i-1, imeistr);
        if (send_atcmd(stty_fd, at_str, path) != 1) {
            close(stty_fd);
            exit_modemd();
        }
    }
}

int open_modem_dev(char *path)
{
    int  fd = -1;

retry:
    fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        MODEMD_LOGE("Failed to open %s error: %s!\n", path, strerror(errno));
        if (errno == EINTR || errno == EAGAIN)
            goto retry;
        else
            return -1;
    }
    return fd;
}

int start_service(char* modem, int is_vlx, int restart)
{
    char mux_2sim_swap[]="echo 1 > /proc/mux_mode";
    char mux_3sim_swap[]="echo 2 > /proc/mux_mode";
    char phoneCount[PROPERTY_VALUE_MAX]="";
    char path[256] = {0};
    int  stty_fd;
    char modem_dev[PROPERTY_VALUE_MAX];
    char at_str[64] = {0};

    snprintf(SP_SIM_NUM_PROP, sizeof(SP_SIM_NUM_PROP), "ro.modem.%s.count", modem);
    snprintf(SP_TTY_DEV_PROP, sizeof(SP_TTY_DEV_PROP), "ro.modem.%s.tty", modem);

    MODEMD_LOGD("enter start_service(ModemType=%s)!", modem);

    /* open vuart/ttyNK for vlx modem */
    if(is_vlx == 1) {
        property_get(TTY_DEV_PROP, ttydev, "ttyNK3");
        sprintf(path, "/dev/%s", ttydev);
        MODEMD_LOGD("open tty dev: %s", path);
        ttydev_fd = open(path, O_RDWR);
        if (ttydev_fd < 0)
            MODEMD_LOGE("Failed to open %s!\n", path);
        if (strcmp(modem,"t")==0 || strcmp(modem,"w")==0) {
            property_get(SP_SIM_NUM_PROP, phoneCount, "");
        }
        if(!strcmp(phoneCount, "2"))
            system(mux_2sim_swap);
        else if(!strcmp(phoneCount, "3"))
            system(mux_3sim_swap);
    } else {
        property_get(SP_TTY_DEV_PROP, modem_dev, "");
        property_get(SP_SIM_NUM_PROP, phoneCount, "");

        /* Open modem dev to send at cmd */
        sprintf(path, "%s0", modem_dev);
        MODEMD_LOGD("open stty dev: %s", path);
        stty_fd = open_modem_dev(path);
        if (stty_fd < 0) return -1;

        if(!strcmp(phoneCount, "2"))
            strcpy(at_str, "AT+SMMSWAP=0\r");
        else if(!strcmp(phoneCount, "3"))
            strcpy(at_str, "AT+SMMSWAP=1\r");
        else
            strcpy(at_str, "AT\r");
        if (send_atcmd(stty_fd, at_str, path) != 1) {
            close(stty_fd);
            exit_modemd();
        }

        strcpy(at_str, "AT+CMUX=0\r");
        if (send_atcmd(stty_fd, at_str, path) != 1) {
            close(stty_fd);
            exit_modemd();
        }
        /* Send IMEI */
        send_imei(stty_fd, path);
        close(stty_fd);
    }

    /*start phoneserver*/
    start_phser(modem);

    /*start eng*/
    start_engservice(modem);

    if(restart == 1) {
        MODEMD_LOGD("restart rild!");
        start_rild(modem);

        MODEMD_LOGD("restart mediaserver!");
        //property_set("ctl.restart", "media");
    } else {
        MODEMD_LOGD("start rild!");
        start_rild(modem);
    }

    start_nvitemd(modem);

    return 0;
}

static void *modemd_listenaccept_thread()
{
    int sfd, n, i;

    for(i=0; i<MAX_CLIENT_NUM; i++)
        client_fd[i]=-1;

    sfd = socket_local_server(MODEM_SOCKET_NAME,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (sfd < 0) {
        MODEMD_LOGE("%s: cannot create local socket server", __FUNCTION__);
        exit_modemd();
    }

    for(;;){

        MODEMD_LOGD("%s: Waiting for new connect ...", __FUNCTION__);
        if ( (n=accept(sfd,NULL,NULL)) == -1)
        {
            MODEMD_LOGE("engserver accept error\n");
            continue;
        }

        MODEMD_LOGD("%s: accept client n=%d",__FUNCTION__, n);
        for(i=0; i<MAX_CLIENT_NUM; i++) {
            if(client_fd[i]==-1){
                client_fd[i]=n;
                MODEMD_LOGD("%s: fill %d to client[%d]\n",__FUNCTION__, n, i);
                break;
            }
            /* if client_fd arrray is full, just fill the new socket to the
             * last element */
            if(i == MAX_CLIENT_NUM - 1) {
                MODEMD_LOGD("%s: client array is full, just fill %d to client[%d]",
                        __FUNCTION__, n, i);
                client_fd[i]=n;
            }
        }
    }
    return NULL;
}

static void control_engservice(int  open)
{
      MODEMD_LOGD("%s :%d",__FUNCTION__,open);
      char prop[265] = {'\0'};
      if(0== open) {
          MODEMD_LOGD("persist.engpc.disable true");
          memset(prop,'\0',256);
          property_get("ro.radio.modemtype",prop, "");

          stop_engservice(prop);

      } else {
          MODEMD_LOGD("persist.engpc.disable false");
          memset(prop,'\0',256);
          property_get("ro.radio.modemtype",prop, "");

          start_engservice(prop);

      }
}


static void *modemd_engcontrol_listen()
{
    int n;
    char controlinfo[32] = { '\0' };
    int controlinfolen = strlen(MODEM_ENGCTRL_PRO) + 1;
    int countRead = -1;
    int readnum = -1;
    int i = 0;
    char prop[256];
    struct timeval tv;
    MODEMD_LOGD("%s:  enter", __FUNCTION__);
    for(;;)
    {
           FD_ZERO(&engpcFds);
           nengfds = notifypipe[0] + 1;
           FD_SET(notifypipe[0], &engpcFds);
        for(i=0; i<MAX_CLIENT_NUM; i++) {
                  if(engpc_client_fd[i]!=-1){
                      FD_SET(engpc_client_fd[i], &engpcFds);
                      if (engpc_client_fd[i] >= nengfds) nengfds = engpc_client_fd[i]+1;
                  }
        }
           tv.tv_sec = 0;
              tv.tv_usec = 1000000ll;
        MODEMD_LOGD("%s:begin select ",__FUNCTION__);
        n = select(nengfds, &engpcFds, NULL, NULL, NULL);
         MODEMD_LOGD("%s:after  select n= %d",__FUNCTION__,n);
         if(n > 0 && (FD_ISSET(notifypipe[0], &engpcFds)))
         {
                         int  len = 0;
                         char buf[128];
                         memset(buf, 0, sizeof(buf));
                         len = read(notifypipe[0], buf, sizeof(buf)-1);
                MODEMD_LOGD("%s:a engcli connect to modemd  n =%d",__FUNCTION__,len);
               }
        for (i = 0; (i < MAX_CLIENT_NUM) && (n > 0); i++) {
            int  nfd=engpc_client_fd[i];
            if (nfd != -1 && FD_ISSET(nfd, &engpcFds)) {
                n--;
                countRead = 0 ;
                do{
                    MODEMD_LOGD("%s:begin   read ",__FUNCTION__);
                    memset(controlinfo, 0, sizeof(controlinfo));
                    readnum =  read (nfd,controlinfo,controlinfolen);
                    MODEMD_LOGD("%s:after    read %d",__FUNCTION__,readnum);
                    if(readnum > 0)
                    {
                        countRead += readnum ;
                    }
                    if(countRead ==controlinfolen ||readnum <= 0 )
                    {
                                          if(readnum <= 0)
                                          {
                                              close(engpc_client_fd[i]) ;
                                              engpc_client_fd[i] = -1 ;
                                          }
                           property_get(MODEM_ENGCTRL_PRO,prop, "");
                           if(!strcmp(prop,ENGPC_REQUSETY_CLOSE))
                        {
                            control_engservice(0);
                        }
                        else if(!strcmp(prop,ENGPC_REQUSETY_OPEN))
                        {
                                control_engservice(1);
                        }
                        else
                        {
                             MODEMD_LOGD("%s:error not prop set%s ",__FUNCTION__, prop);
                        }
                        memset(prop,'\0',256);
                        break;
                    }
                }while(1) ;
            }
        }
    }
    close(notifypipe[0]);
    exit_modemd();
    return NULL;
}

static void *modemd_engcontrol_thread()
{
    int sfd, n, i;
    pthread_t tid;
    for(i=0; i<MAX_CLIENT_NUM; i++)
        engpc_client_fd[i]=-1;
      if(pipe(notifypipe) < 0)
      {
          MODEMD_LOGD("pipe error!\n");
      }
      FD_ZERO(&engpcFds);
      sfd = socket_local_server(MODEM_ENGCTRL_NAME,
      ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
      if (sfd < 0) {
          MODEMD_LOGE("%s: cannot create local socket server", __FUNCTION__);
          exit_modemd();
      }
    pthread_create(&tid, NULL, (void*)modemd_engcontrol_listen, NULL);
    for(; ;){
        MODEMD_LOGD("%s: Waiting for new connect ...", __FUNCTION__);
        if ( (n=accept(sfd,NULL,NULL)) == -1)
        {
            MODEMD_LOGE("%s engserver accept error\n", __FUNCTION__);
            continue;
        }
        MODEMD_LOGD("%s: accept client n=%d",__FUNCTION__, n);
        for(i=0; i<MAX_CLIENT_NUM; i++) {
            if(engpc_client_fd[i]==-1){
                engpc_client_fd[i]=n;
                   write(notifypipe[1], "0", 2);
                MODEMD_LOGD("%s: fill %d to client[%d]\n",__FUNCTION__, n, i);
                break;
            }
            /* if client_fd arrray is full, just fill the new socket to the
             * last element */
            if(i == MAX_CLIENT_NUM - 1) {
                MODEMD_LOGD("%s: client array is full, just fill %d to client[%d]",
                        __FUNCTION__, n, i);
                engpc_client_fd[i]=n;
                   write(notifypipe[1], "0", 2);
                           break;
            }
        }
        }
    close(notifypipe[1]);
    return NULL;
}

void start_modem (char *modem)
{
    pthread_t tid1, tid2;
    static int blk = 0;
    char modem_dev[PROPERTY_VALUE_MAX];
    int vlx = 0;

    MODEMD_LOGD("start_modem() get ModemType %s", modem);

    /* if modem is not alive, wait modem alive here */
    if(modem_alive != 1){
        pthread_mutex_lock(&aliveMutex);
        pthread_cond_wait(&aliveCond, &aliveMutex);
        pthread_mutex_unlock(&aliveMutex);
    }

    MODEMD_LOGD("%s modem is enabled", modem);
    snprintf(SP_PROC_PROP, sizeof(SP_PROC_PROP), "ro.modem.%s.dev", modem);
    property_get(SP_PROC_PROP, modem_dev, "");

    if (strcmp(modem, "t") == 0 || strcmp(modem, "w") == 0 || strcmp(modem, "l") == 0
            || strcmp(modem, "lf") == 0 || strcmp(modem, "tl") == 0){
        vlx = 0;
    }
    if (vlx == 0) {
        MODEMD_LOGD("It's %s native version", modem);
        //start_service(modem, 0, 0);
        if (!is_external_modem()) { // internal modem start sipc detect.
            /*  sipc td/w/lte/tl/lf modem */
            /*remove the detect sipc modem action, It has been moved into modem control */
            //if (pthread_create(&tid1, NULL, (void*) detect_sipc_modem, modem) < 0) {
                //MODEMD_LOGE("Failed to create %s SIPC detected thread.", modem);
            //}
        }
        if (!blk) {
            if (pthread_create(&tid2, NULL, (void*) detect_modem_blocked, modem) < 0) {
                MODEMD_LOGE("Failed to create %s modem blocked thread.", modem);
            } else {
                blk = 1;
            }
        }
    } else {
        /*  vlx version, only one modem */
        MODEMD_LOGD("It's vlx version");
        vlx_reboot_init();
        start_service(modem, 1, 0);
        detect_vlx_modem(modem);
    }
    MODEMD_LOGD("start_modem() done");
}

static void* detect_modem_control(void *param)
{
   int  numRead,is_reset=0;
   char buf[128];
   int ret = -1;
   int modem_value = -1;
   char prop[PROPERTY_VALUE_MAX] = {0};
   char modem[PROPERTY_VALUE_MAX] = {0};

reconnect:
   MODEMD_LOGD("%s: try to connect socket %s...", __func__, SOCKET_NAME_MODEM_CTL);

   do{
      usleep(10*1000);
      control_fd = socket_local_client(SOCKET_NAME_MODEM_CTL,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
   }while(control_fd < 0);
   MODEMD_LOGD("%s: connect socket %s success", __func__, SOCKET_NAME_MODEM_CTL);

   do{
       memset(buf, 0, sizeof(buf));
       MODEMD_LOGD("%s: Monioring modem state on socket %s...", __func__, SOCKET_NAME_MODEM_CTL);
       do {
          numRead = read(control_fd, buf, sizeof(buf));
          MODEMD_LOGD("read %d from fd %d ,erro %s", numRead, control_fd,
              (numRead < 0 ? strerror(errno) : 0));

       } while(numRead < 0 && errno == EINTR);

       if(numRead <= 0) {
           close(control_fd);
           goto reconnect;
       }

       //get modem type from prop
       property_get("ro.radio.modemtype",modem, "");
       MODEMD_LOGD("the modem type is %s!", modem);

       MODEMD_LOGD("%s: read numRead=%d, buf=%s", __func__, numRead, buf);
       if (strstr(buf, "Modem Alive")) {
           //wait_for_modem_alive(modem);
           MODEMD_LOGD("Info modem alive to all clients.");
           modem_alive = 1;
           pthread_mutex_lock(&aliveMutex);
           pthread_cond_signal(&aliveCond);
           pthread_mutex_unlock(&aliveMutex);
           loop_info_sockclients(buf, numRead);
           start_service(modem, 0, 0);
       } else if (strstr(buf, "Modem Assert") ) {
           MODEMD_LOGD("Info Modem Assert to all clients.");
           //detect_clients_dispose_state(MINIDUMP);
           //sendBroadcast("com.android.modemassert.MODEM_STAT_CHANGE","modem_assert",buf);
           loop_info_sockclients(buf, numRead);
           stop_service(modem, 0);
           memset(prop, 0, sizeof(prop));
           property_get(MODEM_RESET_PROP, prop, "0");
           is_reset = atoi(prop);
           MODEMD_LOGD("reload modem: %d",is_reset);
           if(is_reset){
               ret = write(control_fd, "Prepare Reset", sizeof("Prepare Reset"));
               if( ret <= 0) {
                    close(control_fd);
                    control_fd = -1;
                    goto reconnect;
               }
           }
       } else if (strstr(buf, "Modem Reset") ) {
           //sendBroadcast("com.android.modemassert.MODEM_STAT_CHANGE","modem_reset",buf);
           MODEMD_LOGD("Info modem reset to all clients.");
           loop_info_sockclients(buf, numRead);
           stop_service(modem, 0);
           ret = write(control_fd, "Modem Reset", sizeof("Modem Reset"));
           if(ret <= 0) {
                close(control_fd);
                control_fd = -1;
                goto reconnect;
           }
       }
   }while(1);

   close(control_fd);
   return (void*) NULL;
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    struct sigaction action;
    int ret;
    char ModemType[PROPERTY_VALUE_MAX] = {0};

    memset(&action, 0x00, sizeof(action));
    action.sa_handler = SIG_IGN;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    ret = sigaction (SIGPIPE, &action, NULL);
    if (ret < 0) {
        MODEMD_LOGE("sigaction() failed!");
        exit_modemd();
    }

    if (pthread_create(&tid, NULL, (void*)modemd_listenaccept_thread, NULL) < 0){
        MODEMD_LOGE("Failed to create modemd listen accept thread");
    }
    pthread_create(&tid, NULL, (void*)modemd_engcontrol_thread, NULL);

    if (pthread_create(&tid, NULL, (void*)detect_modem_control, NULL) < 0){
        MODEMD_LOGE("Failed to create modemd listen accept thread");
    }
    /* for vlx version, there is only one modem, once one of the follow two functions
     *   matched, it will execute forever and never retrun to check the next modem
     */

    if (!is_external_modem()) {
        property_get("ro.radio.modemtype", ModemType, "");
        if (strcmp(ModemType, "") != 0) {
            /* start td/w/l/tl/lf modem*/
            start_modem(ModemType);
        } else {
            MODEMD_LOGE("ERROR: property for the modem type isn't configured.");
            return 0;
        }
    } else {
        /* start external  modem*/
        start_ext_modem();
    }

    do {
        pause();
    } while(1);

    return 0;
}
