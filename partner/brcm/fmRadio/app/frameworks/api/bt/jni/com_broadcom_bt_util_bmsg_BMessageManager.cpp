/************************************************************************************
 *
 *  Copyright (C) 2009-2010 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ************************************************************************************/
#include "JNIHelp.h"
#include "jni.h"
#include "utils/Log.h"
#include "utils/misc.h"
#include "android_runtime/AndroidRuntime.h"
#include "bmessage/bmessage_support.h"
#include "bmessage/bmessage_api.h"
#include "bmessage/bmessage_co.h"
#include "bmessage/sms.h"

#define BROADCOM_BLUETOOTH_BMSG_MANAGER_CLASS_PATH "com/broadcom/bt/util/bmsg/BMessageManager"


/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    parseBMsgFile
 * Signature: (Ljava/lang/String;)I
 */
static jint parseBMsgFile(JNIEnv* env, jclass clz, jstring filePath)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status ;
  tBTA_MA_BMSG * p_msg = BTA_MaBmsgCreate();

  if (p_msg == NULL)
  {
      ALOGE("%s: Unable to parse BMessage. Memory allocation for BMessage object failed", __FUNCTION__);
    return (jint)0;
  }

  if (filePath == NULL)
  {
    ALOGE("%s: Unable to parse BMessage. Invalid file name", __FUNCTION__);
    return (jint)0;
  }

  const char* filePathStr = env->GetStringUTFChars(filePath,NULL);
  //Open file stream
  int fd = bta_ma_co_open(filePathStr, BTA_FS_O_RDONLY);
  env->ReleaseStringUTFChars(filePath,filePathStr);

  if (fd == BTA_FS_INVALID_FD)
  {
    ALOGE("%s: Unable to parse BMessage. Unable to open file %s", __FUNCTION__, filePathStr);
    return (jint)0;
  }

  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaParseMapBmsgObj(p_msg,&p_stream);
  bta_ma_co_close(fd);

  if (status != BTA_MA_STATUS_OK)
  {
    ALOGE("%s: Error parsing BMessage. Status = %d", __FUNCTION__, status);
    if (p_msg != NULL)
    {
      BTA_MaBmsgFree(p_msg);
    }
    return (jint)0;
  }
  return (jint)p_msg;
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    parseBMsgFile
 * Signature: (Ljava/lang/String;)I
 */
static jint parseBMsgFileFD(JNIEnv* env, jclass clz, jint fd)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status ;
  tBTA_MA_BMSG * p_msg = BTA_MaBmsgCreate();

  if (p_msg == NULL)
  {
      ALOGE("%s: Unable to parse BMessage. Memory allocation for BMessage object failed", __FUNCTION__);
    return (jint)0;
  }

  if (fd <=0)
  {
    ALOGE("%s: Unable to parse BMessage. Invalid file descriptor", __FUNCTION__);
    return (jint)0;
  }


  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaParseMapBmsgObj(p_msg,&p_stream);
  bta_ma_co_close(fd);

  if (status != BTA_MA_STATUS_OK)
  {
    ALOGE("%s: Error parsing BMessage. Status = %d", __FUNCTION__, status);
    if (p_msg != NULL)
    {
      BTA_MaBmsgFree(p_msg);
    }
    return (jint)0;
  }
  return (jint)p_msg;
}

static jboolean writeBMsgFileFD(JNIEnv* env, jclass clz, jint nativeObj, jint fd)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status;
  tBTA_MA_BMSG * p_msg;

  if (nativeObj ==0) {
    ALOGE("%s: Unable to write BMessage:invalid native object!", __FUNCTION__);
    return JNI_FALSE;
  }

  if (fd <= 0)
  {
    ALOGE("%s: Unable to write BMessage. Invalid file descriptor", __FUNCTION__);
    return JNI_FALSE;
  }

  p_msg = (tBTA_MA_BMSG*) nativeObj;

  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaBuildMapBmsgObj(p_msg,&p_stream);

  if (status != BTA_MA_STATUS_OK)
  {
    ALOGE("%s: Error creating BMessage. Status = %d", __FUNCTION__, status);
    return JNI_FALSE;
  }
  return JNI_TRUE;
}


/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    writeBMsgFile
 * Signature: (ILjava/lang/String;)Z
 */
static jboolean  writeBMsgFile
  (JNIEnv* env, jclass clz, jint nativeObj, jstring filePath)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status;
  tBTA_MA_BMSG * p_msg;

  if (nativeObj ==0) {
    ALOGE("%s: Unable to write BMessage:invalid native object!", __FUNCTION__);
    return JNI_FALSE;
  }

  if (filePath == NULL)
  {
    ALOGE("%s: Unable to write BMessage. Invalid file name", __FUNCTION__);
    return (jint)0;
  }

  p_msg = (tBTA_MA_BMSG*) nativeObj;

  const char* filePathStr = env->GetStringUTFChars(filePath,NULL);
  //Open file stream
  int fd = bta_ma_co_open(filePathStr, BTA_FS_O_CREAT | BTA_FS_O_WRONLY);

  if (fd == BTA_FS_INVALID_FD)
  {
    ALOGE("%s: Unable to write BMessage. Unable to open file %s", __FUNCTION__, filePathStr);
    env->ReleaseStringUTFChars(filePath,filePathStr);
    return JNI_FALSE;
  }
  env->ReleaseStringUTFChars(filePath,filePathStr);
  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaBuildMapBmsgObj(p_msg,&p_stream);
  bta_ma_co_close(fd);

  if (status != BTA_MA_STATUS_OK)
  {
    ALOGE("%s: Error creating BMessage. Status = %d", __FUNCTION__, status);
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    createBMsg
 * Signature: ()I
 */
static jint  createBMsg
  (JNIEnv* env, jclass clz)
{
  tBTA_MA_BMSG* msg = BTA_MaBmsgCreate();
  return (jint) msg;
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    deleteBMsg
 * Signature: (I)V
 */
static void  deleteBMsg
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to delete BMessage: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgFree((tBTA_MA_BMSG*)nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBMsgMType
 * Signature: (IB)V
 */
static void  setBMsgMType
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte msgType)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage Message Type: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetMsgType( (tBTA_MA_BMSG*)nativeObj, (tBTA_MA_MSG_TYPE)msgType);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBMsgMType
 * Signature: (I)B
 */
static jbyte  getBMsgMType
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage Message Type: invalid native object!", __FUNCTION__);
    return (jbyte) 0;
  }

  return (jbyte)BTA_MaBmsgGetMsgType((tBTA_MA_BMSG*)nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBMsgOrig
 * Signature: (I)I
 */
static jint  addBMsgOrig
  (JNIEnv* env, jclass clz, jint nativeObj)
{
    if (nativeObj ==0) {
      ALOGE("%s: Unable to add BMessage Message Originator: invalid native object!", __FUNCTION__);
      return (jint) 0;
    }

    return (jint) BTA_MaBmsgAddOrigToBmsg((tBTA_MA_BMSG*)nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBMsgOrig
 * Signature: (I)I
 */
static jint  getBMsgOrig
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage Message Originator: invalid native object!", __FUNCTION__);
    return (jint) 0;
  }
  return (jint) BTA_MaBmsgGetOrigFromBmsg((tBTA_MA_BMSG*)nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBMsgEnv
 * Signature: (I)I
 */
static jint  addBMsgEnv
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage Message Envelope: invalid native object!", __FUNCTION__);
    return (jint) 0;
  }
  return (jint) BTA_MaBmsgAddEnvToBmsg((tBTA_MA_BMSG*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBMsgEnv
 * Signature: (I)I
 */
static jint  getBMsgEnv
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage Message Envelope: invalid native object!", __FUNCTION__);
    return (jint) 0;
  }
  return (jint) BTA_MaBmsgGetEnv((tBTA_MA_BMSG*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBMsgRd
 * Signature: (IZ)V
 */
static void  setBMsgRd
  (JNIEnv* env, jclass clz, jint nativeObj, jboolean isRead)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage Message Status: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetReadSts((tBTA_MA_BMSG*)nativeObj, (isRead == JNI_TRUE? TRUE: FALSE));
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    isBMsgRd
 * Signature: (I)Z
 */
static jboolean  isBMsgRd
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage Message Status: invalid native object!", __FUNCTION__);
    return JNI_FALSE;
  }

  return (TRUE == BTA_MaBmsgGetReadSts((tBTA_MA_BMSG*) nativeObj)? JNI_TRUE:JNI_FALSE);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBMsgFldr
 * Signature: (ILjava/lang/String{})V
 */
static void  setBMsgFldr
  (JNIEnv* env, jclass clz, jint nativeObj, jstring folder)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage folder: invalid native object!", __FUNCTION__);
    return;
  }

  if (folder == NULL) {
    ALOGE("%s: Unable to set BMessage folder: invalid folder!", __FUNCTION__);
    return;
  }

  const char* folderStr = env->GetStringUTFChars(folder,NULL);
  BTA_MaBmsgSetFolder((tBTA_MA_BMSG*)nativeObj, folderStr);
  env->ReleaseStringUTFChars(folder,folderStr);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBMsgFldr
 * Signature: (ILjava/lang/String;)V
 */
static jstring  getBMsgFldr
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage folder: invalid native object!", __FUNCTION__);
    return NULL;
  }
  const char* folderStr= BTA_MaBmsgGetFolder((tBTA_MA_BMSG*)nativeObj);
  return (folderStr==NULL? NULL: env->NewStringUTF(folderStr));
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBEnvChld
 * Signature: (I)I

 */
static jint  addBEnvChld
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage child envelope: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgAddEnvToEnv((tBTA_MA_BMSG_ENVELOPE*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBEnvChld
 * Signature: (I)I
 */
static jint  getBEnvChld
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage child envelope: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgGetNextEnv((tBTA_MA_BMSG_ENVELOPE*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBEnvRecip
 * Signature: (I)I
 */
static jint  addBEnvRecip
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage recipient: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgAddRecipToEnv((tBTA_MA_BMSG_ENVELOPE*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBEnvRecip
 * Signature: (I)I
 */
static jint  getBEnvRecip
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage recipient: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgGetRecipFromEnv((tBTA_MA_BMSG_ENVELOPE*) nativeObj);
}


/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBEnvBody
 * Signature: (I)I
 */
static jint  addBEnvBody
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage env body: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgAddBodyToEnv((tBTA_MA_BMSG_ENVELOPE*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBEnvBody
 * Signature: (I)I
 */
static jint  getBEnvBody
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage env body: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgGetBodyFromEnv((tBTA_MA_BMSG_ENVELOPE*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBBodyEnc
 * Signature: (IB)V
 */
static void  setBBodyEnc
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte encoding)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage body encoding: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetBodyEncoding((tBTA_MA_BMSG_BODY*)nativeObj, (tBTA_MA_BMSG_ENCODING) encoding);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBBodyEnc
 * Signature: (I)B
 */
static jbyte  getBBodyEnc
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage body encoding: invalid native object!", __FUNCTION__);
    return (jbyte)BTA_MA_BMSG_ENC_UNKNOWN;
  }
  return (jbyte) BTA_MaBmsgGetBodyEncoding((tBTA_MA_BMSG_BODY*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBBodyPartId
 * Signature: (II)V
 */
static void  setBBodyPartId
  (JNIEnv* env, jclass clz, jint nativeObj, jint partId)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage body part id: invalid native object!", __FUNCTION__);
    return;
  }

  BTA_MaBmsgSetBodyPartid( (tBTA_MA_BMSG_BODY*) nativeObj, (UINT16) partId);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBBodyPartId
 * Signature: (I)I
 */
static jint  getBBodyPartId
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage body part id: invalid native object!", __FUNCTION__);
    return (jint) -1;
  }

  return (jint )BTA_MaBmsgGetBodyPartid( (tBTA_MA_BMSG_BODY*) nativeObj);
}



/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    isBBodyMultiP
 * Signature: (I)Z
 */
static jboolean  isBBodyMultiP
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage is multipart attribute: invalid native object!", __FUNCTION__);
    return JNI_FALSE;
  }

  return BTA_MaBmsgIsBodyMultiPart((tBTA_MA_BMSG_BODY*)nativeObj)==TRUE? JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBBodyCharset
 * Signature: (IB)V
 */
static void  setBBodyCharset
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte charset)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage body charset: invalid native object!", __FUNCTION__);
    return;
  }

  BTA_MaBmsgSetBodyCharset((tBTA_MA_BMSG_BODY*) nativeObj, (tBTA_MA_CHARSET) charset);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBBodyCharset
 * Signature: (I)B
 */
static jbyte  getBBodyCharset
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage body charset: invalid native object!", __FUNCTION__);
    return (jbyte) BTA_MA_CHARSET_UNKNOWN;
  }

  return BTA_MaBmsgGetBodyCharset((tBTA_MA_BMSG_BODY*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBBodyLang
 * Signature: (IB)V
 */
static void  setBBodyLang
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte langId)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to s BMessage body lang: invalid native object!", __FUNCTION__);
    return;
  }

  BTA_MaBmsgSetBodyLanguage((tBTA_MA_BMSG_BODY*) nativeObj, (tBTA_MA_BMSG_LANGUAGE)langId);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBBodyLang
 * Signature: (I)B
 */
static jbyte  getBBodyLang
  (JNIEnv* env, jclass clz, jint nativeObj)
{

  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage body lang: invalid native object!", __FUNCTION__);
    return (jbyte) BTA_MA_BMSG_LANG_UNSPECIFIED;
  }

  return (jbyte) BTA_MaBmsgGetBodyLanguage((tBTA_MA_BMSG_BODY*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBBodyCont
 * Signature: (I)I
 */
static jint  addBBodyCont
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage body content: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgAddContentToBody((tBTA_MA_BMSG_BODY*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBBodyCont
 * Signature: (I)I
 */
static jint  getBBodyCont
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage body content: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgGetContentFromBody((tBTA_MA_BMSG_BODY*) nativeObj);
}


/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBContNext
 * Signature: (I)I
 */
static jint  getBContNext
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage next body content: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint)BTA_MaBmsgGetNextContent((tBTA_MA_BMSG_CONTENT*) nativeObj);

}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBContMsg
 * Signature: (ILjava/lang/String;)V
 */
static void  addBContMsg
  (JNIEnv* env, jclass clz, jint nativeObj, jstring msg)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage message to body content: invalid native object!", __FUNCTION__);
    return;
  }

  if (msg == NULL) {
    ALOGE("%s: Unable to add message: NULL message!", __FUNCTION__);
    return;
  }

  const char* msgStr = env->GetStringUTFChars(msg,NULL);
  BTA_MaBmsgAddMsgContent((tBTA_MA_BMSG_CONTENT*) nativeObj, msgStr);
  env->ReleaseStringUTFChars(msg,msgStr);

}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBCont1stMsg
 * Signature: (I)Ljava/lang/String;
 */
static jstring  getBCont1stMsg
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage first message in body content: invalid native object!", __FUNCTION__);
    return NULL;
  }

  const char* msgStr =BTA_MaBmsgGetMsgContent( (tBTA_MA_BMSG_CONTENT*) nativeObj);
  return (msgStr == NULL? NULL: env->NewStringUTF(msgStr));
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBContNextMsg
 * Signature: (I)Ljava/lang/String;
 */
static jstring  getBContNextMsg
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage next message in body content: invalid native object!", __FUNCTION__);
    return NULL;
  }

  const char* msgStr =BTA_MaBmsgGetNextMsgContent( (tBTA_MA_BMSG_CONTENT*) nativeObj);
  return (msgStr == NULL? NULL: env->NewStringUTF(msgStr));
}


/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBvCardNext
 * Signature: (I)I
 */
static jint  getBvCardNext
(JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage next vcard: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (jint) BTA_MaBmsgGetNextVcard((tBTA_MA_BMSG_VCARD*)nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    setBvCardVer
 * Signature: (IB)V
 */
static void  setBvCardVer
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte version)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage vcard version: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetVcardVersion((tBTA_MA_BMSG_VCARD*) nativeObj, (tBTA_MA_VCARD_VERSION) version);

}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBvCardVer
 * Signature: (I)B
 */
static jbyte  getBvCardVer
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to set BMessage vcard version: invalid native object!", __FUNCTION__);
    return (jbyte) -1;
  }
  return BTA_MaBmsgGetVcardVersion((tBTA_MA_BMSG_VCARD*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    addBvCardProp
 * Signature: (IBLjava/lang/String;Ljava/lang/String;)I
 */
static jint  addBvCardProp
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte propId, jstring val, jstring param)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to add BMessage vcard property: invalid native object!", __FUNCTION__);
    return (jint) 0;
  }
  const char* valStr=NULL;
  const char* paramStr=NULL;

  if (val != NULL)
  {
    valStr= env->GetStringUTFChars(val,NULL);
  }

  if (param != NULL)
  {
    paramStr= env->GetStringUTFChars(param,NULL);
  }

  jint obj = (jint) BTA_MaBmsgAddVcardProp( (tBTA_MA_BMSG_VCARD*)nativeObj,
					    (tBTA_MA_VCARD_PROP)propId,valStr,paramStr);

  if (valStr != NULL)
  {
    env->ReleaseStringUTFChars(val,valStr);
  }

  if (paramStr != NULL)
  {
    env->ReleaseStringUTFChars(param,paramStr);
  }
  return obj;
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBvCardProp
 * Signature: (IB)I
 */
static jint  getBvCardProp
  (JNIEnv* env, jclass clz, jint nativeObj, jbyte propId)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage vcard property: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgGetVcardProp((tBTA_MA_BMSG_VCARD*) nativeObj ,(tBTA_MA_VCARD_PROP) propId);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBvCardPropNext
 * Signature: (I)I
 */
static jint  getBvCardPropNext
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage vcard property: invalid native object!", __FUNCTION__);
    return (jint)0;
  }
  return (jint) BTA_MaBmsgGetNextVcardProp((tBTA_MA_VCARD_PROPERTY*) nativeObj);
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBvCardPropVal
 * Signature: (I)Ljava/lang/String;
 */
static jstring  getBvCardPropVal
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage vcard property value: invalid native object!", __FUNCTION__);
    return (jint)0;
  }

  const char* propParam = BTA_MaBmsgGetVcardPropValue((tBTA_MA_VCARD_PROPERTY*)nativeObj);
  return (propParam == NULL? NULL: env->NewStringUTF(propParam));
}

/*
 * Class:     com_broadcom_bt_util_bmsg_BMessageManager
 * Method:    getBvCardPropParam
 * Signature: (I)Ljava/lang/String;
 */
static jstring  getBvCardPropParam
  (JNIEnv* env, jclass clz, jint nativeObj)
{
  if (nativeObj ==0) {
    ALOGE("%s: Unable to get BMessage vcard property param: invalid native object!", __FUNCTION__);
    return (jint)0;
  }

  char* propParam = BTA_MaBmsgGetVcardPropParam((tBTA_MA_VCARD_PROPERTY*)nativeObj);
  return (propParam == NULL? NULL: env->NewStringUTF(propParam));
}


//Conversion from native charset to UTF-8 and vice versa functions

static jstring decodeSMSSubmitPDU(JNIEnv* env, jclass clz, jstring pdu)
{
    ALOGV("%s", __FUNCTION__);
    jstring ret = NULL;
    if ( NULL == pdu )
    {
        ALOGE("%s: NULL pdu passed!", __FUNCTION__);
        return ret;
    }
    char* pdustr= (char *)env->GetStringUTFChars(pdu,NULL);

    CSmsTpdu SMSDecoder(CSmsTpdu::SMS_SUBMIT);
    char * pResult = (char *)SMSDecoder.Decode((unsigned char *)pdustr, (unsigned char *)pdustr + strlen(pdustr));

    if ( pResult != pdustr )
    {
        ret  = env->NewStringUTF(SMSDecoder.UserData());
    }
    if (pdustr != NULL)
    {
      env->ReleaseStringUTFChars(pdu,pdustr);
    }

    return ret;


}

static jstring encodeSMSDeliverPDU(JNIEnv* env, jclass clz, jstring content, jstring recipient, jstring sender, jstring dateTime)
{
    ALOGV("%s", __FUNCTION__);
    jstring ret = NULL;

    if ( NULL == content || NULL == recipient || NULL == sender || NULL == dateTime)
    {
        ALOGE("%s: null content or recipient or sender passed", __FUNCTION__);
        return ret;
    }
    char * contentStr = (char *)env->GetStringUTFChars(content, NULL);
    char * recipientStr = (char *)env->GetStringUTFChars(recipient, NULL);
    char * senderStr = (char *)env->GetStringUTFChars(sender,NULL);
    char * dateTimeStr = (char *)env->GetStringUTFChars(dateTime,NULL);

    CSMSCAddress saSMSC;
    char* pszEnd = NULL;
    CSMSAddress saDestination;
    saDestination.Address(recipientStr);

    CSMSAddress saOriginator;
    saOriginator.Address(senderStr);

    int year= 0, month=0, day=0,hour=0,minute=0,second=0;
    sscanf(dateTimeStr,"%4d%2d%2d%2d%2d%2d", &year, &month,&day, &hour,&minute,&second);

    CSMSTime st;

    //parse date time string to setup the data structure
    st.year(year);
    st.month(month);
    st.day(day);
    st.hour(hour);
    st.minute(minute);
    st.second(second);

    CSmsTpdu SMSEncoder(CSmsTpdu::SMS_DELIVER);
    SMSEncoder.ServiceCenterAddress(saSMSC);
    SMSEncoder.DestinationAddress(saDestination);
    SMSEncoder.OriginatingAddress(saOriginator);

    SMSEncoder.ServiceCenterTimeStamp(st);

    bool bResult = SMSEncoder.Encode((const char*)contentStr, &pszEnd);
    ret  = env->NewStringUTF((const char *)SMSEncoder.GetEncodedString());

    /*
    while ( pszEnd && *pszEnd )
    {
        //TODO : This is multipart behavior. check to see how to handle it for SMS
        bResult = SMSEncoder.Encode(pszEnd, &pszEnd);
    }
    */


    if ( NULL != contentStr )
    {
        env->ReleaseStringUTFChars(content, contentStr);
    }
    if ( NULL != recipientStr )
    {
        env->ReleaseStringUTFChars(recipient, recipientStr);
    }
    if ( NULL != senderStr )
    {
        env->ReleaseStringUTFChars(sender, senderStr );
    }
    if ( NULL != dateTimeStr )
    {
        env->ReleaseStringUTFChars(dateTime, dateTimeStr );
    }
    return ret;
}

static JNINativeMethod sMethods[] = {
     /* name, signature, funcPtr */
    {"parseBMsgFile", "(Ljava/lang/String;)I", (void *)parseBMsgFile},
    {"parseBMsgFileFD", "(I)I", (void *)parseBMsgFileFD},
    {"writeBMsgFile", "(ILjava/lang/String;)Z", (void *)writeBMsgFile},
    {"writeBMsgFileFD", "(II)Z", (void *)writeBMsgFileFD},
    {"createBMsg", "()I", (void *)createBMsg},
    {"deleteBMsg", "(I)V", (void *)deleteBMsg},
    {"setBMsgMType", "(IB)V", (void *)setBMsgMType},
    {"getBMsgMType", "(I)B", (void *)getBMsgMType},
    {"addBMsgOrig", "(I)I", (void *)addBMsgOrig},
    {"getBMsgOrig", "(I)I", (void *)getBMsgOrig},
    {"addBMsgEnv", "(I)I", (void *)addBMsgEnv},
    {"getBMsgEnv", "(I)I", (void *)getBMsgEnv},
    {"setBMsgRd", "(IZ)V", (void *)setBMsgRd},
    {"isBMsgRd", "(I)Z", (void *)isBMsgRd},
    {"setBMsgFldr", "(ILjava/lang/String;)V", (void *)setBMsgFldr},
    {"getBMsgFldr", "(I)Ljava/lang/String;", (void *)getBMsgFldr},
    {"addBEnvChld", "(I)I", (void *)addBEnvChld},
    {"getBEnvChld", "(I)I", (void *)getBEnvChld},
    {"addBEnvRecip", "(I)I", (void *)addBEnvRecip},
    {"getBEnvRecip", "(I)I", (void *)getBEnvRecip},
    {"addBEnvBody", "(I)I", (void *)addBEnvBody},
    {"getBEnvBody", "(I)I", (void *)getBEnvBody},
    {"setBBodyEnc", "(IB)V", (void *)setBBodyEnc},
    {"getBBodyEnc", "(I)B", (void *)getBBodyEnc},
    {"setBBodyPartId", "(II)V", (void *)setBBodyPartId},
    {"getBBodyPartId", "(I)I", (void *)getBBodyPartId},
    {"isBBodyMultiP",   "(I)Z", (void *)isBBodyMultiP},
    {"setBBodyCharset", "(IB)V", (void *)setBBodyCharset},
    {"getBBodyCharset", "(I)B", (void *)getBBodyCharset},
    {"setBBodyLang", "(IB)V", (void *)setBBodyLang},
    {"getBBodyLang", "(I)B", (void *)getBBodyLang},
    {"addBBodyCont", "(I)I", (void *)addBBodyCont},
    {"getBBodyCont", "(I)I", (void *)getBBodyCont},
    {"getBContNext", "(I)I", (void *)getBContNext},
    {"addBContMsg", "(ILjava/lang/String;)V", (void *)addBContMsg},
    {"getBCont1stMsg", "(I)Ljava/lang/String;", (void *)getBCont1stMsg},
    {"getBContNextMsg", "(I)Ljava/lang/String;", (void *)getBContNextMsg},
    {"getBvCardNext", "(I)I", (void *)getBvCardNext},
    {"setBvCardVer", "(IB)V", (void *)setBvCardVer},
    {"getBvCardVer", "(I)B", (void *)getBvCardVer},
    {"addBvCardProp", "(IBLjava/lang/String;Ljava/lang/String;)I", (void *)addBvCardProp},
    {"getBvCardProp", "(IB)I", (void *)getBvCardProp},
    {"getBvCardPropNext", "(I)I", (void *)getBvCardPropNext},
    {"getBvCardPropVal", "(I)Ljava/lang/String;", (void *)getBvCardPropVal},
    {"getBvCardPropParam", "(I)Ljava/lang/String;", (void *)getBvCardPropParam},
    {"decodeSMSSubmitPDU", "(Ljava/lang/String;)Ljava/lang/String;", (void *)decodeSMSSubmitPDU},
    {"encodeSMSDeliverPDU", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void *)encodeSMSDeliverPDU}

};


/*
 * Register native methods for all classes we know about.
 *
 * returns JNI_TRUE on success.
 */
int register_com_broadcom_bt_util_bmsg_BMessageManager(JNIEnv *env)
{

    jclass clazz;

    clazz = env->FindClass(BROADCOM_BLUETOOTH_BMSG_MANAGER_CLASS_PATH);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'", BROADCOM_BLUETOOTH_BMSG_MANAGER_CLASS_PATH);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, sMethods,  sizeof(sMethods) / sizeof(sMethods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", BROADCOM_BLUETOOTH_BMSG_MANAGER_CLASS_PATH);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


