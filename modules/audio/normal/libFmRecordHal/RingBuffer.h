#ifndef FM_RINGBUFFER_H
#define FM_RINGBUFFER_H
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>

#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <utils/SortedVector.h>
#include <utils/TypeHelpers.h>
#include <utils/Vector.h>


namespace android {

class RingBuffer
{

public:
    RingBuffer(uint32_t block_count, uint32_t block_size);
    ~RingBuffer();

    uint8_t  *  GetBuf(uint32_t wait);
    void            PutData(uint32_t  wake);
    uint8_t  *  GetData(uint32_t wait);
    uint32_t  GetDataCount();
    void            PutBuf(uint32_t wake);
    uint32_t  BufCount();
   uint32_t   BufUnitSize();

private:
    bool is_full();
    bool is_empty();

    uint8_t * buf_addr;
    uint32_t rd_index;
    uint32_t wr_index;
    uint32_t  unit_count;
    uint32_t unit_size;

     mutable     Mutex                   mLock_rd;
    Condition                           mCond_wr;

    mutable     Mutex                   mLock_wr;
    Condition                           mCond_rd;
};


}
#endif

