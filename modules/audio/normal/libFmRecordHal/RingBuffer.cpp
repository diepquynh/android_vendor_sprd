
#include "RingBuffer.h"

#define LOG_TAG "RingBuffer"

#define LOG_DEBUG 0

#if LOG_DEBUG>=1
#define LOGD(...)  printf("RingBuffer: " __VA_ARGS__), printf("  \n")
#define LOGE(...) LOGD(__VA_ARGS__)
#endif

static const nsecs_t kSetParametersTimeoutNs = seconds(3);


namespace android {


bool RingBuffer::is_full()
{
    return (wr_index - rd_index>= unit_count);
}

bool  RingBuffer::is_empty()
{
    return (wr_index == rd_index );
}

uint32_t RingBuffer::GetDataCount()
{
    return (wr_index - rd_index )*unit_size;
}

uint32_t rd_index;
   uint32_t wr_index;


RingBuffer::RingBuffer(uint32_t block_count, uint32_t block_size):rd_index(0),wr_index(0)
{

    unit_size=block_size;
    unit_count=block_count;

    buf_addr=(uint8_t *)malloc(unit_size*unit_count);
}

RingBuffer::~RingBuffer()
{
    if(buf_addr)
        free(buf_addr);
}



uint8_t  *  RingBuffer::GetBuf(uint32_t wait)
{
    uint8_t * buf=NULL;
    mLock_wr.lock();
    if(is_full()){
        if(wait){
            mCond_rd.wait(mLock_wr);
        }
        else{
            mLock_wr.unlock();
            return NULL;
        }
    }
    buf= buf_addr+unit_size*(wr_index%unit_count);
    return buf;

}

void  RingBuffer::PutData(uint32_t  wake)
{
        uint8_t  * buf=NULL;
        if(is_full()){
            // to do error
        }
        wr_index+=1;
        if(wake){
            mCond_wr.signal();
        }
         mLock_wr.unlock();
        return ;
}

uint8_t  * RingBuffer::GetData(uint32_t wait)
{
        //ALOGE("enter GetData");
        status_t result;
        uint8_t  * buf=NULL;
        mLock_rd.lock();
         if(is_empty()){
             if(wait){
                //ALOGE("before wait");
                result = mCond_wr.waitRelative(mLock_rd,kSetParametersTimeoutNs);
                //ALOGE("after wait");
                if(is_empty()){
                    mLock_rd.unlock();
                    return NULL;
                }
            }
            else{
                mLock_rd.unlock();
                return NULL;
            }
        }
        buf= buf_addr+unit_size*(rd_index%unit_count);
        return buf;
}



void  RingBuffer::PutBuf(uint32_t wake)
{
        if(is_full()){
            // to do error
        }
        rd_index+=1;

         if(wake){
            mCond_rd.signal();
        }
        mLock_rd.unlock();
        return ;
}

uint32_t  RingBuffer::BufCount()
{
        return  unit_count;
}

uint32_t   RingBuffer::BufUnitSize()
{
        return  unit_size;
}


}

