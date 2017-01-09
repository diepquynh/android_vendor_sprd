/**
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef FLV_EXTRACTOR_H_

#define FLV_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>

namespace android {

#define SIZE_OF_TAG_HEAD    11

#define AV_TIME_BASE        1000000

#define AMF_END_OF_OBJECT       0x09

#define FLV_AUDIO_CODECID_MASK    0xf0    //--bit[7:4]
#define FLV_AUDIO_CODECID_OFFSET    4
#define FLV_AUDIO_RATE_MASK    0x0c    //--bit[3:2]
#define FLV_AUDIO_RATE_OFFSET    0x02
#define FLV_AUDIO_SIZE_MASK    0x02    //--bit1
#define FLV_AUDIO_CHANNEL_MASK    0x01    //--bit0

#define FLV_VIDEO_FRAMETYPE_MASK    0xf0
#define FLV_VIDEO_FRAMETYPE_OFFSET    4

#define FLV_MOVIE_TIMESCALE 1000000

enum {
    FLV_HEADER_FLAG_HASVIDEO = 1,
    FLV_HEADER_FLAG_HASAUDIO = 4,
};

enum {
    FLV_TAG_TYPE_AUDIO = 0x08,
    FLV_TAG_TYPE_VIDEO = 0x09,
    FLV_TAG_TYPE_META  = 0x12,
};

enum {
    FLV_CODECID_PCM_BE              = 0,
    FLV_CODECID_ADPCM               = 1 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_MP3                 = 2 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_PCM_LE              = 3 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_8HZ_MONO = 5 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER          = 6 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_AAC                         = 10 << FLV_AUDIO_CODECID_OFFSET,
};

enum{
    FLV_AUDIO_RATE_5500     = 0,
    FLV_AUDIO_RATE_11000    = 1 << FLV_AUDIO_RATE_OFFSET,
    FLV_AUDIO_RATE_22000    = 2 << FLV_AUDIO_RATE_OFFSET,
    FLV_AUDIO_RATE_44000    = 3 << FLV_AUDIO_RATE_OFFSET,
};

enum {
    FLV_CODECID_H263    = 2,
    FLV_CODECID_SCREEN  = 3,
    FLV_CODECID_VP6     = 4,
    FLV_CODECID_VP6A    = 5,
    FLV_CODECID_SCREEN2 = 6,
    FLV_CODECID_AVC = 7
};

enum {
    FLV_FRAME_KEY        = 1 << FLV_VIDEO_FRAMETYPE_OFFSET,
    FLV_FRAME_INTER      = 2 << FLV_VIDEO_FRAMETYPE_OFFSET,
    FLV_FRAME_DISP_INTER = 3 << FLV_VIDEO_FRAMETYPE_OFFSET,
};

typedef enum {
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY  = 0x08,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
} AMFDataType;

typedef struct FLV_TAG_INFORMATION_TAG
{
    uint32_t tag_type;      //--type of cur-packet.
    uint32_t time_stamp;    //--time stamp of cur-packet in ms.
    uint32_t data_size;     //--size of the data.
    uint32_t cur_pos;       //--pos of current packet in file.
    uint32_t next_pos;      //--pos of next packet in file.
    uint32_t pre_pos;       //--pos of previous packet in file.

    union TAG_DATA_HEADER_TAG
    {
        unsigned char data_head;
        unsigned char audio_format;     //audio data head.
        unsigned char video_format;     //video data head.
        unsigned char object_name_type; //object data head.
        uint32_t reserve4B;
    }data_header;

}FLV_TAG_INFO_T;

// Keyframe Link List
typedef struct TKeyFrameLinkListItem_TAG
{
    uint32_t pos;    //position
    uint32_t timestamp;//double timestamp;
    struct TKeyFrameLinkListItem_TAG* next;
}TKeyFrameLinkListItem_T;

struct TKeyFrameEntry
{
    uint32_t pos;   //position
    uint32_t timestamp;//double timestamp;
};

//for broken files, cannot get tagsize
/** SPRD: modify { */
//#define FLV_VIDEO_TAG_MAX_SIZE (512<<10)
#define FLV_VIDEO_TAG_MAX_SIZE (1024<<10)
/** SPRD: modify } */
#define FLV_AUDIO_TAG_MAX_SIZE (12 <<10)

#define FLV_SEEK_MAX_ENTRY 1024
struct FLVExtractor : public MediaExtractor {
    FLVExtractor(const sp<DataSource> &dataSource);

    virtual size_t countTracks();

    virtual sp<IMediaSource> getTrack(size_t index);

    virtual sp<MetaData> getTrackMetaData(
            size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

    virtual uint32_t flags() const;

protected:
    virtual ~FLVExtractor();

private:
    struct FLVSource;
    struct AudioSource;

    struct TagInfo {
        uint32_t mOffset;
        bool mIsKey;
    };

    struct Track {
        sp<MetaData> mMeta;
        Vector<TagInfo> mTags;
        uint32_t mRate;
        uint32_t mScale;

        // If bytes per tag == 0, each chunk represents a single tag,
        // otherwise each chunk should me a multiple of bytes-per-tag in
        // size.
        uint32_t mBytesPerTag;

        enum Kind {
            AUDIO,
            VIDEO,
            OTHER
        } mKind;

        size_t mNumSyncTags;
        size_t mThumbnailTagSize;
        ssize_t mThumbnailTagIndex;
        size_t mMaxTagSize;

        // if no index
        uint32_t mCurTagPos;
        //for replay seek 0 with can not seek
        uint32_t mInitTagPos;
        // If mBytesPertag > 0:
        double mAvgTagSize;
        size_t mFirstTagSize;
    };

    sp<DataSource> mDataSource;
    status_t mInitCheck;
    Vector<Track> mTracks;

    off64_t mMovieOffset;
    bool mFoundIndex;
    bool mOffsetsAreAbsolute;
    TKeyFrameEntry *mKeyFrameEntries;
    uint32_t mKeyFrameNum;
    int64_t mCurrentTimeUs;
    bool mIsMetadataPresent;
    bool mIsKeyframesPresent;
    int64_t mLastseekTimeUs;

    void setInitTagPos(size_t trackIndex);
    status_t parseHeaders();
    status_t parseTagHeaders(off64_t offset, off64_t size);
    status_t parseTag(off_t offset, off64_t size);
    ssize_t flv_read_metabody(off_t offset);
    ssize_t amf_get_string(uint32_t offset, uint8_t *buffer, int32_t buffsize);
    ssize_t amf_parse_object(const char *key, uint32_t  offset, int depth);

    double amf_get_doublenum(uint32_t offset);
    status_t alloc_keyframe_entry(uint32_t array_num);
    status_t getKeyFramePosition(size_t trackIndex,int64_t seekTimeUs,size_t maxTagSize);
    status_t getKeyFrameEntriesIndex(int64_t seekTimeUs,int32_t *index);
    status_t getTagInfoWithOffset(
        size_t trackIndex,off64_t inoffset,
        off64_t *offset, size_t *size, bool *isKey,
        int64_t *tagTimeUs);
    status_t flv_setup_seek_table(off64_t inoffset, off64_t size);
    status_t getTagInfo(
            size_t trackIndex, size_t tagIndex,
            off64_t *offset, size_t *size, bool *isKey,
            int64_t *tagTimeUs);

    status_t getTagTime(
            size_t trackIndex, size_t tagIndex, int64_t *tagTimeUs);

    status_t getTagIndexAtTime(
            size_t trackIndex,
            int64_t timeUs, MediaSource::ReadOptions::SeekMode mode,
            size_t *tagIndex) const;

    status_t addMPEG4CodecSpecificData(size_t trackIndex);
    status_t addH264CodecSpecificData(size_t trackIndex);

    static bool IsCorrectTagType(
        ssize_t trackIndex, Track::Kind kind, uint32_t chunkType);

    DISALLOW_EVIL_CONSTRUCTORS(FLVExtractor);
};

class String8;
struct AMessage;

bool SniffFLV(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  // namespace android

#endif  // FLV_EXTRACTOR_H_

