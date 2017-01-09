#include <UUID.hpp>
#include <common.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace android;

extern "C" {
    ssize_t read(int, void*, size_t);
    int close(int);
};

char* UUID::generate() {
    int fd = open("/proc/sys/kernel/random/uuid", O_RDONLY);
    if (fd == -1) {
        return FAKE_KEY;
    }
    // ee006a1b-18aa-481f-8501-1f645485baaa
    char uuidString[36] = {0};
    ssize_t ret = read(fd, uuidString, 36);
    if (ret != 36) {
        close(fd);
        return FAKE_KEY;
    }
    close(fd);

    int16_t uuidNumber[8] = {0};
    ret = sscanf(uuidString,"%4x%4x-%4x-%4x-%4x%4x%4x%4x",
                (int*)&uuidNumber[0], (int*)&uuidNumber[1],
                (int*)&uuidNumber[2], (int*)&uuidNumber[3],
                (int*)&uuidNumber[4], (int*)&uuidNumber[5],
                (int*)&uuidNumber[6], (int*)&uuidNumber[7]);

    if (ret != 8) {
        return FAKE_KEY;
    }
    return base64((char*)uuidNumber, 16);
}
