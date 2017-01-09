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
static int pc_fd = -1;

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <cutils/str_parms.h>
#include <cutils/sockets.h>
#include <dlfcn.h>

#if (defined CONFIG_BQBTEST) || (defined CONFIG_BRCM_BQBTEST)
#include "bqb.h"

static const char *VENDOR_LIBRARY_NAME = "libbqbbt.so";
static const char *VENDOR_LIBRARY_SYMBOL_NAME = "BLUETOOTH_BQB_INTERFACE";
static void *lib_handle;
static bt_bqb_interface_t *lib_interface = NULL;
static bool bqb_vendor_open() {
  lib_handle = dlopen(VENDOR_LIBRARY_NAME, RTLD_NOW);
  if (!lib_handle) {
    ALOGD("%s unable to open %s: %s", __func__, VENDOR_LIBRARY_NAME, dlerror());
    goto error;
  }

  lib_interface = (bt_bqb_interface_t *)dlsym(lib_handle, VENDOR_LIBRARY_SYMBOL_NAME);
  if (!lib_interface) {
    ALOGD("%s unable to find symbol %s in %s: %s", __func__, VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME, dlerror());
    goto error;
  }
  lib_interface->init();

  return true;

error:;
  lib_interface = NULL;
  if (lib_handle)
    dlclose(lib_handle);
  lib_handle = NULL;
  return false;
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

#if (defined CONFIG_BQBTEST) || (defined CONFIG_BRCM_BQBTEST)
    if(lib_interface !=NULL)
        lib_interface->set_fd(pc_fd);
#endif

    tcgetattr(pc_fd, &ser_settings);
    cfmakeraw(&ser_settings);

    ser_settings.c_lflag |= (ECHO | ECHONL);
    ser_settings.c_lflag &= ~ECHOCTL;


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
    int ret;
    int max_fd = pc_fd;
    fd_set read_set;

    for (;;) {
        ENG_LOG("wait for command / byte stream");
        FD_ZERO(&read_set);

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
        if (FD_ISSET(pc_fd, &read_set)) {
            memset(engbuf, 0, ENG_BUFFER_SIZE);
            len = read(pc_fd, engbuf, ENG_BUFFER_SIZE);
            if (len <= 0) {
                ENG_LOG("%s: read pc_fd buffer error %s",__FUNCTION__,strerror(errno));
                sleep(1);
                start_gser(dev_info->host_int.dev_at);
                continue;
            }

            ENG_LOG("pc got: %s: %d", engbuf, len);

#if (defined CONFIG_BQBTEST) || (defined CONFIG_BRCM_BQBTEST)
            if(lib_interface->check_received_str(pc_fd, engbuf, len))
                continue;
#endif
        } else {
            ENG_LOG("warning !!!");
        }
#ifdef CONFIG_BQBTEST
        if (lib_interface->get_bqb_state() == BQB_OPENED) {
            lib_interface->eng_send_data(engbuf, len);
        } else
#endif
        {
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
#if (defined CONFIG_BQBTEST) || (defined CONFIG_BRCM_BQBTEST)
    bqb_vendor_open();
#endif
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
