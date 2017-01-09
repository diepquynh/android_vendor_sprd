#define LOG_TAG     "MODEMD"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include "modemd.h"
#include "modemd_ext.h"
#include <hardware_legacy/power.h>


#define  ANDROID_WAKE_LOCK_NAME "modemd-ext"

#define  SOCKET_NAME_MODEM_CTL  "modem_control"

static int  sStartLteThread = 0;
static pthread_t  sTidStartLte = 0;
static char *sLTEmodem = "l";
static char *sTDmodem = "t";
static int  sSSDAMode   = 0;  // 0 is SVLTE mode, or CSFB mode
static int  sTestMode   = 0;  // 0 is Test Mode SVLTE
static int  sFdModemCtl = -1;

static void  start_external_modem(void);
static void  stop_external_modem(void);
static void  stop_svlte_service(void);
static void* start_svlte_service(void);
static void  start_svlte_mode(void);
static ext_modem_ops_t  ext_modem_ops;

extern int loop_info_sockclients(const char* buf, const int len);
extern int sp_modem_state;
extern pthread_mutex_t sp_state_mutex;
extern pthread_cond_t  sp_cond;

static int is_modem_reset() {

   char prop[PROPERTY_VALUE_MAX] = "";

   property_get(MODEM_RESET_PROP, prop, "0");

   return atoi(prop);
}

static int is_svlte_mode(void)
{
    char prop[PROPERTY_VALUE_MAX]="";

    property_get(SSDA_MODE_PROP, prop, SVLTE_MODE);
    MODEMD_LOGD("LTE mode %s", prop);

    if (!strcmp(prop, SVLTE_MODE)) {
        return 1;
    } else {
        return 0;
    }
}

static int get_test_mode(void)
{
    char prop[PROPERTY_VALUE_MAX]="0";
    property_get(SSDA_TESTMODE_PROP, prop, 0);
    MODEMD_LOGD("Test mode %s", prop);

    return atoi(prop);
}

static int is_test_mode_changed(void)
{
    return  (sTestMode == get_test_mode()) ? 0 : 1 ;
}

static int send_reload_modemd_message() {
    const char* loadcmd = sSSDAMode ? LTE_RELOAD_CSFB_STR : LTE_RELOAD_SVLTE_STR;

    if (sFdModemCtl != -1) {
        MODEMD_LOGD("Send reload modem image command %s", loadcmd);
        if (write(sFdModemCtl, loadcmd, strlen(loadcmd) + 1) < 0) {
            MODEMD_LOGE("Fail to write reload modem, error: %s", strerror(errno));
            return -1;
        }
    } else {
        MODEMD_LOGE("send_reload_modemd_message sFdModemCtl %d", sFdModemCtl);
        return -1;
    }

    return 0;
}
/* Read modem state form socket modemcontrol, then boardcast to modemd client.*/
static void* detect_modem_control()
{
   int  sfd, numRead;
   char buf[128];

reconnect:
   MODEMD_LOGD("%s: try to connect socket %s...", __func__, SOCKET_NAME_MODEM_CTL);
   sfd = socket_local_client(SOCKET_NAME_MODEM_CTL,
         ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

   while(sfd < 0) {
      usleep(10*1000);
      sfd = socket_local_client(SOCKET_NAME_MODEM_CTL,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
   }
   MODEMD_LOGD("%s: connect socket %s success", __func__, SOCKET_NAME_MODEM_CTL);
   sFdModemCtl = sfd;

   for(;;) {
       memset(buf, 0, sizeof(buf));
       MODEMD_LOGD("%s: Monioring modem state on socket %s...", __func__, SOCKET_NAME_MODEM_CTL);
       do {
          numRead = read(sfd, buf, sizeof(buf));
       } while(numRead < 0 && errno == EINTR); 

       if(numRead <= 0) {
           MODEMD_LOGE("%s: read numRead=%d, error: %s", __func__, numRead, strerror(errno));
           close(sfd);
           sFdModemCtl = -1;
           goto reconnect;
       }

       MODEMD_LOGD("%s: read numRead=%d, buf=%s", __func__, numRead, buf);
       if (strstr(buf, TD_MODEM_ALIVE_STR) ||
           strstr(buf, LTE_MODEM_ALIVE_STR) ||
           strstr(buf, GEN_MODEM_ALIVE_STR)) {
           // Start external modem, when receive modem alive info.
           ext_modem_ops.start_modem_service();

           MODEMD_LOGD("Info modem alive to all clients.");
           //loop_info_sockclients("Modem Alive", strlen("Modem Alive"));
           loop_info_sockclients(buf, numRead);
       } else if (strstr(buf, TD_MODEM_ASSERT_STR) ||
                  strstr(buf, LTE_MODEM_ASSERT_STR) ||
                  strstr(buf, GEN_MODEM_ASSERT_STR)) {
           ext_modem_ops.stop_modem_service();

           if (is_test_mode_changed()) {
                 MODEMD_LOGD("Test mode is changed , reload modem image");
           } else {
              if (!is_modem_reset()) {
                  MODEMD_LOGD("Modem reset is not enabled , do not reset");
              }
              MODEMD_LOGD("Info modem assert to all clients.");
              loop_info_sockclients(buf, numRead);
           }

           if (is_modem_reset() || is_test_mode_changed()) {
               MODEMD_LOGD("Modem reset is enabled, reload modem image");
               if (ext_modem_ops.load_modem_image() < 0) {
                   close(sfd);
                   sFdModemCtl = -1;
                   goto reconnect;
               }
           } 
       } else if (strstr(buf, LTE_MODEM_RESET_STR) ||
                  strstr(buf, WTD_MODEM_RESET_STR) ||
                  strstr(buf, GEN_MODEM_RESET_STR)) {
           MODEMD_LOGD("modem reset happen, reload modem...");
           MODEMD_LOGD("Info modem reset to all clients.");
           //loop_info_sockclients("Modem Reset", strlen("Modem Reset"));
           loop_info_sockclients(buf, numRead);

           ext_modem_ops.stop_modem_service();
           if (ext_modem_ops.load_modem_image() < 0) {
               close(sfd);
               sFdModemCtl = -1;
               goto reconnect;
           }
       }
   }

   close(sfd);
   sFdModemCtl = -1;
   return (void*) NULL;
}

static void stop_svlte_service(void)
{
    if (sStartLteThread) {
         sStartLteThread = 0;
         pthread_join(sTidStartLte, NULL);
         sTidStartLte = 0;
    }
    stop_service(sLTEmodem, 0);
}

/* start LTE modem relation service */
static void* start_svlte_service(void)
{
    char ltes[PROPERTY_VALUE_MAX]  = "-1";
    int  lteStart = LTE_MODEM_DEFAULT;

    while (sStartLteThread) {
      property_get(LTE_MODEM_START_PROP, ltes, "-1");
      lteStart = atoi(ltes);

      if (lteStart == LTE_MODEM_DEFAULT) {
         sleep(1);
         continue;
      } else if (lteStart == LTE_MODEM_ON) {
         start_modem(sLTEmodem);
         break;
      } else if (lteStart == LTE_MODEM_OFF){
         MODEMD_LOGD("LTE Modem needn't start, so exit!!!");
         return NULL;
      } else {
         /*
         * sometime the prop can't be gotten, so add this.
         */
         MODEMD_LOGE("LTE Modem property value(%s) is invalid.", ltes);
         sleep(2);
      }
    } 

    sStartLteThread = 0;
    sTidStartLte = 0;
    return (void*) NULL;
}

static void start_svlte_mode(void)
{
    if (sStartLteThread == 1) {
        MODEMD_LOGD("LTE Modem thread is running.");
        return;
    }

    MODEMD_LOGD("start LTE modem thread for SVLTE.");
    sStartLteThread = 1;
    if (pthread_create(&sTidStartLte, NULL, (void*)start_svlte_service, NULL) < 0) {
        MODEMD_LOGE("Failed to create lte service thread.");
        sStartLteThread = 0;
        return ;
    }
}

static void getPartialWakeLock() {
    acquire_wake_lock(PARTIAL_WAKE_LOCK, ANDROID_WAKE_LOCK_NAME);
}

static void releaseWakeLock() {
    release_wake_lock(ANDROID_WAKE_LOCK_NAME);
}

static void start_external_modem(void)
{
    if (is_svlte_mode()) {
        sSSDAMode   = 0;
        getPartialWakeLock();
        start_modem(sTDmodem);
        releaseWakeLock();
        MODEMD_LOGD("start TD modem service for SVLTE.");
        pthread_mutex_lock(&sp_state_mutex);
        sp_modem_state = MODEM_READY;
        pthread_cond_signal(&sp_cond);
        pthread_mutex_unlock(&sp_state_mutex);

        sTestMode = get_test_mode();
        if (sTestMode == SSDA_TEST_MODE_SVLTE) {  // SVLTE TESTMODE, Start svlte thread
            MODEMD_LOGD("start LTE modem service for SVLTE.");
            start_svlte_mode();

            pthread_mutex_lock(&sp_state_mutex);
            sp_modem_state = MODEM_READY;
            pthread_cond_signal(&sp_cond);
            pthread_mutex_unlock(&sp_state_mutex);
        }
    } else {
        MODEMD_LOGD("start LTE modem service for CSFB.");
        getPartialWakeLock();
        start_modem(sLTEmodem);
        releaseWakeLock();
        sSSDAMode   = 1;

        sTestMode = get_test_mode();
        pthread_mutex_lock(&sp_state_mutex);
        sp_modem_state = MODEM_READY;
        pthread_cond_signal(&sp_cond);
        pthread_mutex_unlock(&sp_state_mutex);
    }
}

static void stop_external_modem(void)
{
    if (sSSDAMode) {
        pthread_mutex_lock(&sp_state_mutex);
        sp_modem_state = MODEM_ASSERT;
        pthread_mutex_unlock(&sp_state_mutex);
        getPartialWakeLock();
        stop_service(sLTEmodem, 0);
        releaseWakeLock();
    } else {
        if (sTestMode == SSDA_TEST_MODE_SVLTE) {  // SVLTE TESTMODE, Start svlte thread
            pthread_mutex_lock(&sp_state_mutex);
            sp_modem_state = MODEM_ASSERT;
            pthread_mutex_unlock(&sp_state_mutex);
            stop_svlte_service();
        }
        pthread_mutex_lock(&sp_state_mutex);
        sp_modem_state = MODEM_ASSERT;
        pthread_mutex_unlock(&sp_state_mutex);
        getPartialWakeLock();
        stop_service(sTDmodem, 0);
        releaseWakeLock();
    }
}

static void init_ext_modem(void) {
    ext_modem_ops.start_modem_service = start_external_modem;
    ext_modem_ops.stop_modem_service  = stop_external_modem;
    ext_modem_ops.load_modem_image    = send_reload_modemd_message;
}

void start_ext_modem(void)
{
    pthread_t tid;

    init_ext_modem();
    /* start external  modem*/
    if (pthread_create(&tid, NULL, (void*)detect_modem_control, NULL) < 0) {
         MODEMD_LOGE("Failed to Create detect modem control thread!");
    }
}

int is_external_modem(void) {

    char prop[PROPERTY_VALUE_MAX]="0";

    property_get(MODEM_EXTERNAL_PROP, prop, "0");
    MODEMD_LOGD("%s value is %s", MODEM_EXTERNAL_PROP, prop);

    return atoi(prop);
}

ext_modem_ops_t *get_ext_modem_if() {
    return &ext_modem_ops;
}

