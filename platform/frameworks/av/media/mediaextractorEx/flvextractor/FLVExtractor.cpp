/* *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

//#define LOG_NDEBUG 0
#define LOG_TAG "FLVExtractor"
#include <utils/Log.h>

#include "include/avc_utils.h"
#include "FLVExtractor.h"

#include <binder/ProcessState.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

namespace android {

struct FLVExtractor::FLVSource : public MediaSource {
public:
    FLVSource(const sp<FLVExtractor> &extractor, size_t trackIndex);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~FLVSource();

private:
    sp<FLVExtractor> mExtractor;
    size_t mTrackIndex;
    const FLVExtractor::Track &mTrack;
    MediaBufferGroup *mBufferGroup;
    size_t mTagIndex;

    // for AVC.
    bool mIsAVC;
    size_t mNALLengthSize;
    uint8_t *mSrcBuffer;

    //for AAC.
    bool mIsAAC;

    //sp<MP3Splitter> mSplitter;

    bool mIsStartAfterEOS;
    size_t parseNALSize(const uint8_t *data) const;

    DISALLOW_EVIL_CONSTRUCTORS(FLVSource);
};

FLVExtractor::FLVSource::FLVSource(
        const sp<FLVExtractor> &extractor, size_t trackIndex)
    : mExtractor(extractor),
      mTrackIndex(trackIndex),
      mTrack(mExtractor->mTracks.itemAt(trackIndex)),
      mBufferGroup(NULL),
      mIsStartAfterEOS(false){

    const char *mime;
    bool success = mTrack.mMeta->findCString(kKeyMIMEType, &mime);
    CHECK(success);
    mIsAVC = !strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC);
    mIsAAC = !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC);

    if (mIsAVC) {
        uint32_t type;
        const void *data;
        size_t size;
        CHECK(mTrack.mMeta->findData(kKeyAVCC, &type, &data, &size));

        const uint8_t *ptr = (const uint8_t *)data;

        CHECK(size >= 7);
        CHECK_EQ((unsigned)ptr[0], 1u);  // configurationVersion == 1

        // The number of bytes used to encode the length of a NAL unit.
        mNALLengthSize = 1 + (ptr[4] & 3);
    }
    ALOGE("mIsAVC=%d, mNALLengthSize=%zd",mIsAVC,mNALLengthSize);

}

FLVExtractor::FLVSource::~FLVSource() {
    if (mBufferGroup) {
        stop();
    }
}

status_t FLVExtractor::FLVSource::start(MetaData *params) {
    if (params) {
        ALOGI("%s, have MetaData: %p", __FUNCTION__, params);
    }

    CHECK(!mBufferGroup);

    mBufferGroup = new MediaBufferGroup;

    mBufferGroup->add_buffer(new MediaBuffer(mTrack.mMaxTagSize));
    mBufferGroup->add_buffer(new MediaBuffer(mTrack.mMaxTagSize));
    mTagIndex = 0;

    mSrcBuffer = NULL;
    if(mIsAVC)
    {
        mSrcBuffer = new uint8_t[mTrack.mMaxTagSize]; ;
    }

    return OK;
}

status_t FLVExtractor::FLVSource::stop() {
    CHECK(mBufferGroup);

    delete mBufferGroup;
    mBufferGroup = NULL;

    if(NULL != mSrcBuffer)
    {
        delete[] mSrcBuffer;
        mSrcBuffer = NULL;
    }
    return OK;
}

sp<MetaData> FLVExtractor::FLVSource::getFormat() {
    return mTrack.mMeta;
}

size_t FLVExtractor::FLVSource::parseNALSize(const uint8_t *data) const {
    switch (mNALLengthSize) {
        case 1:
            return *data;
        case 2:
            return U16_AT(data);
        case 3:
            return ((size_t)data[0] << 16) | U16_AT(&data[1]);
        case 4:
            return U32_AT(data);
    }

    // This cannot happen, mNALLengthSize springs to life by adding 1 to
    // a 2-bit integer.
    CHECK(!"Should not be here.");

    return 0;
}

status_t FLVExtractor::FLVSource::read(
        MediaBuffer **buffer, const ReadOptions *options) {
     CHECK(mBufferGroup);

    *buffer = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode seekMode;
    status_t status;
    if(mIsAAC)
    {
        if (options && options->getSeekTo(&seekTimeUs, &seekMode)
            && mExtractor->mCurrentTimeUs>=0)
        {
            status = mExtractor->getKeyFramePosition(mTrackIndex, seekTimeUs,mTrack.mMaxTagSize);
            if(mIsStartAfterEOS && (ERROR_MALFORMED == status)) {
                mExtractor->setInitTagPos(mTrackIndex);
            }
            if(mIsStartAfterEOS)
                mIsStartAfterEOS = false;
        }
        else if(options && options->getSeekTo(&seekTimeUs, &seekMode)
            && mExtractor->mLastseekTimeUs<0 && seekTimeUs!=0)
        {
            ALOGE("seek before play !trackid:%d ,save seektime:%lld",mTrack.mKind, (long long)seekTimeUs);
            mExtractor->mLastseekTimeUs = seekTimeUs;
        }
        else if(mExtractor->mCurrentTimeUs > 0 && mExtractor->mLastseekTimeUs > 0
                  && mTrack.mKind == Track::AUDIO)
        {
            ALOGE("seek track:%d with mLastseekTimeUs %lld",mTrack.mKind,(long long)mExtractor->mLastseekTimeUs);
            mExtractor->getKeyFramePosition(mTrackIndex, mExtractor->mLastseekTimeUs,mTrack.mMaxTagSize);
            mExtractor->mLastseekTimeUs = -1;
        }
    }
    else
    {
        if (options && options->getSeekTo(&seekTimeUs, &seekMode))
        {
            status = mExtractor->getKeyFramePosition(mTrackIndex, seekTimeUs,mTrack.mMaxTagSize);
            if(((0 == seekTimeUs) || mIsStartAfterEOS) && (ERROR_MALFORMED == status)) {
                mExtractor->setInitTagPos(mTrackIndex);
            }

            if(mIsStartAfterEOS)
                mIsStartAfterEOS = false;

        }
    }

    for (;;) {
        off64_t offset;
        size_t size;
        bool isKey;
        int64_t timeUs;
        status_t err = mExtractor->getTagInfoWithOffset(mTrackIndex, mTrack.mCurTagPos,
            &offset, &size, &isKey, &timeUs);
        if(mIsAAC)
        {
            mExtractor->mCurrentTimeUs = timeUs;
        }
        ++mTagIndex;

        //ALOGE("getTagInfo offset:%4lld, size:%d, trackID:%d,  tagId:%d,time:%4lld", offset, size,mTrackIndex, mTagIndex, timeUs);
        if (err != OK) {
           // if( mTagIndex < mTrack.mTags.size() ) {
            //    continue;
            //} else {
            if(mIsAAC) {
                mExtractor->mCurrentTimeUs = 0;
            }
            mIsStartAfterEOS = true;

                return ERROR_END_OF_STREAM;
            //}
        }
        if(size > mTrack.mMaxTagSize)
        {
            ALOGE("buffer is not enough, size=%zu,maxsize=%zu",size, mTrack.mMaxTagSize);
            return ERROR_MALFORMED;
        }

        MediaBuffer *out;
        CHECK_EQ(mBufferGroup->acquire_buffer(&out), (status_t)OK);
        //
        out->meta_data()->setInt64(kKeyTime, timeUs);
        if (isKey) {
            out->meta_data()->setInt32(kKeyIsSyncFrame, 1);
        }

        if(!mIsAVC)
        {
            ssize_t n = mExtractor->mDataSource->readAt(offset, out->data(), size);
            if (n < (ssize_t)size) {
                out->release();
                return n < 0 ? (status_t)n : (status_t)ERROR_MALFORMED;
            }

            if(!mIsAAC)
            {
                out->set_range(0, size);
            }
            else
            {
                // read out one AACAUDIODATA(the 1Byte head of AUDIODATA has been skipped).
                /*   struct for AACAUDIODATA
                AACPacketType       UI8         0: AAC sequence header
                                                1: AAC raw
                Data                UI8[n]      if AACPacketType == 0
                                                    AudioSpecificConfig
                                                else if AACPacketType == 1
                                                    Raw AAC frame data
                */
                 // NOTE: skip the  the header of AACAUDIODATA(1Bytes).
                out->set_range(1, (size>1)?(size-1):0);
            }

            *buffer = out;
        }
        else
        {
            // Whole NAL units are returned but each fragment is prefixed by
            // the start code (0x00 00 00 01).
            uint8_t *dstData;
            uint8_t *srcData;
            size_t srcOffset;
            size_t dstOffset;

            /* Read one AVCVIdeoPacket  to the temp buffer( the 1Byte header of VIDEODATA has
                 been skipped ).  */
            ssize_t n = mExtractor->mDataSource->readAt(offset, mSrcBuffer, size);
            if ((n < (ssize_t)size ) || (size < 4)) {
                out->release();
                return n < 0 ? (status_t)n : (status_t)ERROR_MALFORMED;
            }

            // check the AVCPacketType.  Only  send out AVC NALU.
            if( mSrcBuffer[0] != 1)
            {
                // AVCPacketType has been saved in meta data. discard it.
                ALOGE("AVCPacketType = %d, discard. ",mSrcBuffer[0]);
                out->release();
                continue;
            }
            // skip the header of AVCVIdeoPacket(4Bytes).
            size  -= 4;
            srcData = mSrcBuffer + 4;
            srcOffset = 0;
            dstData = (uint8_t *)out->data();
            dstOffset = 0;

            while (srcOffset < size) {
                bool isMalFormed = (srcOffset + mNALLengthSize > size);
                size_t nalLength = 0;
                if (!isMalFormed) {
                    nalLength = parseNALSize(&srcData[srcOffset]);
                    srcOffset += mNALLengthSize;
                    isMalFormed = srcOffset + nalLength > size;
                }

                if (isMalFormed) {
                    ALOGE("Video is malformed,srcOffset=%zu, nalLength=%zu, size=%zu",srcOffset,nalLength,size);
                    out->release();
                    return ERROR_MALFORMED;
                }

                if (nalLength == 0) {
                    ALOGE("nalLength is error or end of the tag");
                    break;
                }

                CHECK(dstOffset + 4 <= out->size());

                dstData[dstOffset++] = 0;
                dstData[dstOffset++] = 0;
                dstData[dstOffset++] = 0;
                dstData[dstOffset++] = 1;
                memcpy(&dstData[dstOffset], &srcData[srcOffset], nalLength);
                srcOffset += nalLength;
                dstOffset += nalLength;
            }
            CHECK_EQ(srcOffset, size);
            CHECK(out != NULL);
            out->set_range(0, dstOffset);

             *buffer = out;
        }
       break;
    }

    return OK;
}

FLVExtractor::FLVExtractor(const sp<DataSource> &dataSource)
    : mDataSource(dataSource),
    mKeyFrameEntries(NULL),
    mKeyFrameNum(0),
    mCurrentTimeUs(0),
    mIsMetadataPresent(false),
    mIsKeyframesPresent(false),
    mLastseekTimeUs(-1) {
    mInitCheck = parseHeaders();

    /*check video height and width to avoid Native Crash*/
    for(size_t i = 0; i < mTracks.size(); i++) {
        sp<MetaData> mdata = mTracks.editItemAt(i).mMeta;
        const char *mime;
        CHECK(mdata->findCString(kKeyMIMEType, &mime));
        if (!strncasecmp(mime, "video/", 6)) {
            int32_t width, height;
            bool success = mdata->findInt32(kKeyWidth, &width);
            success = success && mdata->findInt32(kKeyHeight, &height);
            if(!success) {
                ALOGE("the video track has no height or width profile");
                mInitCheck = UNKNOWN_ERROR;
            }
        }
    }

    if (mInitCheck != OK) {
        mTracks.clear();
    }
}

FLVExtractor::~FLVExtractor(){
    delete[] mKeyFrameEntries;
    mKeyFrameEntries = NULL;
}

size_t FLVExtractor::countTracks() {
    return mTracks.size();
}

sp<IMediaSource> FLVExtractor::getTrack(size_t index) {
    return index < mTracks.size() ? new FLVSource(this, index) : NULL;
}

sp<MetaData> FLVExtractor::getTrackMetaData(
        size_t index, uint32_t flags) {
    if (flags) {
        ALOGI("%s, type: %d", __FUNCTION__, flags);
    }
    return index < mTracks.size() ? mTracks.editItemAt(index).mMeta : NULL;
}

sp<MetaData> FLVExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (1){//mInitCheck == OK) {
        meta->setCString(kKeyMIMEType, "video/flv");
    }

    return meta;
}

uint32_t FLVExtractor::flags() const {
    if(mKeyFrameEntries != NULL)
    {
        return MediaExtractor::flags();
    }
    else
    {
        ALOGI("key frame entries is null, can't seek");
        return CAN_PAUSE;
    }
}

status_t FLVExtractor::parseHeaders() {
    mTracks.clear();

    off64_t dataSize = 0;
    status_t err = mDataSource->getSize(&dataSize);
    if(err == ERROR_UNSUPPORTED)
    {
        dataSize = -1;
    }
    else if(err != OK)
    {
        return err;
    }

    ssize_t res = parseTagHeaders(0ll, dataSize);

    if (res < 0) {
        return (status_t)res;
    }

    return OK;
}

status_t FLVExtractor::parseTagHeaders(off64_t offset, off64_t size) {
    sp<MetaData> meta = new MetaData;
    int32_t maxTagSize = 0;

    if (size >= 0 && size < 9) {
        return ERROR_MALFORMED;
    }

    uint8_t hdr[9];
    if (mDataSource->readAt(offset, hdr, 9) < 9) {
        return ERROR_IO;
    }
    //ALOGE("parseTagHeaders hdr:0x%x %x %x %x %x %x %x %x %x", hdr[0],hdr[1],hdr[2],hdr[3],hdr[4],hdr[5],hdr[6],hdr[7],hdr[8] );

    if(hdr[4]&0x4) {//Audio tags are present
        ALOGE(" detect audio tags");
        mTracks.push();
        Track *track = &mTracks.editItemAt( mTracks.size() - 1 );
        maxTagSize = FLV_AUDIO_TAG_MAX_SIZE;
        sp<MetaData> meta = new MetaData;
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);
        track->mKind = Track::AUDIO;
        track->mMeta = meta;
        track->mMaxTagSize = maxTagSize;
     }

    if(hdr[4]&0x1) {//Video tags are present
        ALOGE(" detect video tags");
        mTracks.push();
        Track *track = &mTracks.editItemAt( mTracks.size() - 1 );
        maxTagSize = FLV_VIDEO_TAG_MAX_SIZE;
        sp<MetaData> meta = new MetaData;
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_H263);
        meta->setInt32(kKeyWidth,1920); //give default value for Width
        meta->setInt32(kKeyHeight,1088); //give default value for Height
        track->mKind = Track::VIDEO;
        track->mMeta = meta;
        track->mMaxTagSize = maxTagSize;
    }

    uint32_t DataOffset = U32_AT(&hdr[5]);

    ssize_t res = parseTag(offset+DataOffset, size);

    if (res < 0) {
        return (status_t)res;
    }

    //store mInitTagPos
    for(size_t i = 0; i<mTracks.size(); i++) {
        Track *track = &mTracks.editItemAt(i);
        track->mInitTagPos = track->mCurTagPos;
    }

    return OK;
}

void FLVExtractor:: setInitTagPos(size_t trackIndex) {
    if (trackIndex >= mTracks.size()) {
        ALOGE("trackId:%zd, size:%zd", trackIndex, mTracks.size());
        return;
    }

    Track *track = &mTracks.editItemAt(trackIndex);
    track->mCurTagPos= track->mInitTagPos;
}

status_t FLVExtractor::flv_setup_seek_table(off64_t inoffset, off64_t size) {
    uint32_t type, len, flags;
    uint32_t kframe_index = 0;
    uint8_t tmp[4 + SIZE_OF_TAG_HEAD + 1];

    /* At first, we have to know the max numbers of seekable frame
      * existed in all video tag,but this will spend times on scanning
      * overall video clip. For saving time to define FLV_SEEK_MAX_ENTRY
      * to suppose and limit the number of seekable frame(AVC or H263). */
    mKeyFrameEntries = new TKeyFrameEntry[FLV_SEEK_MAX_ENTRY];

    while (inoffset < size) {
        ssize_t n = mDataSource->readAt(inoffset, tmp, 4 + SIZE_OF_TAG_HEAD + 1);

        if (n < (4 + SIZE_OF_TAG_HEAD + 1)) {
            ALOGE("readSize:%zd", n );
            return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
        }

           //ALOGE("parseVideoTagHeaders hdr:0x%x,%x,%x,%x,%x,%x,%x,%x,%x", tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[15]);
        type   = tmp[4];
        len    = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);//tag data size
        flags  = tmp[4 + SIZE_OF_TAG_HEAD];

        if ( FLV_TAG_TYPE_VIDEO == type) {
            uint8_t frameType = tmp[15] & 0x17;
            int64_t kframeTimeMs = ( (tmp[8] << 16) | (tmp[9] << 8) | (tmp[10]) );

            /* 0x17 means keyframe for AVC, a seekable frame for AVC
        * 0x12 means keyframe frame for AVC, a seekable frame for Sorenson H.263
        * 0x14 means keyframe for AVC, a seekable frame for On2 VP6
        * 0x15 means keyframe for AVC, a seekable frame for On2 VP6 with alpha channel*/
            if(frameType == 0x17 || frameType == 0x12 || frameType == 0x14 || frameType == 0x15) {
                //ALOGE("Video tag tagpos:%lld kframe[%d] timeMs:%lld", inoffset, kframe_index, kframeTimeMs);
                if (mKeyFrameEntries == NULL) {
                    ALOGE("Alloc keyframe array failed");
                    return ERROR_MALFORMED;
                }

                if (kframe_index < FLV_SEEK_MAX_ENTRY) {
                    mKeyFrameEntries[kframe_index].pos = (uint32_t)inoffset + 4; //Add 4 here Since getKeyFrameEntriesIndex will minus 4 offset back
                    mKeyFrameEntries[kframe_index].timestamp = (uint32_t)kframeTimeMs/1000;
                    mKeyFrameNum = kframe_index;
                } else {
                    ALOGE("kframe index[%d] is more than FLV_SEEK_MAX_ENTRY", kframe_index);
                    delete[] mKeyFrameEntries;
                    mKeyFrameEntries = NULL;
                    return ERROR_MALFORMED;
                }
                kframe_index ++;
            } else {
                /* tmp[15] & 0x20 == 0x20 means a non-seekable frame */
                if(0x20 != (tmp[15] & 0x20)) {
                    ALOGE("This CodecID:%x of videoTag doesn't be defined", (tmp[15] & 0xf));
                    delete[] mKeyFrameEntries;
                    mKeyFrameEntries = NULL;
                    return ERROR_MALFORMED;
                }
            }
        }
        //SIZE_OF_TAG_HEAD + len(tag data size) = Previous tag size
        inoffset += 4 + SIZE_OF_TAG_HEAD + len;
        if (inoffset + SIZE_OF_TAG_HEAD > size) {
            ALOGE("Parse FLV Video seekable frame num:[%d] EOF!", mKeyFrameNum);
            break;
        }
    }
    return OK;
}

status_t FLVExtractor::parseTag(off_t offset, off64_t size) {
     if (size >= 0 && size < (4 + SIZE_OF_TAG_HEAD + 1) ) {
        return ERROR_MALFORMED;
    }
    uint8_t tmp[4 + SIZE_OF_TAG_HEAD + 1];

    ssize_t n = mDataSource->readAt(offset, tmp, 4 + SIZE_OF_TAG_HEAD + 1);

    if (n < (4 + SIZE_OF_TAG_HEAD + 1)) {
        ALOGE("readSize:%zd", n );
        return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
    }

    uint32_t type, len, flags;
    Track *Vtrack = NULL, *Atrack=NULL;
    //ALOGE("parseTag hdr:0x%x %x %x %x %x %x %x %x %x", tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[15]);
    type    = tmp[4];
    len    = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
    //*p_pts  = (tmp[8] << 16) | (tmp[9] << 8) | (tmp[10]);
    flags   = tmp[4 + SIZE_OF_TAG_HEAD];

    //ALOGE("parseTag first run: type:0x%x, len:0x%x, flags:0x%x", type, len, flags);

    if (FLV_TAG_TYPE_META == type) {
        ALOGI("Metadata is presented");
        mIsMetadataPresent = true;
    }

    for( uint32_t i=0; i<mTracks.size(); i++) {
        Track *track = &mTracks.editItemAt( i );
        if (mIsMetadataPresent) {
            track->mCurTagPos = offset + 4 + SIZE_OF_TAG_HEAD + len;
        } else{
            track->mCurTagPos = offset;
        }

        if( track->mKind == Track::VIDEO ) {
            ALOGI("Track::VIDEO");
            Vtrack = track;
        } else if ( track->mKind == Track::AUDIO) {
            ALOGI("Track::AUDIO");
            Atrack = track;
        } else
            ALOGE("parseTag error, track is invalid");
    }

    if( FLV_TAG_TYPE_META == type) {
        flv_read_metabody(offset+4+SIZE_OF_TAG_HEAD);
    }

    if( Vtrack) {
        offset = Vtrack->mCurTagPos;

        if (!mIsKeyframesPresent) {
            if(OK != flv_setup_seek_table(offset, size))
                ALOGE("setup seek table failed");
        }

        for(;;) {
        if( mIsMetadataPresent) {
            ssize_t n = mDataSource->readAt(offset, tmp, 4 + SIZE_OF_TAG_HEAD + 1);

            if (n < (4 + SIZE_OF_TAG_HEAD + 1)) {
                ALOGE("readSize:%zd", n );
                return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
            }

            //ALOGE("parseVideoTagHeaders hdr:0x%x,%x,%x,%x,%x,%x,%x,%x,%x", tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[15]);
            type    = tmp[4];
            len    = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
            flags   = tmp[4 + SIZE_OF_TAG_HEAD];
        }

            if(FLV_TAG_TYPE_VIDEO==type) {
                Vtrack->mCurTagPos = offset;
                ALOGE("get video tag Header! Vtrack->mCurTagPos %d",Vtrack->mCurTagPos);
                switch(flags&0x0f)
                {
                case FLV_CODECID_H263:
                    Vtrack->mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_H263);
                    break;
                case FLV_CODECID_AVC:
                    Vtrack->mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
                    // NOTE: skip the head of VIDEODATA(1Byte) and the header of AVCPacketType(4Bytes).
                    if( len > 5 ) // 1+4
                    {
                        /*   struct for AVCVIDEOPACKET
                            AVCPacketType       UI8         0: AVC sequence header
                                                            1: AVC NALU
                                                            2: AVC end of sequence (lower level NALU
                                                                sequence ender is not required or supported)
                            CompositionTime     SI24        if AVCPacketType == 1
                                                                Composition time offset
                                                            else
                                                                0
                            Data                            UI8[n]      if AVCPacketType == 0
                                                                AVCDecoderConfigurationRecord
                                                            else if AVCPacketType == 1
                                                                One or more NALUs (can be individual
                                                                slices per FLV packets; that is, full frames
                                                                are not strictly required)
                                                            else if AVCPacketType == 2
                                                                Empty
                        */
                        uint8_t *pSrcBuffer;
                        ssize_t n, l;

                        pSrcBuffer = new uint8_t[len-1];
                        if(NULL ==  pSrcBuffer)
                        {
                            return MEDIA_ERROR_BASE;
                        }

                        // read one AVCVIdeoPacket. skip the head of VIDEODATA(1Byte).
                        n = mDataSource->readAt(offset + 4 + SIZE_OF_TAG_HEAD + 1, pSrcBuffer, len-1);
                        l = len - 1;
                        if (n < l) {
                            ALOGE("read AVCVIdeoPacket error, size:%lld vs %zd", (long long)size, n);
                            delete[] pSrcBuffer;
                            return ERROR_MALFORMED;
                        }

                        // check the AVCPacketType.
                        if(pSrcBuffer[0] == 0)
                        {
                            // save the AVCDecoderConfigurationRecord in meta data.
                            // skip the head of AVCVIdeoPacket(4Bytes).
                            Vtrack->mMeta->setData(kKeyAVCC, kTypeAVCC, &(pSrcBuffer[4]), len-5);
                        }
                        // TODO: else.

                        delete[] pSrcBuffer;
                    }
                    break;
                case FLV_CODECID_SCREEN:
                case FLV_CODECID_SCREEN2:
                case FLV_CODECID_VP6:
                case FLV_CODECID_VP6A:
                default:
                    break;
                }
                break;
            } else {
                offset += 4 + SIZE_OF_TAG_HEAD + len;
                continue;
            }
        }
    }

   if(Atrack) {
        offset = Atrack->mCurTagPos;
        for(;;) {
            ssize_t n = mDataSource->readAt(offset, tmp, 4 + SIZE_OF_TAG_HEAD + 1);

            if (n < (4 + SIZE_OF_TAG_HEAD + 1)) {
                ALOGE("readSize:%zd", n );
                return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
            }

            //ALOGE("parseAudioTagHeaders hdr:0x%x,%x,%x,%x,%x,%x,%x,%x,%x", tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[15]);
            type    = tmp[4];
            len    = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
            flags   = tmp[4 + SIZE_OF_TAG_HEAD];

            if(FLV_TAG_TYPE_AUDIO==type) {
                ALOGE("get audio tag Header!");
                Atrack->mCurTagPos = offset;
                uint8_t audio_codec = flags&FLV_AUDIO_CODECID_MASK;
                switch(audio_codec)
                {
                    case FLV_CODECID_MP3:
                        Atrack->mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);
                        break;
                    case FLV_CODECID_AAC:
                    Atrack->mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);

                    // NOTE: skip the head of AUDIODATA(1Byte) and the header of AACPacketType(1Bytes).
                    if ( len > 2 )  {
                        /*   struct for AACAUDIODATA
                        AACPacketType       UI8         0: AAC sequence header
                                                        1: AAC raw
                        Data                            UI8[n]      if AACPacketType == 0
                        AudioSpecificConfig
                        else if AACPacketType == 1
                        Raw AAC frame data
                        */
                        uint8_t *esds;
                        ssize_t n, l;

                        esds = new uint8_t[22+len-2];
                        if (NULL ==  esds) {
                            ALOGE("esds alloc failed");
                            return MEDIA_ERROR_BASE;
                        }
                        memset(esds, 0, 22+len-2);

                        // read one AACAUDIODATA. skip the head of AUDIODATA(1Byte).
                        n = mDataSource->readAt(offset + 4 + SIZE_OF_TAG_HEAD + 1, &(esds[21]), len-1);
                        l = len - 1;
                        if (n < l) {
                            ALOGE("read AACAUDIODATA error, size:%lld vs %zd", (long long)size, n);
                            delete[] esds;
                            return ERROR_MALFORMED;
                        }

                        // check the AACPacketType.
                        if (0 == esds[21] ) {
                            // packet the AudioSpecificConfig into ESDS
                            // skip the head of AACPacketType(1Bytes)
                            esds[0] = 0x3;
                            esds[1] = 18+len;
                            esds[5] = 0x4;
                            esds[6] = 13+len;
                            esds[20] = 0x5;
                            esds[21] = len-2;

                            //saved esds in meta data.
                            Atrack->mMeta->setData(kKeyESDS, kTypeESDS, esds, 22+len-2);

                            //skip AudioSpecificConfig auidodata
                            Atrack->mCurTagPos += 4 + SIZE_OF_TAG_HEAD + len;
                        }
                        // TODO: else.

                        delete[] esds;
                    }
                    break;
                case FLV_CODECID_NELLYMOSER_8HZ_MONO:
                case FLV_CODECID_NELLYMOSER:
                case FLV_CODECID_PCM_BE:
                case FLV_CODECID_ADPCM:
                case FLV_CODECID_PCM_LE:
                default:
                    const char *mime = "application/octet-stream";
                    Atrack->mMeta->setCString(kKeyMIMEType, mime);
                    break;
                }

                //--sample rate.(Hz).bit[3:2].
                if( audio_codec != FLV_CODECID_NELLYMOSER_8HZ_MONO)
                {
                    switch( flags & FLV_AUDIO_RATE_MASK )
                    {
                        case FLV_AUDIO_RATE_5500:
                            Atrack->mMeta->setInt32(kKeySampleRate, 5500);
                            break;
                        case FLV_AUDIO_RATE_11000:
                            Atrack->mMeta->setInt32(kKeySampleRate, 11025);
                            break;
                        case FLV_AUDIO_RATE_22000:
                            Atrack->mMeta->setInt32(kKeySampleRate, 22050);
                            break;
                        case FLV_AUDIO_RATE_44000:
                            Atrack->mMeta->setInt32(kKeySampleRate, 44100);
                            break;
                        default:
                            break;
                    }
                }

                //--sample size(8/16 bits/sample),bit[1]
                if( flags & FLV_AUDIO_SIZE_MASK )
                {
                   //bitspersample = 16;
                }
                else
                {
                    //bitspersample = 8;
                }

                //--sample channnel(mono/stereo),bit[0]
                if( flags & FLV_AUDIO_CHANNEL_MASK )
                {
                    Atrack->mMeta->setInt32(kKeyChannelCount, 2);
                }
                else
                {
                    Atrack->mMeta->setInt32(kKeyChannelCount, 1);
                }

                break;
            } else {
                offset += 4 + SIZE_OF_TAG_HEAD + len;
                continue;
            }
        }
    }

    return OK;
}

ssize_t FLVExtractor::flv_read_metabody(off_t offset)
{
    uint8_t buffer[11]; //only needs to hold the string "onMetaData". Anything longer is something we don't want.

    //first object needs to be "onMetaData" string
    if(mDataSource->readAt(offset, buffer, 4) <1)
        return ERROR_IO;

    CHECK (buffer[0] == AMF_DATA_TYPE_STRING );
    if (amf_get_string(offset+1, buffer, sizeof(buffer)) <= 11) {
       return ERROR_MALFORMED;
    }
    CHECK (!strcmp((const char *)buffer, "onMetaData"));

    offset += (1+12);
    //parse the second object (we want a mixed array)
    //ALOGE("flv_read_metabody, offset:%d", offset);
    return amf_parse_object((const char *)buffer, offset, 0);
}

ssize_t FLVExtractor::amf_get_string(uint32_t offset, uint8_t *buffer, int32_t buffsize)
{
   int length;
   mDataSource->readAt(offset, buffer, 2);
   length =U16_AT(buffer);
   //ALOGE("amf_get_string keylen:%d", length);
    if (length >= buffsize) {
        return -1;
    }

    mDataSource->readAt(offset+2, buffer, length);
    buffer[length] = '\0';
    //ALOGE("amf_get_string %s", buffer);
    return length+2;
}

status_t FLVExtractor::alloc_keyframe_entry(uint32_t array_num) {
    if(mKeyFrameEntries == NULL) {
        mKeyFrameNum = array_num;
        mKeyFrameEntries = new TKeyFrameEntry[mKeyFrameNum];
        if(mKeyFrameEntries == NULL) {
            return ERROR_MALFORMED;
        }
    } else if(mKeyFrameNum != array_num) {
        ALOGE("the two arrays have not the same number of elements!!");
        return ERROR_MALFORMED;
    }
    return OK;
}

static double av_int2dbl(int64_t v)
{
    //if(v+v > 0xFFEULL<<52)
//      return 0.0/0.0;
//      return ldexp( (double)(  ( (v&((1LL<<52)-1))  +  (1LL<<52)  )  *  (v>>63|1)  ),(int)((v>>52&0x7FF)-1075) );

    off64_t tmp;
    off64_t k_tmp;
    double x;
    int exp_p;
    int i =0;

    //--(fract<<52).
    k_tmp = 1;
    k_tmp = (k_tmp<<52) -1;
    tmp = v & k_tmp;

    //--add signal and 1.fract.
    k_tmp = 1;
    k_tmp = (k_tmp<<52);
    tmp += k_tmp;	//(1+fract)<<52.
    k_tmp = (v>>63) | 1 ; //-- x*(-1)^signal.
    tmp = tmp * k_tmp;

    //--exponent.
    exp_p = (int)( (v>>52&0x7FF) - 52 - 1023 );
    if(exp_p>=0)
    {
        tmp = ( tmp << exp_p);
    }
    else
    {
        exp_p = -exp_p;

        //tmp = (tmp >> exp_p);
        for(i=0;i < exp_p;i++)
        {
            tmp = tmp>>1;
        }
    }

    x= (double)tmp;
    return x;
}

double FLVExtractor::amf_get_doublenum(uint32_t offset) {
    uint8_t buffer[8];
    mDataSource->readAt(offset, buffer, 1);
    if(buffer[0]!=0) {
        ALOGE("amf_get_doublenum not double num");
        return 0;
    }
    mDataSource->readAt(offset+1, buffer, 8);
    return av_int2dbl(U64_AT(buffer));
}

ssize_t FLVExtractor::amf_parse_object(const char *key, uint32_t offset, int depth)
{
    AMFDataType amf_type;
    uint8_t str_val[256], tmp[8];
    double num_val = 0;
    uint32_t len = 0;
    uint32_t array_num;

    if(mDataSource->readAt(offset, tmp, 4)<1)
        return ERROR_IO;

    //ALOGE("amf_parse_object,offset:%d, tmp:%x, %x, %x, %x", offset, tmp[0],tmp[1],tmp[2],tmp[3]);
    amf_type = (AMFDataType)tmp[0];

    offset += 1;
    //ALOGE("amf_parse_object, amf-type:0x%x", amf_type);
    switch(amf_type) {
        case AMF_DATA_TYPE_NUMBER:
            mDataSource->readAt(offset, tmp, 8);
            //ALOGE("amf_parse_object,tmp:%x, %x, %x, %x,%x, %x, %x, %x", tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7]);
            offset += 8;
            num_val = av_int2dbl(U64_AT(tmp));
            //ALOGE("AMF_DATA_TYPE_NUMBER, %4llf", num_val);
            break;
        case AMF_DATA_TYPE_BOOL:
            mDataSource->readAt(offset, &num_val, 1);
            offset+=1;
            break;
        case AMF_DATA_TYPE_STRING:
        {
        int i = amf_get_string(offset, str_val, sizeof(str_val));
        if(i < 0)
            return -1;
        len = i;
                offset += len;
        }
            break;
        case AMF_DATA_TYPE_OBJECT:
        /*        while((uint32_t)stream_Tell(p_stream) < max_pos - 2 && amf_get_string(str_val, sizeof(str_val)) > 0) {
                    if(amf_parse_object(str_val, max_pos, depth + 1) < 0)
                        return -1; //if we couldn't skip, bomb out.
                }
                if(get_byte(p_stream) != AMF_END_OF_OBJECT)
                    return -1;
        */

        if(key && depth ==1) {
            if(!strcmp(key,"keyframes")) {
                len = amf_get_string(offset, str_val, sizeof(str_val));//"times"
                offset += len;
                offset = amf_parse_object((const char*)str_val, offset, depth + 1);
                len = amf_get_string(offset, str_val, sizeof(str_val));//"filepositions"
                offset += len;
                offset = amf_parse_object((const char*)str_val, offset, depth + 1);
            }
        }

        mDataSource->readAt(offset, &str_val, 3);
        offset +=3;
        if((str_val[0]|str_val[1]|str_val[2] )!= AMF_END_OF_OBJECT)
            return -1;

        return offset;

        break;
    case AMF_DATA_TYPE_NULL:
    case AMF_DATA_TYPE_UNDEFINED:
    case AMF_DATA_TYPE_UNSUPPORTED:
        break; //these take up no additional space
    case AMF_DATA_TYPE_MIXEDARRAY:
        mDataSource->readAt(offset, tmp, 4);
        offset += 4;

        array_num = U32_AT(tmp);
        //ALOGE("AMF_DATA_TYPE_MIXEDARRAY, array_num:%d",array_num );

        for(uint32_t i=0; i<array_num; i++) {
            len = amf_get_string(offset, str_val, sizeof(str_val));
            offset += len;
            offset = amf_parse_object((const char*)str_val, offset, depth + 1);
        }

        mDataSource->readAt(offset, &str_val, 3);
        offset +=3;
        if((str_val[0]|str_val[1]|str_val[2] )!= AMF_END_OF_OBJECT)
            return -1;

        return offset;
        break;
    case AMF_DATA_TYPE_ARRAY:
        mDataSource->readAt(offset, tmp, 4);
        offset += 4;
        array_num = U32_AT(tmp);
        mIsKeyframesPresent = true;
        ALOGE("AMF_DATA_TYPE_ARRAY,array name:%s, array_num:%d",key,array_num );

        if(depth == 2 && key) {
            if(!strcmp(key,"times")) {
                if(alloc_keyframe_entry(array_num) != OK) {
                    break;
                }
                TKeyFrameEntry * temp = mKeyFrameEntries;
                for(uint32_t i=0; i<array_num; i++) {
                    num_val = amf_get_doublenum(offset);
                    temp->timestamp = (uint32_t) num_val;
                    //LOGD("times AMF_DATA_TYPE_ARRAY:%d num is %ld",i,temp->timestamp);
                    offset += 9;
                    temp += 1;
                }
            } else if(!strcmp(key,"filepositions")) {
                if(alloc_keyframe_entry(array_num) != OK) {
                    break;
                }
                TKeyFrameEntry * temp = mKeyFrameEntries;
                for(uint32_t i=0; i<array_num; i++) {
                    num_val = amf_get_doublenum(offset);
                    //LOGD("filepositions AMF_DATA_TYPE_ARRAY:%d num is %4llf",i,num_val);
                    temp->pos = (uint32_t)num_val;
                    offset += 9;
                    temp += 1;
                }
            }
        }
        return offset;
        /*    unsigned int arraylen, i;

            arraylen = get_be32(p_stream);//--length of the array.

            if (depth == 2 && key)
            {
                uint32_t array_elem_size = 0;

                //--arraylen of two tables must be equal.
                if(!strcmp(key,"filepositions"))
                {
                    p_sys->haskeyframe = 1;
                    p_sys->key_table_pos = stream_Tell(p_stream);
                    p_sys->key_table_len = arraylen;
                    array_elem_size = 9;//-9B per element.
                }
                else if(!strcmp(key,"times"))
                {
                    p_sys->key_table_tim = stream_Tell(p_stream);
                    array_elem_size = 9;
                }

                if (array_elem_size > 0)
                {
                    mplayer_SetSeekable(SCI_TRUE);
                }

                if(array_elem_size > 0 )
                {
                    uint32_t cur_pos = stream_Tell(p_stream);
                    uint32_t next_pos;

                    next_pos = cur_pos + arraylen*array_elem_size;
                    if(next_pos >= max_pos)
                    {
                        return -1;
                    }
                    else
                    {
                        FILE_SEEK(p_stream,next_pos,SEEK_SET);
                        break;
                    }
                }
            }

            for(i = 0; i < arraylen && (uint32_t)stream_Tell(p_stream) < max_pos - 1; i++) {
                if(amf_parse_object(NULL, max_pos, depth + 1) < 0)
                    return -1; //if we couldn't skip, bomb out.
            }*/
            break;
        case AMF_DATA_TYPE_DATE:
            offset+=(8+2);
            break;
        default: //unsupported type, we couldn't skip
            return -1;
    }

    if (depth == 1 && key)
    {
        Track *vtrack=NULL, *atrack=NULL;
        for( uint32_t i=0; i<mTracks.size(); i++) {
            Track *track = &mTracks.editItemAt( i );
            if( track->mKind == Track::VIDEO )
                vtrack = track;
            else if ( track->mKind == Track::AUDIO)
                atrack = track;
            else
                ALOGE("parseTag error, track is invalid");
        }

        //ALOGE("amf_parse_object, key:%s", key);
        //only look for metadata values when we are not nested and key != NULL
        if(amf_type == AMF_DATA_TYPE_BOOL) {
            if(!strcmp(key,"hasAudio"))
            {
            }
            else if(!strcmp(key, "stereo"))
            {
                if(atrack ) atrack->mMeta->setInt32(kKeyChannelCount, 2);
            }
            else if(!strcmp(key,"hasVideo"))
            {
            }
            else if(!strcmp(key,"hasKeyframes"))
            {
            }
            else if(!strcmp(key,"canSeekToEnd"))
            {
            }
        } else if(amf_type== AMF_DATA_TYPE_NUMBER) {
            if(!strcmp(key, "duration"))
            {
                ALOGE("amf_parse_object, duration:%.2fs", num_val);
                if(vtrack) {
                    vtrack->mMeta->setInt64(kKeyDuration, (num_val)*FLV_MOVIE_TIMESCALE);
                    vtrack->mMeta->setInt32(kKeyMaxInputSize, vtrack->mMaxTagSize);
                }

                if(atrack) {
                    atrack->mMeta->setInt64(kKeyDuration, (num_val)*FLV_MOVIE_TIMESCALE);
                    atrack->mMeta->setInt32(kKeyMaxInputSize, atrack->mMaxTagSize);
                }
            }
            else if(!strcmp(key, "videocodecid"))
            {
            }
            else if(!strcmp(key, "width") && num_val > 0)
            {
               ALOGE("amf_parse_object, width:%4f", num_val);
               if(vtrack )
                   vtrack->mMeta->setInt32(kKeyWidth, num_val);
            }
            else if(!strcmp(key, "height") && num_val > 0)
            {
                ALOGE("amf_parse_object, height:%4f", num_val);
                if(vtrack ) vtrack->mMeta->setInt32(kKeyHeight, num_val);
            }
            else if(!strcmp(key,"lastkeyframetimestamp"))
            {
            }
            else if(!strcmp(key,"framerate")&& num_val > 0)
            {
            }
            else if(!strcmp(key, "audiocodecid"))
            {
                //flv_set_audio_codec((int)num_val << FLV_AUDIO_CODECID_OFFSET);
            }
            else if(!strcmp(key,"audiosamplerate")&& num_val > 0)
            {
                ALOGE("amf_parse_object, audiosamplerate:%4f", num_val);
               if(atrack) atrack->mMeta->setInt32(kKeySampleRate, num_val);
            }
            else if(!strcmp(key,"audiosamplesize")&& num_val >= 0)
            {
            }
        }//--end of "else if(amf_type == AMF_DATA_TYPE_NUMBER)"
    }

    return offset;
}

status_t FLVExtractor::getTagInfo(
        size_t trackIndex, size_t tagIndex,
        off64_t *offset, size_t *size, bool *isKey,
        int64_t *tagTimeUs) {
    if (trackIndex >= mTracks.size()) {
        ALOGE("trackId:%zd, size:%zu", trackIndex, mTracks.size());
        return -ERANGE;
    }
    uint32_t tagType;
    Track *track = &mTracks.editItemAt(trackIndex);
    //ALOGE("getTagInfo trackID:%d, tagId:%d, tagNum:%d",trackIndex, tagIndex, track->mTags.size());
    while(tagIndex >= track->mTags.size()) {
            uint8_t tmp[4+SIZE_OF_TAG_HEAD+1];
            ssize_t n = mDataSource->readAt(track->mCurTagPos, tmp, 4+SIZE_OF_TAG_HEAD+1);
            //ALOGE("getTagInfo pos:%x, tmp:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", track->mCurTagPos,
            //    tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[12],tmp[13],tmp[14],tmp[15]);
            if (n < (4+SIZE_OF_TAG_HEAD+1)) {
                return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
            }

            tagType = tmp[4]&0x1F;
            uint32_t tagSize = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
            if( ((FLV_TAG_TYPE_AUDIO== tagType)&&(track->mKind != Track::AUDIO)) || ((FLV_TAG_TYPE_VIDEO== tagType)&&(track->mKind != Track::VIDEO)) ) {
                track->mCurTagPos += (tagSize + 4 + SIZE_OF_TAG_HEAD);
                continue;
            }
            //new tag
            track->mTags.push();
            TagInfo *info = &track->mTags.editItemAt(track->mTags.size() - 1);
            info->mOffset = track->mCurTagPos;
            info->mIsKey = true;//don't know.

            //ALOGE("getTagInfo push pos:%x, info:%x ", track->mCurTagPos, info->mOffset);

            track->mCurTagPos += (tagSize + 4+SIZE_OF_TAG_HEAD);

            if(tagIndex == 0)
            {
                track->mFirstTagSize = tagSize;
                track->mAvgTagSize = tagSize; //don't care it
            }
    //TODO: actually don't need to execute the following data, should return now?
    }

    const TagInfo &info = track->mTags.itemAt(tagIndex);
    *offset = info.mOffset;

    *size = 0;

    uint8_t tmp[4+SIZE_OF_TAG_HEAD+1];
    ssize_t n = mDataSource->readAt(info.mOffset, tmp, 4+SIZE_OF_TAG_HEAD+1);

    //ALOGE("getTagInfo pos:%x, tmp:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", info.mOffset,
    //    tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[12],tmp[13],tmp[14],tmp[15]);

    if (n < (4+SIZE_OF_TAG_HEAD+1) ) {
        return n < 0 ? (status_t)n : (status_t)ERROR_MALFORMED;
    }

    *offset = info.mOffset+4+SIZE_OF_TAG_HEAD+1;
    *size = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
    *size -= 1;
    *isKey = info.mIsKey;
    //*tagTimeUs = ( (tmp[8] << 16) | (tmp[9] << 8) | (tmp[10]) )*1000;
    int64_t tagTimeMs = ( (tmp[8] << 16) | (tmp[9] << 8) | (tmp[10]) );
    *tagTimeUs = tagTimeMs*1000;
    //ALOGE("getTagInfo timeUs:%4lld", *tagTimeUs);

    return OK;
}

status_t FLVExtractor::getTagInfoWithOffset(
    size_t trackIndex,off64_t inoffset,
    off64_t *outoffset, size_t *size, bool *isKey,
    int64_t *tagTimeUs) {

    if (trackIndex >= mTracks.size()) {
        ALOGE("trackId:%zu, size:%zd", trackIndex, mTracks.size());
        return -ERANGE;
    }

    uint32_t tagType;
    off64_t tagpos;
    Track *track = &mTracks.editItemAt(trackIndex);
    tagpos = inoffset;

    while(1) {
        uint8_t tmp[4+SIZE_OF_TAG_HEAD+1];
        ssize_t n = mDataSource->readAt(tagpos, tmp, 4+SIZE_OF_TAG_HEAD+1);
        //LOGE("getTagInfo2:  %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],
        //tmp[10],tmp[11]);
        if (n < (4+SIZE_OF_TAG_HEAD+1)) {
            return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
        }

        tagType = tmp[4]&0x1F;
        uint32_t tagSize = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);

        if( ((FLV_TAG_TYPE_AUDIO== tagType)&&(track->mKind != Track::AUDIO))
                || ((FLV_TAG_TYPE_VIDEO== tagType)&&(track->mKind != Track::VIDEO)) ) {
            tagpos += (tagSize + 4 + SIZE_OF_TAG_HEAD);
            continue;
        } else if(tagType != FLV_TAG_TYPE_AUDIO && tagType != FLV_TAG_TYPE_VIDEO) {
            ALOGE("The tag type is error,tagType is %d",tagType);
            return ERROR_MALFORMED;
        }

        break;
    }

    uint8_t tmp[4+SIZE_OF_TAG_HEAD+1];
    mDataSource->readAt(tagpos, tmp, 4+SIZE_OF_TAG_HEAD+1);


    *outoffset = tagpos+4+SIZE_OF_TAG_HEAD+1;
    *size = (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
    *size -= 1;
    *isKey = 0;
    //*tagTimeUs = ( (tmp[8] << 16) | (tmp[9] << 8) | (tmp[10]) )*1000;
    int64_t tagTimeMs = ( (tmp[8] << 16) | (tmp[9] << 8) | (tmp[10]) );
    *tagTimeUs = tagTimeMs*1000;
    //LOGE("inoffset is %lld,outoffset is %lld,size is %ld,%lld",inoffset,*outoffset,*size,*tagTimeUs);
    //set offset to next tag
    track->mCurTagPos = *outoffset+*size;
    return OK;
}

status_t FLVExtractor::getKeyFramePosition(size_t trackIndex,int64_t seekTimeUs,size_t maxTagSize) {
    off64_t offset;
    off64_t inoffset;
    size_t size;
    bool isKey;
    int64_t timeUs;
    int64_t seekTimeS;
    ALOGE("the %zu track seek time is %lld",trackIndex,(long long)seekTimeUs);
    if (trackIndex >= mTracks.size()) {
        ALOGE("trackId:%zu, size:%zu", trackIndex, mTracks.size());
        return -ERANGE;
    }
    if(mKeyFrameEntries != NULL) {

        Track *track = &mTracks.editItemAt(trackIndex);
        seekTimeS = seekTimeUs/1000000ll;
        int i = 0;
        if(OK != getKeyFrameEntriesIndex(seekTimeUs,&i)) {
            return ERROR_MALFORMED;
        }
        inoffset = mKeyFrameEntries[i].pos-4;
        status_t err = getTagInfoWithOffset(trackIndex,inoffset,&offset, &size, &isKey, &timeUs);
        if(err == OK && size <= maxTagSize) {
            //LOGD("get a audio or video track");
            track->mCurTagPos = offset-4-SIZE_OF_TAG_HEAD-1;
            return OK;
        }

    }
    return ERROR_MALFORMED;
}

status_t FLVExtractor::getKeyFrameEntriesIndex(int64_t seekTimeUs,int32_t *index) {
    if(mKeyFrameEntries != NULL) {
        int64_t seekTimeS;
        off64_t inoffset = 0;
        seekTimeS = seekTimeUs/1000000ll;
        int i,start = 0,end = 0;
        end = mKeyFrameNum-1;
        i = (end-start)/2+start;
        while( start<i && i<end ) {
            inoffset = mKeyFrameEntries[i].pos-4;
            if(inoffset <= 0) {
                ALOGE("the keyframe fileposition is abnormal!");
                return ERROR_MALFORMED;
            }
            if(mKeyFrameEntries[i].timestamp < seekTimeS) {
                start = i;
            } else if(mKeyFrameEntries[i].timestamp > seekTimeS) {
                end = i;
            } else {
                break;
            }
            i = (end - start)/2+start;
        }
        *index = i;
        ALOGE("the key frame index is %d ,the pos is %lld,the time is %d",
              i, (long long)inoffset, mKeyFrameEntries[i].timestamp);
        return OK;

    }

    return ERROR_MALFORMED;
}

bool SniffFLV(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {
    char tmp[4];
    if (source->readAt(0, tmp, 4) < 4) {
        return false;
    }

    if ( !memcmp(tmp, "FLV", 3)) {
         ALOGE("detect FLV files!!!!");
        mimeType->setTo("video/flv");

        // Just a tad over the mp3 extractor's confidence, since
        // these FLV files may contain mp3 content that otherwise would
        // mistakenly lead to us identifying the entire file as a .mp3 file.
        *confidence = 0.31;

        return true;
    }

    return false;
}

}

