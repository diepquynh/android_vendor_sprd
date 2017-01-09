/* //vendor/sprd/proprietories-source/ril/sprd_libatci/sprd_atci.cpp
 *
 * AT Command Interface Client Socket implementation
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <alloca.h>
#include <cutils/sockets.h>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <arpa/inet.h>
#include <telephony/record_stream.h>
#include "sprd_atci.h"
#include <cutils/jstring.h>

using namespace android;

#define LOG_TAG "RILC_ATCI"
#define RIL_REQUEST_SEND_CMD  1
#define MAX_COMMAND_BYTES (8 * 1024)

void writeStringToParcel(Parcel &p, const char *s) {
    char16_t *s16;
    size_t s16_len;
    s16 = strdup8to16(s, &s16_len);
    p.writeString16(s16, s16_len);
    free(s16);
}

char * strdupReadString(Parcel &p) {
    size_t stringlen;
    const char16_t *s16;

    s16 = p.readString16Inplace(&stringlen);

    return strndup16to8(s16, stringlen);
}

int blockingWrite(int fd, const void *buffer, size_t len) {
    size_t writeOffset = 0;
    const uint8_t *toWrite;

    toWrite = (const uint8_t *)buffer;

    while (writeOffset < len) {
        ssize_t written;
        do {
            written = write (fd, toWrite + writeOffset,
                                len - writeOffset);
        } while (written < 0 && ((errno == EINTR) || (errno == EAGAIN)));

        if (written >= 0) {
            writeOffset += written;
        } else {   // written < 0
            ALOGE ("RIL Response: unexpected error on write errno:%d, %s", errno, strerror(errno));
            close(fd);
            return -1;
        }
    }
    return 0;
}

int sendRequest (const void *data, size_t dataSize, int fd) {
    int ret;
    uint32_t header;

    if (fd < 0)
        return -1;

    if (dataSize > MAX_COMMAND_BYTES) {
        ALOGE("ATCI Client: packet larger than %u (%u)",
                MAX_COMMAND_BYTES, (unsigned int )dataSize);
        close(fd);
        return -1;
    }

    header = htonl(dataSize);
    ret = blockingWrite(fd, (void *)&header, sizeof(header));
    if (ret < 0) {
        ALOGE("ATCI Client:  blockingWrite header error");
        return ret;
    }

    ret = blockingWrite(fd, data, dataSize);
    if (ret < 0) {
        ALOGE("ATCI Client:  blockingWrite data error");
        return ret;
    }
    return 0;
}

void recvResponse (int fd, char data[]) {
    int ret;
    RecordStream *pRS;
    void *p_record;
    size_t recordlen;
    char *err = "ERROR";

    if (fd < 0) {
        memcpy(data, err, strlen(err)+1);
        return;
    }

    pRS = record_stream_new(fd, MAX_COMMAND_BYTES);

AGAIN:
    ret = record_stream_get_next(pRS, &p_record, &recordlen);
    if (ret == 0 && p_record == NULL) {
        ALOGE("end of stream");
        goto ERROR;
    } else if (ret < 0) {
        if (errno == EAGAIN)
            goto AGAIN;
        else
            goto ERROR;
    } else if (ret == 0) {
        Parcel p;
        int e;
        int token;
        int numStrings;
        char **pStrings = NULL;

        p.setData((uint8_t *) p_record, recordlen);
        p.readInt32(&token);
        p.readInt32(&e);
        p.readInt32 (&numStrings);

        if (numStrings > 0) {
            int datalen;
            datalen = sizeof(char *) * numStrings;
            pStrings = (char **)alloca(datalen);
            for (int i = 0 ; i < numStrings ; i++) {
                pStrings[i] = strdupReadString(p);
            }
            memcpy(data, pStrings[0], strlen(pStrings[0])+1);

            if (pStrings != NULL) {
                for (int i = 0 ; i < numStrings ; i++) {
                    free(pStrings[i]);
                }
            }
            close(fd);
            record_stream_free(pRS);
            return;
        }
    }

ERROR:
    close(fd);
    record_stream_free(pRS);
    memcpy(data, err, strlen(err)+1);
}

const char* sendCmd(int phoneId, const char* atCmd)
{
    int fd = -1;
    int ret;
    int mSerial = 0;
    int request = RIL_REQUEST_SEND_CMD;
    int retry_times = 0;
    char s_name_socket[128] = {0};
    char resp[MAX_COMMAND_BYTES] = {0};

    if (atCmd == NULL)
        return "ERROR";

    ALOGD("> AT Command '%s'. phoneId = %d.", atCmd, phoneId);

    snprintf(s_name_socket, sizeof(s_name_socket), "atci_socket%d", phoneId+1);
    fd = socket_local_client(s_name_socket,
                    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    while (fd <= 0 && retry_times < 10) {
        retry_times ++;
        sleep(1);
        fd = socket_local_client(s_name_socket,
                ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if (fd <= 0)
        return "ERROR";

    Parcel p;
    p.writeInt32(mSerial);
    p.writeInt32(request);
    writeStringToParcel(p, atCmd);

    ret = sendRequest(p.data(), p.dataSize(), fd);
    if (ret < 0)
        return "ERROR";

    recvResponse(fd, resp);
    ALOGD("resp = %s", resp);
    return resp;
}

