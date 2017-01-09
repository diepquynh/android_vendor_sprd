/**
 * Copyright (C) 2012 Thundersoft Corporation
 * All rights Reserved
 */
#ifndef __TS_MAKEUP_API_H__
#define __TS_MAKEUP_API_H__
#include "ts_makeup_data.h"
#include "ts_makeup_image.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef char bool;
    /*
     * PARAMETERS :
     *   @param[in] pInData : The TSMakeupData pointer.MUST not NULL.
     *   @param[out] pOutData : The TSMakeupData pointer.MUST not NULL.
     *   @param[in] skinCleanLevel : Skin clean level, value range [0,100].
     *   @param[in] skinWhitenLevel : Skin whiten level, value range [0,100].
     *   @param[in] pfaceRect: pointer to facerect, if NULL,use ts facedetect, else use system facedata
     * RETURN    : TS_OK if success, otherwise failed.
     *
     */
    int ts_face_beautify(TSMakeupData *pInData, TSMakeupData *pOutData, int skinCleanLevel, int skinWhitenLevel,TSRect* pfaceRect, bool Isfacefd);

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif // __TS_MAKEUP_API_H__
