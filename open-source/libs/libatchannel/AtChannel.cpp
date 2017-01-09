#define LOG_TAG "IAtChannel"

#include <android/log.h>
#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include "IAtChannel.h"
#include "AtChannel.h"
#include <cutils/properties.h>

using namespace android;

const int MAX_SERVICE_NAME = 100;

#define  MSMS_PHONE_COUNT_PROP             "persist.msms.phone_count"
#define  MODEM_TD_ENABLE_PROP              "persist.modem.t.enable"
#define  MODEM_TD_ID_PROP                  "ro.modem.t.id"
#define  MODEM_TD_COUNT_PROP               "ro.modem.t.count"
#define  MODEM_WCDMA_ENABLE_PROP           "persist.modem.w.enable"
#define  MODEM_WCDMA_ID_PROP               "ro.modem.w.id"
#define  MODEM_WCDMA_COUNT_PROP            "ro.modem.w.count"
#define  MODEM_TL_ENABLE_PROP              "persist.modem.tl.enable"
#define  MODEM_TL_COUNT_PROP               "ro.modem.tl.count"
#define  MODEM_LF_ENABLE_PROP              "persist.modem.lf.enable"
#define  MODEM_LF_COUNT_PROP               "ro.modem.lf.count"

static int getPhoneId(int modemId, int simId)
{
    /*
     * Because there is no docs, LTE should be supported later.
     */
    char prop[PROPERTY_VALUE_MAX] = "";
    int tdEnable, wEnable,tlEnable,lfEnable;
    int tdCount,  wCount,tlCount,lfCount;

    property_get(MODEM_TD_ENABLE_PROP, prop, "0");
    tdEnable = atoi(prop);
    memset(prop, '\0', sizeof(prop));
    property_get(MODEM_WCDMA_ENABLE_PROP, prop, "0");
    wEnable = atoi(prop);
    property_get(MODEM_TL_ENABLE_PROP, prop, "0");
    tlEnable = atoi(prop);
    memset(prop, '\0', sizeof(prop));
    property_get(MODEM_LF_ENABLE_PROP,prop, "0");
    lfEnable = atoi(prop);


    memset(prop, '\0', sizeof(prop));
    property_get(MODEM_TD_COUNT_PROP, prop, "0");
    tdCount = atoi(prop);
    memset(prop, '\0', sizeof(prop));
    property_get(MODEM_WCDMA_COUNT_PROP, prop, "0");
    wCount = atoi(prop);
    memset(prop, '\0', sizeof(prop));
    property_get(MODEM_TL_COUNT_PROP, prop, "0");
    tlCount = atoi(prop);
    memset(prop, '\0', sizeof(prop));
    property_get(MODEM_LF_COUNT_PROP, prop, "0");
    lfCount = atoi(prop);


    if (tdEnable) {
        if (wEnable) {
            return modemId * tdCount + simId;
        } else {
            return simId;
        }
    } else if(wEnable) {
           return simId;
    } else if(tlEnable) {
           return simId;
    } else {
        if(lfEnable) {
            return simId;
        } else {
            ALOGE("Both TD modem and WCDMA modem are disable, use default phoneID\n");
        }
    }

    return 0;
}

static String16 getServiceName(int modemId, int simId)
{
    char serviceName[MAX_SERVICE_NAME];
    char phoneCount[PROPERTY_VALUE_MAX] = "";

    memset(serviceName, 0, sizeof(serviceName));
    property_get(MSMS_PHONE_COUNT_PROP, phoneCount, "1");

    if (atoi(phoneCount) > 1){
        snprintf(serviceName, MAX_SERVICE_NAME - 1, "atchannel%d", simId);
    } else {
        strcpy(serviceName,  "atchannel");
    }

    return String16(serviceName);
}

const char* sendAt(int modemId, int simId, const char* atCmd)
{

    sp < IServiceManager > sm = NULL;
    sp<IAtChannel> atChannel = NULL;
    String16 serviceName;
    do {
        sm = defaultServiceManager();
        if (sm == NULL) {
            ALOGE("Couldn't get default ServiceManager\n");
            continue;
        }
        serviceName = getServiceName(modemId, simId);
        atChannel = interface_cast < IAtChannel > (sm->getService(serviceName));
    } while (atChannel == NULL);

    return atChannel->sendAt(atCmd);
}
