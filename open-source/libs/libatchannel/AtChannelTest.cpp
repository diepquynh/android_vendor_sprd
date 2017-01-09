#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#if 1
#define ALOGI(x...)  fprintf(stderr, "AtChannelTest: " x)
#define ALOGE(x...)  fprintf(stderr, "AtChannelTest: " x)
#else
#define LOG_TAG "AtChannelTest"
#include <cutils/log.h>
#endif

#include "AtChannel.h"

int main(int argc, char **argv)
{
    const char* atTestCmd = "AT";
    const char* atrsp = NULL;

    atrsp = sendAt(0, 0, atTestCmd);

    if (atrsp == NULL) {
        ALOGE("Send %s, but no response\n", atTestCmd);
    } else {
        ALOGI("The response for the AT command \"%s\" is %s \n", atTestCmd, atrsp);
        if (strstr(atrsp, "OK")) {
            ALOGI("ATChannel is OK!\n");
        } else {
            ALOGE("ATChannel is wrong!\n");
        }
    }

    return 0;
}
