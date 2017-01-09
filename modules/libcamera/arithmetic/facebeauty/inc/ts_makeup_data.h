/**
 * Copyright (C) 2012 Thundersoft Corporation
 * All rights Reserved
 */
#ifndef __TS_MAKEUP_DATA_H__
#define __TS_MAKEUP_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif

    #define TS_OK                (0x00000000)    //Successful
    #define TS_ERROR_PARAM       (0x00000001)    //Parameters error
    #define TS_ERROR_IO          (0x00000002)    //Input or output error
    #define TS_ERROR_INTERNAL    (0x00000003)    //Internal error
    #define TS_NO_MEMORY         (0x00000004)    //No memory error


    /*
     * Data struct : rectangle
     */
    typedef struct __tag_tsrect
    {
        long left;
        long top;
        long right;
        long bottom;
    } TSRect;

    /*
     * Data struct : point
     */
    typedef struct __tag_tsmakeuppoint
    {
        long x;
        long y;
    } TSPoint;

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif // __TS_MAKEUP_DATA_H__
