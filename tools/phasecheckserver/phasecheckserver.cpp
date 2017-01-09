#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <jni.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <android/log.h>
#include <string.h>
#include "NativeService.h"
#include "engphasecheck.h"

//#define PHASE_CHECKE_FILE  "/dev/block/platform/sprd-sdhci.3/by-name/miscdata"
#define PHASE_CHECKE_FILE  "/dev/block/platform/sdio_emmc/by-name/miscdata"
#define CHARGE_SWITCH_FILE  "/sys/class/power_supply/battery/stop_charge"

namespace android
{
    //static struct sigaction oldact;
    static pthread_key_t sigbuskey;

    int NativeService::Instance()
    {
        ALOGE("phasecheck_sprd new ..NativeService Instantiate\n");
        int ret = defaultServiceManager()->addService(
                String16("phasechecknative"), new NativeService());
        ALOGE("phasecheck_sprd new ..NatveService ret = %d\n", ret);
        return ret;
    }

    NativeService::NativeService()
    {
        ALOGE("phasecheck_sprd NativeService create\n");
        //m_NextConnId = 1;
        pthread_key_create(&sigbuskey,NULL);
    }

    NativeService::~NativeService()
    {
        pthread_key_delete(sigbuskey);
        ALOGE("phasecheck_sprd NativeService destory\n");
    }

    char * get_sn1()
    {
        SP09_PHASE_CHECK_T Phase;
        SP09_PHASE_CHECK_T* pPhase = &Phase;
        SP15_PHASE_CHECK_T phase_check_sp15;
        int readnum;
        unsigned int magic;
        FILE *fd = NULL;
        int len;
        fd = fopen(PHASE_CHECKE_FILE,"r");
        if(NULL != fd){
            readnum = fread(&magic, sizeof(unsigned int), 1, fd);
            fclose(fd);
            ALOGE("mmitest magic: 0x%x!;sizeof(phase_check_sp15)= %d, %d IN", magic, sizeof(phase_check_sp15), __LINE__);
        }else{
            ALOGE("fail to open miscdata! %d IN", __LINE__);
        }
        if(magic == SP09_SPPH_MAGIC_NUMBER || magic == SP05_SPPH_MAGIC_NUMBER){
            ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = point 1 \n",__FUNCTION__);
            int ifd = open(PHASE_CHECKE_FILE,O_RDWR);
            if (ifd >= 0) {
                ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                len = read(ifd,pPhase,sizeof(SP09_PHASE_CHECK_T));
                close(ifd);
                if (len <= 0){
                    ENG_LOG("phasecheck_sprd: %s read fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                    return NULL;
                }
            } else {
                ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return NULL;
            }
            ENG_LOG("phasecheck_sprd SN1 %s'", pPhase->SN1);
            ENG_LOG("phasecheck_sprd SN2 %s'", pPhase->SN2);

            return pPhase->SN1;
        }else if(magic == SP15_SPPH_MAGIC_NUMBER){
            fd = fopen(PHASE_CHECKE_FILE, "r");
            if(NULL != fd){
                readnum = fread(&phase_check_sp15, sizeof(SP15_PHASE_CHECK_T), 1, fd);
                fclose(fd);
            }else{
                ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return NULL;
            }
            return phase_check_sp15.SN1;
        }
        return NULL;
    }
    char * get_sn2()
    {
        SP09_PHASE_CHECK_T Phase;
        SP09_PHASE_CHECK_T* pPhase = &Phase;
        SP15_PHASE_CHECK_T phase_check_sp15;
        int readnum;
        unsigned int magic;
        FILE *fd = NULL;
        int len;
        fd = fopen(PHASE_CHECKE_FILE,"r");
        if(NULL != fd){
            readnum = fread(&magic, sizeof(unsigned int), 1, fd);
            fclose(fd);
            ALOGE("mmitest magic: 0x%x!;sizeof(phase_check_sp15)= %d, %d IN", magic, sizeof(phase_check_sp15), __LINE__);
        }else{
            ALOGE("fail to open miscdata! %d IN", __LINE__);
        }
        if(magic == SP09_SPPH_MAGIC_NUMBER || magic == SP05_SPPH_MAGIC_NUMBER){
            ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = point 1 \n",__FUNCTION__);
            int ifd = open(PHASE_CHECKE_FILE,O_RDWR);
            if (ifd >= 0) {
                ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                len = read(ifd,pPhase,sizeof(SP09_PHASE_CHECK_T));
                close(ifd);
                if (len <= 0){
                    ENG_LOG("phasecheck_sprd: %s read fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                    return NULL;
                }
            } else {
                ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return NULL;
            }
            ENG_LOG("phasecheck_sprd SN1 %s'", pPhase->SN1);
            ENG_LOG("phasecheck_sprd SN2 %s'", pPhase->SN2);

            return pPhase->SN2;
        }else if(magic == SP15_SPPH_MAGIC_NUMBER){
            fd = fopen(PHASE_CHECKE_FILE, "r");
            if(NULL != fd){
                readnum = fread(&phase_check_sp15, sizeof(SP15_PHASE_CHECK_T), 1, fd);
                fclose(fd);
            }else{
                ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return NULL;
            }
            return phase_check_sp15.SN2;
        }
        return NULL;
    }
    char *str_cat(char *ret, const char *str1, const char *str2, bool first){
        int len1 = 0;
        int len2 = 0;
        for (len1 = 0; *(str1+len1) != '\0'; len1++){}
        for (len2 = 0; *(str2+len2) != '\0'; len2++){}
        ENG_LOG("phasecheck_sprd str1 len: %d'", len1);
        ENG_LOG("phasecheck_sprd str2 len: %d'", len2);
        int i;
        if(first) {
            for (i=0; i<len1; i++){
                *(ret+i) = *(str1+i);
            }
        }
        *(ret+len1) = '|';
        for (i=0; i<len2; i++){
            *(ret+len1+i+1) = *(str2+i);
        }
        *(ret+len1+len2+1) = '\0';

        return ret;
    }
    char* get_phasecheck(char *ret, int* testSign, int* item)
    {
        SP09_PHASE_CHECK_T Phase;
        SP09_PHASE_CHECK_T* pPhase = &Phase;
        SP15_PHASE_CHECK_T phase_check_sp15;
        int readnum;
        unsigned int magic;
        FILE *fd = NULL;
        int len;
        fd = fopen(PHASE_CHECKE_FILE,"r");
        if(NULL != fd){
            readnum = fread(&magic, sizeof(unsigned int), 1, fd);
            fclose(fd);
            ALOGE("mmitest magic: 0x%x!;sizeof(phase_check_sp15)= %d, %d IN", magic, sizeof(phase_check_sp15), __LINE__);
        }else{
            ALOGE("fail to open miscdata! %d IN", __LINE__);
        }
        if(magic == SP09_SPPH_MAGIC_NUMBER || magic == SP05_SPPH_MAGIC_NUMBER){
            ENG_LOG("phasecheck_sprd: %s magic number in [SP09,SP05] \n",__FUNCTION__);
            int ifd = open(PHASE_CHECKE_FILE,O_RDWR);
            if (ifd >= 0) {
                ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                len = read(ifd,pPhase,sizeof(SP09_PHASE_CHECK_T));
                close(ifd);
                if (len <= 0){
                    ENG_LOG("phasecheck_sprd: %s read fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                    return NULL;
                }
            } else {
                ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return NULL;
            }
            if(pPhase->StationNum <= 0) return NULL;
            if(pPhase->StationNum == 1) return pPhase->StationName[0];
            unsigned int i = 0;
            while(i < pPhase->StationNum) {
                ENG_LOG("phasecheck_sprd StationName[%d]: %s'",i, pPhase->StationName[i]);
                if(i == 0) {
                    str_cat(ret, pPhase->StationName[i], pPhase->StationName[i+1], true);
                    ENG_LOG("phasecheck_sprd i = 0 result: %s'", ret);
                } else if(i != 1) {
                    str_cat(ret, ret, pPhase->StationName[i], false);
                    ENG_LOG("phasecheck_sprd result: %s'", ret);
                }
                i++;
            }

            ENG_LOG("phasecheck_sprd iTestSign %x'", pPhase->iTestSign);
            ENG_LOG("phasecheck_sprd iItem %x'", pPhase->iItem);
            *testSign = (int)pPhase->iTestSign;
            *item = (int)pPhase->iItem;

        }else if(magic == SP15_SPPH_MAGIC_NUMBER){
            ENG_LOG("phasecheck_sprd: %s magic number is SP15 \n",__FUNCTION__);
            fd = fopen(PHASE_CHECKE_FILE, "r");
            if(NULL != fd){
                readnum = fread(&phase_check_sp15, sizeof(SP15_PHASE_CHECK_T), 1, fd);
                fclose(fd);
            }else{
                ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return NULL;
            }
            if(phase_check_sp15.StationNum <= 0) return NULL;
            if(phase_check_sp15.StationNum == 1) return phase_check_sp15.StationName[0];
            unsigned int i = 0;
            while(i < phase_check_sp15.StationNum) {
                ENG_LOG("phasecheck_sprd StationName[%d]: %s'",i, phase_check_sp15.StationName[i]);
                if(i == 0) {
                    str_cat(ret, phase_check_sp15.StationName[i], phase_check_sp15.StationName[i+1], true);
                    ENG_LOG("phasecheck_sprd i = 0 result: %s'", ret);
                } else if(i != 1) {
                    str_cat(ret, ret, phase_check_sp15.StationName[i], false);
                    ENG_LOG("phasecheck_sprd result: %s'", ret);
                }
                i++;
            }

            ENG_LOG("phasecheck_sprd iTestSign %x'", phase_check_sp15.iTestSign);
            ENG_LOG("phasecheck_sprd iItem %x'", phase_check_sp15.iItem);
            *testSign = (int)phase_check_sp15.iTestSign;
            *item = (int)phase_check_sp15.iItem;
        }

        return ret;
    }

    jint eng_writephasecheck(jint type, jint station, jint value)
    {
        int ret;
        int len;
        SP09_PHASE_CHECK_T Phase;
        SP09_PHASE_CHECK_T* pPhase = &Phase;
        printf("%s open Ok PHASE_CHECKE_FILE = point 1 \n",__FUNCTION__);
        int fd = open(PHASE_CHECKE_FILE,O_RDWR);
        if (fd >= 0)
        {
            printf("%s open Ok PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
            len = read(fd,pPhase,sizeof(SP09_PHASE_CHECK_T));
            close(fd);
            if (len <= 0){
                printf("%s read fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                return -1;
            }
        } else {
            printf("%s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
            return -1;
        }

        fd = open(PHASE_CHECKE_FILE,O_RDWR);
        if (fd > 0){
            if(type == SIGN_TYPE)
            {
                if(value)
                {
                    pPhase->iTestSign |= (unsigned short)(1<<station);
                } else {
                    pPhase->iTestSign &= (unsigned short)(~(1<<station));
                }

                len = write(fd,pPhase,sizeof(SP09_PHASE_CHECK_T));
                fsync(fd);
                ENG_LOG("phasecheck_sprd iTestSign 0x%x'", pPhase->iTestSign);
            }

            if(type == ITEM_TYPE)
            {
                if(value)
                {
                    pPhase->iItem |= (unsigned short)(1<<station);
                } else {
                    pPhase->iItem &= (unsigned short)(~(1<<station));
                }

                len = write(fd,pPhase,sizeof(SP09_PHASE_CHECK_T));
                fsync(fd);
                ENG_LOG("phasecheck_sprd iItem 0x%x'", pPhase->iItem);
            }
            close(fd);
        }else{
            printf("engphasecheck------open fail chmod 0600 \n");
            ret = -1;
        }
        return ret;
    }

    jint enable_charge(void)
    {
        int fd;
        int ret = -1;
        fd = open(CHARGE_SWITCH_FILE,O_WRONLY);
        if(fd >= 0){
            ret = write(fd,"0",2);
            if(ret < 0){
            close(fd);
            return 0;
            }
            close(fd);
        }else{
            return 0;
        }
        return 1;
    }

    jint disable_charge(void)
    {
        int fd;
        int ret = -1;
        fd = open(CHARGE_SWITCH_FILE,O_WRONLY);
        if(fd >= 0){
            ret = write(fd,"1",2);
            if(ret < 0){
            close(fd);
           return 0;
            }
            close(fd);
        }else{
           return 0;
        }
        return 1;
    }

    void write_ledlight_node_values(const char * pathname,jint value) {
        int len;
        char out;
        int fd = open(pathname, O_RDWR);
        if (fd > 0) {
            if(value != 2){
               out = value == 0 ? '0' : '1';
            }else{
               out = '2';
            }
            len = write(fd, &out, 1);
            close(fd);
        } else {
            ALOGE("engphasecheck------write_ledlight_node_values open fail chmod 0600 \n");
        }
    }

    status_t NativeService::onTransact(uint32_t code,
                                   const Parcel& data,
                                   Parcel* reply,
                                   uint32_t flags)
    {
        ALOGE("phasecheck_sprd nativeservice onTransact code:%d",code);
        switch(code)
        {
        case 0:
            {
                //get sn
                static int i = 1;
                ALOGE("phasecheck_sprd nativeservice get sn count %d",i++);
                char * sn1 = get_sn1();
                ALOGE("phasecheck_sprd nativeservice sn1 read is %s",sn1);
                reply->writeString16(String16(sn1));
                return NO_ERROR;
            } break;
        case 1:
            {
                //get sn
                static int i = 1;
                ALOGE("phasecheck_sprd nativeservice get sn count %d",i++);
                char * sn2 = get_sn2();
                ALOGE("phasecheck_sprd nativeservice sn2 read is %s", sn2);
                reply->writeString16(String16(sn2));
                return NO_ERROR;
            } break;
        case 2:
            {
                //write station tested
                static int i = 1;
                ALOGE("phasecheck_sprd nativeservice write station tested count %d",i++);
                int station = data.readInt32();
                eng_writephasecheck(0, station, 0);
                return NO_ERROR;
            } break;
        case 3:
            {
                //write station pass
                static int i = 1;
                ALOGE("phasecheck_sprd nativeservice write station pass count %d",i++);
                int station = data.readInt32();
                eng_writephasecheck(1, station, 0);
                return NO_ERROR;
            } break;
        case 4:
            {
                //write station fail
                static int i = 1;
                ALOGE("phasecheck_sprd nativeservice write station count %d",i++);
                int station = data.readInt32();
                eng_writephasecheck(1, station, 1);
                return NO_ERROR;
            } break;
        case 5:
            {
                //get phasecheck
                static int i = 1;
                ALOGE("phasecheck_sprd nativeservice get phasecheck count %d",i++);
                int testSign, item;
                char *ret = (char *)malloc(sizeof(char)*(SP09_MAX_STATION_NUM*SP09_MAX_STATION_NAME_LEN));
                get_phasecheck(ret, &testSign, &item);
                ALOGE("phasecheck_sprd nativeservice get phasecheck is %s", ret);
                ALOGE("phasecheck_sprd nativeservice get phasecheck station is %x, %x", testSign, item);
                reply->writeInt32(testSign);
                reply->writeInt32(item);
                reply->writeString16(String16(ret));
                free(ret);
                return NO_ERROR;
            } break;
        case 6:
            {
                //write to charge switch
                static int i = 1;
                int value = data.readInt32();
                ALOGE("phasecheck_sprd nativeservice write station count %d. value: %d",i++,value);
                static int result = -1;
                if(value == 0){
                     result = enable_charge();
                }else if(value == 1){
                     result = disable_charge();
                }
                if(result == 0){
                     reply->writeString16(String16("fail write charge node !"));
                }else if(result == 1){
                     reply->writeString16(String16("success write charge node !"));
                }
                return NO_ERROR;
             } break;
        case 7:
            {
                //write values to leds system node.
                static int i = 1;
                int value = data.readInt32();
                ALOGE("phasecheck_sprd nativeservice write station count %d. value: %d", i++, value);
                if (value == 1) {
                     write_ledlight_node_values("/sys/class/leds/red_bl/high_time", 2);
                     write_ledlight_node_values("/sys/class/leds/red_bl/low_time", 2);
                     write_ledlight_node_values("/sys/class/leds/red_bl/rising_time", 2);
                     write_ledlight_node_values("/sys/class/leds/red_bl/falling_time",2);
                }
                write_ledlight_node_values("/sys/class/leds/red_bl/on_off", value);
                reply->writeString16(String16("OK"));
                return NO_ERROR;
            }break;
        case 8:
            {
                //write values to leds system node.
                static int i = 1;
                int value = data.readInt32();
                ALOGE("phasecheck_sprd nativeservice write station count %d. value: %d", i++, value);
                if (value == 1) {
                     write_ledlight_node_values("/sys/class/leds/blue_bl/high_time", 2);
                     write_ledlight_node_values("/sys/class/leds/blue_bl/low_time", 2);
                     write_ledlight_node_values("/sys/class/leds/blue_bl/rising_time",2);
                     write_ledlight_node_values("/sys/class/leds/blue_bl/falling_time",2);
                }
                write_ledlight_node_values("/sys/class/leds/blue_bl/on_off", value);
                reply->writeString16(String16("OK"));
                return NO_ERROR;
            }break;
        case 9:
            {
                //write values to leds system node.
                static int i = 1;
                int value = data.readInt32();
                ALOGE("phasecheck_sprd nativeservice write station count %d. value: %d", i++, value);
                if (value == 1) {
                     write_ledlight_node_values("/sys/class/leds/green_bl/high_time", 2);
                     write_ledlight_node_values("/sys/class/leds/green_bl/low_time", 2);
                     write_ledlight_node_values("/sys/class/leds/green_bl/rising_time",2);
                     write_ledlight_node_values("/sys/class/leds/green_bl/falling_time",2);
                }
                write_ledlight_node_values("/sys/class/leds/green_bl/on_off", value);
                reply->writeString16(String16("OK"));
                return NO_ERROR;
            }break;
        case 10:
            {
                int len;
                SP09_PHASE_CHECK_T Phase;
                SP09_PHASE_CHECK_T* pPhase = &Phase;
                ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = point 1 \n",__FUNCTION__);
                int fd = open(PHASE_CHECKE_FILE,O_RDWR);
                if (fd >= 0)
                {
                    ENG_LOG("phasecheck_sprd: %s open Ok PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                    len = read(fd,pPhase,sizeof(SP09_PHASE_CHECK_T));
                    close(fd);
                    if (len <= 0){
                        ENG_LOG("phasecheck_sprd: %s read fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                        pPhase->Magic = 0;
                    }
                } else {
                    ENG_LOG("phasecheck_sprd: %s open fail PHASE_CHECKE_FILE = %s \n",__FUNCTION__ , PHASE_CHECKE_FILE);
                    pPhase->Magic = 0;
                }
                reply->write((void*)pPhase, sizeof(SP09_PHASE_CHECK_T));
                return NO_ERROR;
             } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
}

using namespace android;

int main(int arg, char** argv)
{
    ALOGE("phasecheck_sprd Nativeserver - main() begin\n");
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
    //LOGI("ServiceManager: %p\n", sm.get());
    ALOGE("phasecheck_sprd new server - serviceManager: %p\n", sm.get());
    //int ret = NativeService::Instance();
    int ret = defaultServiceManager()->addService(
                String16("phasechecknative"), new NativeService());
    ALOGE("phasecheck_sprd new ..server - NativeService::Instance return %d\n", ret);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
