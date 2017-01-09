/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WAV_EXTRACTOREX_H_

#define WAV_EXTRACTOREX_H_

#include <utils/Errors.h>
#include <media/stagefright/MediaExtractor.h>
#include "include/WAVExtractor.h"

namespace android {

struct AMessage;
class DataSource;
class String8;

class WAVExtractorEx : public WAVExtractor {
public:
    // Extractor assumes ownership of "source".
    WAVExtractorEx(const sp<DataSource> &source);

    virtual size_t countTracks();
    virtual sp<IMediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();
    virtual const char * name() { return "WAVExtractorEx"; }

protected:
    virtual ~WAVExtractorEx();

private:
    sp<DataSource> mDataSource;
    status_t mInitCheck;
    bool mValidFormat;
    uint16_t mWaveFormat;
    uint16_t mNumChannels;
    uint32_t mChannelMask;
    uint32_t mSampleRate;
    uint16_t mBlockAlign;
    uint16_t mBitsPerSample;
    off64_t mDataOffset;
    size_t mDataSize;
    sp<MetaData> mTrackMeta;

    status_t init();

    WAVExtractorEx(const WAVExtractorEx &);
    WAVExtractorEx &operator=(const WAVExtractorEx &);
};
bool SniffWAVEx(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);


}  // namespace android

#endif  // WAV_EXTRACTOR_H_

