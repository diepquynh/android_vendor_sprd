#include "FMHalSource.h"



#define LOG_TAG "FMHalSource"

#define LOG_DEBUG 0

#if LOG_DEBUG>=1
#define ALOGD(...)  printf("FMSource: " __VA_ARGS__), printf("  \n")
#define ALOGE(...) ALOGD(__VA_ARGS__)
#endif


extern "C"
{
    #define FM_CARD_NAME "sprdphone"
    int get_snd_card_number(const char *card_name);
}


namespace android {

FMTrack::FMTrack(uint32_t bufferCount,uint32_t BufferUnitSize):mState(FMTrack::IDLE),overflow(1)
{
    ring= new RingBuffer(bufferCount,BufferUnitSize);
}

FMTrack::~FMTrack()
{
    delete ring;
}

uint8_t * FMTrack::GetBuf(uint32_t wait)
{
    return ring->GetBuf( wait);
}

void FMTrack::PutData(uint32_t wake)
{
    return ring->PutData( wake);
}

uint8_t *FMTrack::GetData(uint32_t wait)
{
    return ring->GetData( wait);
}


uint32_t FMTrack::GetDataCount()
{
    return ring->GetDataCount();
}

void FMTrack::PutBuf(uint32_t wake)
{
    return ring->PutBuf( wake);
}

uint32_t FMTrack::BufCount()
{
    return ring->BufCount( );
}

uint32_t FMTrack::BufUnitSize()
{
    return ring->BufUnitSize( );
}


int32_t   FMTrack:: SetState(int state)
{
    overflow = state;
    return 0;
}

int32_t   FMTrack:: GetState()
{
    return  overflow;
}

FMHalThread::FMHalThread(struct pcm *pcmHandle,uint32_t  readUnitSize):Thread(false),
pcmHandle(pcmHandle),readSize(readUnitSize),exit_var(0)
{
    readBuf=(uint8_t *)malloc(readSize);
}

FMHalThread::~FMHalThread()
{
    ALOGD("~FMHalThread in");
    free(readBuf);
    readBuf=NULL;
     ALOGD("~FMHalThread out");
}


int32_t   FMHalThread:: start( sp<FMTrack>  &track)
{
    int32_t  exist=false;
    int   need_signal=false;
    ALOGD("FMHalThread: start in");
     sp <FMHalThread> strongMe = this;
     if(track == 0){
        ALOGD("FMHalThread track is NULL");
        return -1;
     }
    AutoMutex lock(&mLock);
    for( size_t i=0;i<mTrack.size();i++) {
       if( mTrack[i] ==track ){
            exist=true;
            break;
       }
    }

    if(mTrack.size()==0){
        need_signal=true;
    }

    if( !exist){
        mTrack.push(track);
    }
    track->mState=FMTrack::ACTIVE;

    if(need_signal){
        mWaitCond.signal();
    }
    return 0;

}

int32_t   FMHalThread:: stop(sp<FMTrack>  &track)
{
    int32_t  exist=false;
    size_t i=0;
    sp <FMHalThread> strongMe = this;
    ALOGD("FMHalThread: stop in");
    AutoMutex lock(&mLock);
    for(  i=0;i<mTrack.size();i++) {
        if( mTrack[i] == track ){
            exist=true;
            break;
        }
    }

    if( !exist){
        ALOGE(" track to stop is not exist");
        return -1;
    }
    track->mState=FMTrack::IDLE;
    mTrack.removeAt(i);
    return 0;
}

 void FMHalThread::exit(){
    sp <FMHalThread> strongMe = this;
    ALOGD("peter: FMHalThread exit in ");
    if(mTrack.size()==0){
	exit_var = 1;
        requestExit();
        mWaitCond.signal();
        mLock.unlock();
        ALOGD("peter: FMHalThread exit in wait");
        requestExitAndWait();
	exit_var = 0;
        mLock.lock();
        ALOGD("peter: FMHalThread exit in wait out");
    }
}
 void FMHalThread::onFirstRef()
{
    const size_t SIZE = 256;
    char buffer[SIZE];

    snprintf(buffer, SIZE, "FMHalThread Thread %p", this);
    run(buffer, PRIORITY_HIGHEST);
}


 bool        FMHalThread:: threadLoop()
{
    int reval=0;
     sp <FMHalThread> strongMe = this;
    uint8_t * trackBuf=NULL;
    ALOGD("threadLoop start");
    while (!exitPending()) {
            mLock.lock();
            if(mTrack.size()==0){
                ALOGD("threadLoop in wait,refcount is %d ",this->getStrongCount());
		if(exit_var) {
			mLock.unlock();
			break;
		}
                mWaitCond.wait(mLock);
                ALOGD("threadLoop in wait out");
            }

            mLock.unlock();
        //ALOGD("pcm_read threadloop in");
            reval=pcm_read(pcmHandle,readBuf,readSize);
        //ALOGD("pcm_read threadloop out result is %d",reval);
            if(reval) {
                ALOGD("pcm_read error %d",reval);
                memset(readBuf,0,readSize);
                usleep(50);
            }
            mLock.lock();
            for(size_t i=0;i<mTrack.size();i++) {
                sp<FMTrack>   track  = mTrack[i];
                if(track != 0){
                    if(track->mState==FMTrack::ACTIVE) {
            if(track->GetState()) {
                trackBuf= track->GetBuf(false);
                if(trackBuf){
                    memcpy(trackBuf,readBuf,readSize);
                    track->PutData(true);
                }
                else {
                    track->SetState(0);
                    ALOGE("peter: no buffer for track %d and drop the pcm data first time",i);
                }
            }
            else {
                if(track->GetDataCount() <= (track->BufCount()*track->BufUnitSize()/2)) {
                    track->SetState(1);
                    ALOGE("peter: no buffer for track %d no data and recover ok",i);
                }
                else
                    ALOGE("peter: no buffer for track %d and drop the pcm data",i);
            }
                    }
                }
            }
            mLock.unlock();
    }

    ALOGD("thread loop exit refcount is %d",this->getStrongCount());
    return false;
}



sp<FMHalThread>  FMHalSource::FMThread=NULL;
 Mutex            FMHalSource::mLock;
struct pcm * FMHalSource::pcmHandle=NULL;
uint32_t  FMHalSource::openCount=0;

 int FMHalSource::mChannel=1;
 int FMHalSource::mSamplerate=32000;
 //int FMHalSource::mPeriodSize=160;
 int FMHalSource::mPeriodSize=1280;
 //int FMHalSource::mPeriodCount=10;
 int FMHalSource::mPeriodCount=8;

#define FM_PEROID_SIZE          (160*4*2)
//#define FM_PERIOD_COUNT   8
#define FM_PERIOD_COUNT   4

FMHalSource::FMHalSource(int samplerate,int channel):ply_len(0),ply_buf(NULL),mTrack(0),state(IDLE),is_ready(false)
{
        int unitBytes=0;
       struct pcm_config cfg={0};
       cfg.channels=channel;
       cfg.period_count=FM_PERIOD_COUNT;
       cfg.period_size=FM_PEROID_SIZE;
       cfg.rate=samplerate;
       cfg.format =  PCM_FORMAT_S16_LE;
        ALOGD("FMHalSource construct in samplerate is %d, channel is %d",samplerate,channel);
        if((channel!=2) ||(samplerate != 44100)) {
            cfg.rate=44100;
            cfg.channels=2;
        }
        AutoMutex lock(&mLock);
        if(!pcmHandle) {
            int card=get_snd_card_number((const char *)FM_CARD_NAME);
            if(card < 0){
                ALOGE("SmartPA:For Record,can not find the fm card");
                return;
            }
            pcmHandle = pcm_open(card, 0, PCM_IN , &cfg);
             if (!pcm_is_ready(pcmHandle)) {
                ALOGE("FM  cannot open pcm_in driver: %s", pcm_get_error(pcmHandle));
                pcm_close(pcmHandle);
                pcmHandle = NULL;
            }
            mPeriodSize=FM_PEROID_SIZE;
            mPeriodCount=FM_PERIOD_COUNT;
            mChannel= channel;
            mSamplerate = samplerate;
        }
        else{
            if(  (mChannel != channel) ||
                  (mSamplerate !=  samplerate)) {
                  //todo
            }
        }
        if(pcmHandle){
            unitBytes= pcm_frames_to_bytes(pcmHandle, mPeriodSize);
            ALOGE("SmartPA:For New FMTrack");
            mTrack = new FMTrack(mPeriodCount*4,unitBytes);
            ALOGE("SmartPA:For New FMTrack open end");
        }
        if(FMThread == NULL) {
            FMThread = new FMHalThread(pcmHandle,  unitBytes);
        }
        openCount++;
}


FMHalSource:: ~FMHalSource()
{
    int result=-1;
    ALOGD("~FMHalSource in openCount is %d",openCount);
    AutoMutex lock(&mLock);
    openCount--;
    if(!openCount) {
        if(state == START) {
            result= FMThread->stop(mTrack);
        }
        FMThread->exit();
        FMThread=NULL;
        if(pcmHandle) {
            pcm_close(pcmHandle);
            pcmHandle = NULL;
        }
    }
    mTrack=NULL;
    ALOGD("~FMHalSource out");
}



int32_t FMHalSource::start(void)
{
    int unitBytes=0;
    int result=-1;
    sp <FMHalSource> strongMe = this;
    ALOGD("start start in");
    AutoMutex lock(&mLock);
    if(FMThread != 0) {
        AutoMutex lock(&mLock_l);
        result= FMThread->start(mTrack);
        if(!result){
            state=START;
        }
    }
    return result;
}



int32_t FMHalSource::stop(void)
{
        int result=-1;
        ALOGD("start stop in");
        sp <FMHalSource> strongMe = this;
        AutoMutex lock(&mLock);
        if(FMThread != 0) {
             AutoMutex lock(&mLock_l);
             result= FMThread->stop(mTrack);
             if(!result){
                if(ply_buf){
                    mTrack->PutBuf(false);
                    ply_buf=NULL;
                }
                ply_len=0;
                state = STOP;
             }
         }
        return result;
}


int32_t FMHalSource::read(void *  buf, uint32_t  bytes, bool flag)
{
    size_t  left=bytes;
    uint8_t * cur_ptr=(uint8_t *)buf;
    uint32_t copy_count=0;
     sp <FMHalSource> strongMe = this;
 //  ALOGD("peter: read in bytes %d, buffered bytes is %d",bytes,mTrack->GetDataCount());
    AutoMutex lock(&mLock_l);
    if(state !=START){
    memset(buf,0,bytes);
        return bytes;
    }

    if(!is_ready && !flag){
        if(mTrack->GetDataCount() < (mTrack->BufCount()*mTrack->BufUnitSize()/2)) {
            memset(buf,0,bytes);
            ALOGE("peter: fmhalsource read in not ready fill in zero data");
            //usleep(20);
            return bytes;
        }
        else{
            is_ready=true;
            ALOGE("peter: fmhalsource is ready");
        }
    }

    while(left){
        if(!ply_len){
            if(ply_buf) {
                mTrack->PutBuf(true);
                ply_buf=NULL;
            }
           ply_buf= mTrack->GetData(flag);//todo
           if(!ply_buf){
                break;
           }
           ply_len=mTrack->BufUnitSize();
        }
        if(ply_len&&ply_buf){
            copy_count= ply_len>left?left:ply_len;
            memcpy(cur_ptr,ply_buf,copy_count);
            cur_ptr+=copy_count;
            ply_len-=copy_count;
            ply_buf +=copy_count;
            left-=copy_count;
        }
    }

#if 0
    if(left>4){
        int i=0;
        int16_t *buf_left=(int16_t *)buf+(bytes-left)/2;
        for(i=0;i<left/4;i++) {
            buf_left[2*i]=last_l;
            buf_left[2*i+1]=last_r;
        }
        is_ready = false;
        ALOGE("peter: error, no data to read and fill with 0");
        }
        if(bytes > 4) {
            last_r = *((int16_t *)buf+bytes/2-1);
            last_l = *((int16_t *)buf+bytes/2 -2);
        }
#endif
    //ALOGD("peter: read out,bytes read %d",(bytes-left));
    return (bytes-left);
    }


typedef struct {
    int16_t * tmpbuf;
    sp<FMHalSource> mFmhalSource;
    int state;
    int channel;
    int samplerate;
}FMPcmRes;


FMPcmHandler fm_pcm_open(int samplerate,int channels,int bytes,int required_channel)//return 1,ok;
{
    FMPcmRes * pcm = NULL;
    ALOGE("wangzuo_debug:in fm_pcm_open");
    pcm = (FMPcmRes *)calloc(1,sizeof(FMPcmRes));
    if(!pcm)
    {
        ALOGE("wangzuo_debug:malloc pcm in fm_pcm_open failed");
        goto exit;
    }

    if(required_channel < channels )
    {
        ALOGE("wangzuo_debug:channel 1");
        pcm->tmpbuf = (int16_t*)calloc(1,2*bytes);
        if(!pcm->tmpbuf)
        {
            ALOGE("wangzuo_debug:malloc tmpbuf in fm_pcm_open failed");
            goto exit;
        }
    }

    pcm->mFmhalSource = new FMHalSource(samplerate,channels);
    if(pcm->mFmhalSource == NULL)
    {
        ALOGE("wangzuo_debug:new FMHalSource in fm_pcm_open failed");
        goto exit;
    }
    pcm->state = 0;
    pcm->samplerate = samplerate;
    pcm->channel = channels;
     ALOGE("wangzuo_debug:in   fm_pcm_open open sucess");
    return (FMPcmHandler)pcm;

exit:
    if(pcm->tmpbuf)
        free(pcm->tmpbuf);
    if(pcm->mFmhalSource != NULL)
        pcm->mFmhalSource = NULL;
    if(pcm)
        free(pcm);
    return NULL;
}

int fm_pcm_read(FMPcmHandler pcm,void* buf,int bytes,int waitflag,int required_channel) //return 0,OK;here channel is required channel
{
    int16_t * destbuf =(int16_t *)buf;//pcm data 16 bit
    FMPcmRes * pcmtmp = (FMPcmRes *)pcm;
    int bytes_read = 0;
    int j = 0;
    if(!pcm) {
        return 0;
    }
    if(pcmtmp->state == 0)
    {
        pcmtmp->mFmhalSource->start();
        pcmtmp->state = 1;
    }
    if(required_channel < pcmtmp->channel ){
        bytes_read = pcmtmp->mFmhalSource->read((void *)pcmtmp->tmpbuf,2*bytes,waitflag);
        for(j = 0;j<bytes_read/2; j++)
        {
            destbuf[j] = pcmtmp->tmpbuf[2*j];
        }
        return bytes_read/2;
    }
    else {
            bytes_read = pcmtmp->mFmhalSource->read(buf,bytes,waitflag);
            return bytes_read;
    }
}


void fm_pcm_close(FMPcmHandler pcm)
{
    FMPcmRes * pcmtmp = (FMPcmRes *)pcm;
    ALOGE("fm_pcm_close in");
    if(pcmtmp->state == 1)
    {
        pcmtmp->mFmhalSource->stop();
    }
    pcmtmp->state = 0;
    pcmtmp->mFmhalSource = NULL;
    if(pcmtmp->tmpbuf)
        free(pcmtmp->tmpbuf);
}

}

