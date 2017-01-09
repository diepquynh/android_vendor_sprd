#ifndef __DCF_PARSER_H__
#define __DCF_PARSER_H__
#include <utils/String8.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <common.hpp>
#include <drm/DrmMetadata.h>

namespace android {
    #define PARSE_ERR_MAX                     5
    #define PARSER_CONTENT_TYPE_MAX           128
    #define PARSER_CONTENT_URI_MAX            512
    #define PARSER_HEADER_MAX                 1024

    class DcfParser {

    public:
        DcfParser(int fd);
        DcfParser(const char* file);
        DcfParser(const char* file, int fd);
        virtual ~DcfParser();
        bool parse();
        friend class DrmOmaPlugIn;

    private:
        bool firstRead;
        bool eof;
        int fd;
        int offset;
        int length;
        const char* dcfFileName;
        bool parseHeader(const char*, int);
        EVP_CIPHER_CTX* decryptContext;
        unsigned char key[16];
        int mDefaultAct;

        int readBlock(char* buffer, int firstBlock, int numBlocks);
    public:
        int version;
        String8 contentType;
        String8 contentUri;
        int dataLength;
        String8 encryptionMethod;
        String8 rightsIssuer;
        String8 contentName;
        String8 contentDescription;
        String8 contentVendor;

        bool setKey(char* key);
        ssize_t readAt(char* buffer, ssize_t numBytes, int offset);
        ssize_t read(char* buffer, ssize_t numBytes);
        DrmMetadata* getMetadata();
    };
};
#endif /* __DCF_PARSER_H__ */
