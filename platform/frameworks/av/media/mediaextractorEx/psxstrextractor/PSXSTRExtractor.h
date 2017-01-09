/*
 * Sony Playstation (PSX) STR File Demuxer
 *
 * PSX STR file demuxer
 *
 * TODO: Implement this demux in the future
 */

#ifndef PSX_EXTRACTOR_H_

#define PSX_EXTRACTOR_H_

//#define LOG_NDEBUG 0
//#define LOG_TAG "PSXSTRExtractor"
//#include <utils/Log.h>

namespace android {

bool SniffPSXSTR(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {

    uint8_t header[12];
    if (source->readAt(0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return false;
    }

    if (memcmp("\x52\x49\x46\x46", &header[0], 4) ||
            memcmp("\x43\x44\x58\x41", &header[8], 4)) {
        return false;
    }

   // ALOGW("This is PSX STR format which is not supported so far");


    *confidence = 0.21f;  //Slightly larger than .mp3 extractor confidence

    mimeType->setTo("video/psxstr");
    return true;

}

}  //namespace android

#endif
