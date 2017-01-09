#ifndef __DM_PARSER_H
#define __DM_PARSER_H
#include <utils/String8.h>
#include <common.hpp>
#include <drm/DrmRights.h>
#include <openssl/evp.h>
#include <openssl/base64.h>
#include <stdio.h>

namespace android {
    class DmParser {
        enum {
            UNKNOWN, BASE64, BINARY
        };
    private:
        EVP_ENCODE_CTX  decodeContext;

        int rightsStartOffset;
        int rightsEndOffset;
        char* rightsData;

        char* rightsContentType;
        int dataStartOffset;
        int dataEndOffset;

        const char* dmFileName;
        FILE* file;

        bool parseOneBoundary();
        int seekToNextBoundary();

        bool firstRead;
        char* boundary;
        void seekBoundaryDelimiter();

    public:
        char* dataContentType;
        int  dataTransferEncoding;
        char* dataContentId;

        DmParser(const char* file);
        virtual ~DmParser();
        bool parse();

        bool hasRights() {
            return rightsStartOffset !=0 && rightsEndOffset != 0;
        }
        bool hasData() {
            return dataContentType != NULL && dataTransferEncoding != UNKNOWN && dataStartOffset != 0 && dataEndOffset != 0;
        }

        DrmRights getDrmRights();
        int read(char* buffer, int numBytes);
    };
};
#endif  // __DM_PARSER_H
