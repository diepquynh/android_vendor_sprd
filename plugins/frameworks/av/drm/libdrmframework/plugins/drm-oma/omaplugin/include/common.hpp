#ifndef __COMMON_H
#define __COMMON_H

#ifdef __LP64__
#include <time.h>
typedef time_t time64_t;
#define mktime64(t)(mktime(t))
#else
#include <time64.h>
#endif


//#define USE_LAGACY
#define MIMETYPE_DCF                    "application/vnd.oma.drm.content"
#define MIMETYPE_DM                     "application/vnd.oma.drm.message"
#define MIMETYPE_RO                     "application/vnd.oma.drm.rights+xml"
#define MIMETYPE_WB_RO                  "application/vnd.oma.drm.rights+wbxml"

#define RIGHTS_DB                       "/data/drm/rights.db"
#define FAKE_UID                        "sprd_fl_fake_rights"
#define FAKE_KEY                        "MDEyMzQ1Njc4OTAxMjM0NQ=="
#define FAKE_IV                         "0123456789012345"
#define FL_PREFIX                       "sprd_fl_uid_"

#define KEY_REQUEST_FILE_IN             "file_in"
#define KEY_REQUEST_FILE_OUT            "file_out"
#define KEY_CONVERT_ID                  "convert_id"

#define KEY_RIGHTS_ISSUER               "rights_issuer"
#define KEY_CONTENT_VENDOR              "content_vendor"
#define KEY_CONTENT_NAME                "content_name"
#define KEY_EXTENDED_DATA               "extended_data"

#define MAX_LINE                        1024
#define DM_BOUNDARY                     "--boundary-1"

#define SUNWAY_NOISY(x) x

#define RO_ELEMENT_RIGHTS               "o-ex:rights"
#define RO_ELEMENT_CONTEXT              "o-ex:context"
#define RO_ELEMENT_VERSION              "o-dd:version"
#define RO_ELEMENT_AGREEMENT            "o-ex:agreement"
#define RO_ELEMENT_ASSET                "o-ex:asset"
#define RO_ELEMENT_UID                  "o-dd:uid"
#define RO_ELEMENT_KEYINFO              "ds:KeyInfo"
#define RO_ELEMENT_KEYVALUE             "ds:KeyValue"
#define RO_ELEMENT_PERMISSION           "o-ex:permission"

#define RO_ELEMENT_DISPLAY              "o-dd:display"
#define RO_ELEMENT_PLAY                 "o-dd:play"
#define RO_ELEMENT_EXECUTE              "o-dd:execute"
#define RO_ELEMENT_PRINT                "o-dd:print"

#define RO_ELEMENT_CONSTRAINT           "o-ex:constraint"
#define RO_ELEMENT_COUNT                "o-dd:count"
#define RO_ELEMENT_DATE_TIME            "o-dd:datetime"
#define RO_ELEMENT_START_TIME           "o-dd:start"
#define RO_ELEMENT_EXPIRY_TIME          "o-dd:end"
#define RO_ELEMENT_AVAILABLE_TIME       "o-dd:interval"
#define RO_ELEMENT_NIL                  "nil"

#define HTTP_CONTENT_TYPE               "Content-type"
#define HTTP_CONTENT_TYPE1              "Content-Type"
#define HTTP_CONTENT_ID                 "Content-ID"
#define HTTP_CONTENT_TRANSFER_ENCODING  "Content-Transfer-Encoding"

#define ASSERT_XML_EQUAL(x,y)  do {if (String8(x) != String8(y)) return false;} while (0)
#define ASSERT_XML_NOT_NULL(x)  do {if (x == NULL) return false;} while (0)

#define TIME_INVALID                   -1

#define ACTION_NUM                     8
#define CONSTRAINT_NUM                 5

#define CONVERT_SIZE                   4096

#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/base64.h>
#include <string.h>
#include <utils/String8.h>
#include <utils/Log.h>

namespace android {
    extern time64_t ntp_property_get();

    inline char* chomp(char * line) {
        int len = strlen(line);
        for (int i=0;i<len;++i) {
            if (line[i] == '\r' || line[i] == '\n') {
                line[i] = 0;
                break;
            }
        }
        return line;
    }
    inline unsigned char* unBase64(unsigned char* input, int* out) {
        EVP_ENCODE_CTX  ctx;
        int length = strlen((const char*)input);
        unsigned char* ret = (unsigned char*)malloc(length);
        int result, tmpLen;
        EVP_DecodeInit(&ctx);
        EVP_DecodeUpdate(&ctx, (unsigned char *)ret, &result, (unsigned char *)input, length);
        EVP_DecodeFinal(&ctx, (unsigned char *)&ret[result], &tmpLen);
        result += tmpLen;
        *out = result;
        return ret;
    }
    inline char* base64(char* input, int length) {
        EVP_ENCODE_CTX  ctx;
        char* ret = (char*)malloc(2*length);
        bzero(ret, 2*length);
        int result, tmpLen;
        EVP_EncodeInit(&ctx);
        EVP_EncodeUpdate(&ctx, (unsigned char *)ret, &result, (unsigned char *)input, length);
        EVP_EncodeFinal(&ctx, (unsigned char *)&ret[result], &tmpLen);
        return ret;
    }

    inline time64_t getCurrentTime() {
        // return NTP::getStandardTime();
        time64_t delta = 0;
        delta = ntp_property_get();
        ALOGD("drm_ntp, getStandardTime, delta: %lld.", delta);
        return time(NULL)+delta;
    }

};

#define DCF_ENCRYTION_METHOD            "Encryption-Method"
#define DCF_CONTENT_NAME                "Content-Name"
#define DCF_RIGHTS_ISSUER               "Rights-Issuer"
#define DCF_CONTENT_DESCRIPTION         "Content-Description"
#define DCF_CONTENT_VENDOR              "Content-Vendor"
#define DCF_UNKNOWN_CONTENT_TYPE        "application/unknown"
#define DCF_ICON_URI                    "Icon-Uri"

#define AES_BLOCK_SIZE                  128

#define DCF_DEFAULT_ENCRYPTION_METHOD   "AES128CBC"
#endif
