#ifndef DCFCREATOR_HPP
#define DCFCREATOR_HPP

#include <utils/String8.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <common.hpp>

namespace android {
    class DcfCreator {

    friend class DrmOmaPlugIn;
    private:
        EVP_CIPHER_CTX* encryptContext;
        char* key;
        // unsigned char key[16], iv[16];
        off64_t dataLenOffset;
        off64_t dataStartOffset;
        int fd;

        int dataLength;

        bool writeHeaders();
        bool setKey(char* key);
    public:
        DcfCreator(char* file, char* key, char* contentType, char* contentUri = FAKE_UID);
        ~DcfCreator();

        int version;
        char* contentType;
        char* contentUri;

        char* encryptionMethod;

        char* rightsIssuer;
        char* contentName;
        char* contentDescription;
        char* contentVendor;
        String8 headers;

        ssize_t write(char* buffer, ssize_t numBytes);
        bool save();
        char* convert(char* buffer, ssize_t bytesReaded, ssize_t* bytesConverted);
        ssize_t convertend(char* buff);
        bool convertHeaders();
    };
};

#endif // DCFCREATOR_HPP
