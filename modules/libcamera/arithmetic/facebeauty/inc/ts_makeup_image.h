/**
 * Copyright (C) 2012 Thundersoft Corporation
 * All rights Reserved
 */
#ifndef __TS_MAKEUP_IMGAGE_H__
#define __TS_MAKEUP_IMGAGE_H__

#ifdef __cplusplus
extern "C" {
#endif
    /*
     * Data struct : TSMakeupData
     */
    typedef struct  __tag_tsmakeupdata
    {
        int frameWidth;                 //NV21 Frame width.MUST > 0.
        int frameHeight;                //NV21 Frame height. MUST > 0.
        unsigned char *yBuf;            //NV21 Y buffer pointer.MUST not null.
        unsigned char *uvBuf;           //NV21 UV buffer pointer.MUST not null.
    }TSMakeupData;

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif // __TS_MAKEUP_IMGAGE_H__
