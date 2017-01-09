#include <DcfCreator.hpp>
#include <utils/Log.h>

using namespace android;

#ifdef USE_LEGACY
DcfCreator::DcfCreator(char* file, char* transKey, char* contentType, char* contentUri)
    : dataStartOffset(0), dataLenOffset(0), fd(-1)
    , version(1), contentType(contentType), contentUri(contentUri), contentName(NULL), contentDescription(NULL)
    , contentVendor(NULL), dataLength(0),rightsIssuer(NULL), encryptContext(NULL), key(NULL) {
    if ((fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0777)) == -1) {
        SUNWAY_NOISY (ALOGE("DcfCreator::DcfCreator: cant open file %s, %s",file, strerror(errno)));
    }

    encryptionMethod = DCF_DEFAULT_ENCRYPTION_METHOD;

    if (!setKey(transKey) && encryptContext) {
        delete encryptContext;
        encryptContext = NULL;
    }

}

DcfCreator::~DcfCreator() {
    if (fd != -1) {
        close(fd);
    }
    if (key) {
        free(key);
        key = NULL;
    }
    if (encryptContext) {
        EVP_CIPHER_CTX_cleanup(encryptContext);
        delete encryptContext;
        encryptContext = NULL;
    }
}
#else
DcfCreator::DcfCreator(char* file, char* transKey, char* contentType, char* contentUri)
    : dataStartOffset(0), dataLenOffset(0), fd(-1)
    , version(1), contentType(contentType), contentUri(contentUri), contentName(NULL), contentDescription(NULL)
    , contentVendor(NULL), dataLength(0),rightsIssuer(NULL), encryptContext(NULL), key(NULL) {
    encryptionMethod = DCF_DEFAULT_ENCRYPTION_METHOD;

    if (!setKey(transKey) && encryptContext) {
        delete encryptContext;
        encryptContext = NULL;
    }

}

DcfCreator::~DcfCreator() {
    if (key) {
        free(key);
        key = NULL;
    }
    if (encryptContext) {
        EVP_CIPHER_CTX_cleanup(encryptContext);
        delete encryptContext;
        encryptContext = NULL;
    }
}
#endif

bool DcfCreator::setKey(char* transKey) {
    // key is encoded in base64
    int origKeyLength = 0;
    unsigned char* origKey = unBase64((unsigned char*)transKey, &origKeyLength);
    SUNWAY_NOISY (ALOGD("DcfCreator::setKey: trans key: %s",transKey));
    SUNWAY_NOISY (ALOGD("DcfCreator::setKey: origKey %s, origKeyLen: %d",origKey, origKeyLength));

    if (key) {
        free(key);
    }
    key = (char*)malloc(origKeyLength);
    memcpy(key, origKey, origKeyLength);
    free(origKey);
    return true;
}

ssize_t DcfCreator::write(char* buffer, ssize_t numBytes) {
    if (fd == -1) {
        return -1;
    }
    if (dataStartOffset == 0) {
        // calcuate the date offset;
        if (! writeHeaders()) {
            return -1;
        }

        encryptContext =new EVP_CIPHER_CTX();
        EVP_CIPHER_CTX_init(encryptContext);
        EVP_EncryptInit_ex(encryptContext, EVP_aes_128_cbc(), NULL, (unsigned char*)key, (unsigned char*)FAKE_IV);

        // write iv
        ::write(fd, FAKE_IV, 16);
    }

    int cLen = numBytes+AES_BLOCK_SIZE-1;
    unsigned char cipherText[cLen];
    bzero(cipherText,cLen);
    EVP_EncryptUpdate(encryptContext, cipherText, &cLen, (unsigned char*)buffer, numBytes);
    ::write(fd, cipherText, cLen);
    return cLen;
}

char* DcfCreator::convert(char* buffer, ssize_t bytesReaded, ssize_t* bytesConverted) {
        char* cipherText = 0;
    if (!encryptContext) {
        encryptContext = new EVP_CIPHER_CTX();
        EVP_CIPHER_CTX_init(encryptContext);
        EVP_EncryptInit_ex(encryptContext, EVP_aes_128_cbc(), NULL, (unsigned char*)key, (unsigned char*)FAKE_IV);

        int cLen = bytesReaded+AES_BLOCK_SIZE-1;
        cipherText = new char[cLen+16];
        bzero(cipherText, cLen+16);
        // write iv
        memcpy(cipherText, FAKE_IV, 16);
        EVP_EncryptUpdate(encryptContext, (unsigned char*)(cipherText+16), &cLen, (unsigned char*)buffer, bytesReaded);
        *bytesConverted = (cLen+16);
    } else {
        int cLen = bytesReaded+AES_BLOCK_SIZE-1;
        cipherText = new char[cLen];
        bzero(cipherText, cLen);
        EVP_EncryptUpdate(encryptContext, (unsigned char*)cipherText, &cLen, (unsigned char*)buffer, bytesReaded);
        *bytesConverted = cLen;
        }

    return cipherText;
}

ssize_t DcfCreator::convertend(char* buff) {
    int cLen = 0;
    if (encryptContext) {
        cLen = AES_BLOCK_SIZE;
        char cipherText[cLen];
        bzero(cipherText, cLen);
        EVP_EncryptFinal_ex(encryptContext, (unsigned char*)cipherText, &cLen);
        memcpy(buff, cipherText, cLen);
    }

    return cLen;
}

bool DcfCreator::save() {
    if (fd == -1) {
        return false;
    }
    int cLen = AES_BLOCK_SIZE;
    unsigned char cipherText[cLen];
    bzero(cipherText, cLen);
    EVP_EncryptFinal_ex(encryptContext, cipherText, &cLen);
    ::write(fd, cipherText, cLen);

    int dataEndOffset = lseek(fd, 0, SEEK_CUR);
    int dataLength = dataEndOffset - dataStartOffset;
    lseek(fd, dataLenOffset, SEEK_SET);

    // uintvar can be encoded into 5 octets at most
    for (int i=4;i>=0;--i) {
        // ssize_t is typically unsigned, thus logical right shift
        // would work
        char tmp = (dataLength >> (i*7)) & 0x7f;
        if (i != 0) {
            tmp |= 0x80;
        }
        ::write(fd, &tmp, 1);
    }
    return true;
}

bool DcfCreator::writeHeaders() {
    if (fd == -1) {
        return false;
    }

    // version
    ::write(fd, &version, 1);
    // content type len
    if (! contentType) {
        SUNWAY_NOISY (ALOGE("DcfCreator::contentType unset"));
        return false;
    }
    int contentTypeLen = strlen(contentType);
    ::write(fd, &contentTypeLen, 1);
    // content uri len
    if (! contentUri) {
        SUNWAY_NOISY (ALOGE("DcfCreator::contentUri unset"));
        return false;
    }
    int contentUriLen = strlen(contentUri)+2;
    ::write(fd, &contentUriLen, 1);
    // content type
    ::write(fd, contentType, contentTypeLen);
    // content uri
    ::write(fd, "<",1);
    ::write(fd, contentUri, contentUriLen-2);
    ::write(fd, ">",1);
    // headers len (uintvar)
    String8 headers;
    if (! encryptionMethod) {
        SUNWAY_NOISY (ALOGE("DcfCreator::encryptionMethod unset"));
        return false;
    }
    headers.appendFormat(DCF_ENCRYTION_METHOD": %s\r\n", encryptionMethod);

    if (rightsIssuer) {
        headers.appendFormat(DCF_RIGHTS_ISSUER": %s\r\n", rightsIssuer);
    }

    if (contentName) {
        headers.appendFormat(DCF_CONTENT_NAME": %s\r\n", contentName);
    }

    if (contentDescription) {
        headers.appendFormat(DCF_CONTENT_DESCRIPTION": %s\r\n", contentDescription);
    }

    if (contentVendor) {
        headers.appendFormat(DCF_CONTENT_VENDOR": %s\r\n", contentVendor);
    }

    ssize_t headersLen = headers.length();

    SUNWAY_NOISY (ALOGD("DcfCreator::writeHeaders: headers: %s, headersLen: %d",headers.string(), headersLen));
    // uintvar can be encoded into 5 octets at most
    for (int i=4;i>=0;--i) {
        // ssize_t is typically unsigned, thus logical right shift
        // would work
        char tmp = (headersLen >> (i*7)) & 0x7f;
        if (i != 0) {
            tmp |= 0x80;
        }
        ::write(fd, &tmp, 1);
    }

    // placeholder for data len
    dataLenOffset = lseek(fd,0,SEEK_CUR);
    lseek(fd, 5, SEEK_CUR);
    // headers
    ::write(fd, headers.string(), headers.length());
    dataStartOffset = lseek(fd,0,SEEK_CUR);
    return true;
}

bool DcfCreator::convertHeaders() {
    // version
        headers.append((const char*)&version, 1);
    // content type len
    if (! contentType) {
        SUNWAY_NOISY (ALOGE("DcfCreator::contentType unset"));
        return false;
    }
    int contentTypeLen = strlen(contentType);
    headers.append((const char*)&contentTypeLen, 1);
    // content uri len
    if (! contentUri) {
        SUNWAY_NOISY (ALOGE("DcfCreator::contentUri unset"));
        return false;
    }
    int contentUriLen = strlen(contentUri)+2;
    headers.append((const char*)&contentUriLen, 1);
    // content type
    headers.append((const char*)contentType, contentTypeLen);
    // content uri
    headers.appendFormat("<%s>", contentUri);

    String8 head;
    // headers len (uintvar)
    if (! encryptionMethod) {
        SUNWAY_NOISY (ALOGE("DcfCreator::encryptionMethod unset"));
        return false;
    }
    head.appendFormat(DCF_ENCRYTION_METHOD": %s\r\n", encryptionMethod);

    if (rightsIssuer) {
        head.appendFormat(DCF_RIGHTS_ISSUER": %s\r\n", rightsIssuer);
    }

    if (contentName) {
        head.appendFormat(DCF_CONTENT_NAME": %s\r\n", contentName);
    }

    if (contentDescription) {
        head.appendFormat(DCF_CONTENT_DESCRIPTION": %s\r\n", contentDescription);
    }

    if (contentVendor) {
        head.appendFormat(DCF_CONTENT_VENDOR": %s\r\n", contentVendor);
    }

    ssize_t headersLen = head.length();

    SUNWAY_NOISY (ALOGD("DcfCreator::convertHeaders: headers: %s, headersLen: %d",head.string(), headersLen));
    // uintvar can be encoded into 5 octets at most
    for (int i=4; i>=0; --i) {
        // ssize_t is typically unsigned, thus logical right shift
        // would work
        char tmp = (headersLen >> (i*7)) & 0x7f;
        if (i != 0) {
            tmp |= 0x80;
        }
        headers.append((const char*)&tmp, 1);
    }

    // length of the data section, no use for now, filled with zero
    dataLenOffset = headers.size();
    char tmp[] = {0x80, 0x80, 0x80, 0x80, 0x00};
    headers.append(tmp, 5);
    // headers
    headers.append(head);
    dataStartOffset = headers.size();
    return true;
}

