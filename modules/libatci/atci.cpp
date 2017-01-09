/**
 * AT Command Interface Client Socket implementation
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 */

#define LOG_TAG "ATCI"

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
#include <cutils/jstring.h>
#include "atci.h"

using namespace android;

#define RIL_REQUEST_OEM_HOOK_STRINGS    60
#define HEADER_SIZE                     4
#define MAX_COMMAND_BYTES               (8 * 1024)
#define MAX_DATA_SIZE                   ((MAX_COMMAND_BYTES + HEADER_SIZE) / 4)

void writeStringToParcel(Parcel &p, const char *s) {
    char16_t *s16;
    size_t s16_len;
    s16 = strdup8to16(s, &s16_len);
    p.writeString16(s16, s16_len);
    free(s16);
}

char *strdupReadString(Parcel &p) {
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
            written = write(fd, toWrite + writeOffset,
                                len - writeOffset);
        } while (written < 0 && ((errno == EINTR) || (errno == EAGAIN)));

        if (written >= 0) {
            writeOffset += written;
        } else {   // written < 0
            RLOGE("RIL Response: unexpected error on write errno: %d, %s",
                    errno, strerror(errno));
            close(fd);
            return -1;
        }
    }
    return 0;
}

int sendRequest(const void *data, size_t dataSize, int fd) {
    int ret;
    uint32_t header;
    uint32_t buffer[MAX_DATA_SIZE];
    if (fd < 0) {
        return -1;
    }

    if (dataSize > MAX_COMMAND_BYTES) {
        RLOGE("ATCI Client: packet larger than %u (%u)",
                MAX_COMMAND_BYTES, (unsigned int)dataSize);
        close(fd);
        return -1;
    }

    header = htonl(dataSize);
    buffer[0] = header;
    memcpy(buffer + 1, data, dataSize);
    ret = blockingWrite(fd, buffer, dataSize + HEADER_SIZE);
    if (ret < 0) {
        RLOGE("ATCI Client:  blockingWrite data error");
        return ret;
    }

    return 0;
}

void recvResponse(int fd, char resp[], size_t respLen) {
    int ret;
    RecordStream *pRS = NULL;
    void *p_record;
    size_t recordlen;
    const char *err = "ERROR";

    if (fd < 0) {
        snprintf(resp, respLen, "%s", err);
        return;
    }

    pRS = record_stream_new(fd, MAX_COMMAND_BYTES);

AGAIN:
    ret = record_stream_get_next(pRS, &p_record, &recordlen);
    if (ret == 0 && p_record == NULL) {
        RLOGE("end of stream");
        goto ERROR;
    } else if (ret < 0) {
        if (errno == EAGAIN) {
            goto AGAIN;
        } else {
            goto ERROR;
        }
    } else if (ret == 0) {
        Parcel p;
        int e;
        int token, repsType;
        int numStrings;
        char **pStrings = NULL;

        p.setData((uint8_t *)p_record, recordlen);
        p.readInt32(&repsType);
        p.readInt32(&token);
        p.readInt32(&e);
        p.readInt32(&numStrings);

        if (numStrings > 0) {
            int datalen;
            datalen = sizeof(char *) * numStrings;
            pStrings = (char **)alloca(datalen);
            for (int i = 0; i < numStrings; i++) {
                pStrings[i] = strdupReadString(p);
            }
            snprintf(resp, respLen, "%s", pStrings[0]);

            if (pStrings != NULL) {
                for (int i = 0; i < numStrings; i++) {
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
    snprintf(resp, respLen, "%s", err);
}

const char *sendCmd(int phoneId, const char *atCmd) {
    int fd = -1;
    int ret;
    int mSerial = 0;
    int request = RIL_REQUEST_OEM_HOOK_STRINGS;
    int countStrings = 1;
    int retryTimes = 0;
    Parcel p;

    if (atCmd == NULL) {
        return "ERROR";
    }

    RLOGD("> AT Command '%s'. phoneId = %d.", atCmd, phoneId);

    fd = socket_local_client("atci_socket",
                    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    while (fd <= 0 && retryTimes < 10) {
        retryTimes++;
        sleep(1);
        fd = socket_local_client("atci_socket",
                ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if (fd <= 0) {
        return "ERROR";
    }

    p.writeInt32(request);
    p.writeInt32(mSerial);
    p.writeInt32(phoneId);
    p.writeInt32(countStrings);
    writeStringToParcel(p, atCmd);

    ret = sendRequest(p.data(), p.dataSize(), fd);
    if (ret < 0) {
        return "ERROR";
    }

    static char resp[MAX_COMMAND_BYTES];
    memset(resp, 0, MAX_COMMAND_BYTES);
    recvResponse(fd, resp, MAX_COMMAND_BYTES);
    RLOGD("resp = %s", resp);
    return resp;
}

int sendATCmd(int phoneId, const char *atCmd, char *resp, size_t respLen) {
    int fd = -1;
    int ret;
    int mSerial = 0;
    int request = RIL_REQUEST_OEM_HOOK_STRINGS;
    int countStrings = 1;
    int retryTimes = 0;
    Parcel p;

    if (atCmd == NULL || resp == NULL) {
        RLOGE("atCmd is NULL or resp is NULL");
        return -1;
    }

    RLOGD("sendATCmd > AT Command '%s'. phoneId = %d.", atCmd, phoneId);

    fd = socket_local_client("atci_socket",
                    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    while (fd <= 0 && retryTimes < 10) {
        retryTimes++;
        sleep(1);
        fd = socket_local_client("atci_socket",
                ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if (fd <= 0) {
        return -1;
    }

    p.writeInt32(request);
    p.writeInt32(mSerial);
    p.writeInt32(phoneId);
    p.writeInt32(countStrings);
    writeStringToParcel(p, atCmd);

    ret = sendRequest(p.data(), p.dataSize(), fd);
    if (ret < 0) {
        return -1;
    }

    recvResponse(fd, resp, respLen);
    RLOGD("resp = %s", resp);
    return 0;
}
