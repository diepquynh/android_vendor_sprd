/****
Problem List:
1: Need DeleteGlobalRef In C.

****/


#define LOG_TAG "dm_jni"
#include <stdio.h>
#include "jni.h"
#include "JNIHelp.h"
#include "sprd_dm.h"
#undef LOG   
#include <utils/Log.h>


static JavaVM* g_vm                   = NULL;
static JNIEnv* JniEnv                 = NULL;
static jclass  JniClass               = NULL;
static jobject JniObject              = NULL;
static const char *classPathName      = "com/spreadtrum/dm/DmNativeInterface";

jclass     DmNativeInterface_Clazz    = NULL;
jmethodID  Spdm_SendDataCb_Id         = NULL;
jmethodID  Spdm_OpenDialogCb_Id       = NULL;
jmethodID  Spdm_WriteNullCb_Id        = NULL;
jmethodID  Spdm_WriteCb_Id            = NULL;
jmethodID  Spdm_ReadCb_Id             = NULL;
jmethodID  Spdm_ExitNotifyCb_Id       = NULL;

jboolean init_javacb(JNIEnv *env, jobject thiz)
{

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    jboolean IsDmJavaCbSucc = FALSE;    
    JniEnv    = env;
    JniObject = (*env)->NewGlobalRef(env, thiz);

    // Get the Class.
    JniClass = (*env)->FindClass(env, classPathName);   
    if (!JniClass)
    {
        ALOGE("JniClass Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Get the JMethodID for the spdm_sendDataCb.
    Spdm_SendDataCb_Id = (*JniEnv)->GetMethodID(JniEnv, JniClass, "spdm_sendDataCb", "([BII[B)V");
    if (!Spdm_SendDataCb_Id)
    {
        ALOGE("Spdm_SendDataCb_Id Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Get the JMethodID for the spdm_openDialogCb.
    Spdm_OpenDialogCb_Id = (*JniEnv)->GetMethodID(JniEnv, JniClass, "spdm_openDialogCb", "(I[B[BI)V");
    if (!Spdm_OpenDialogCb_Id)
    {
        ALOGE("Spdm_OpenDialogCb_Id Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Get the JMethodID for the spdm_writeNullCb.
    Spdm_WriteNullCb_Id = (*JniEnv)->GetMethodID(JniEnv, JniClass, "spdm_writeNullCb", "(I)V");
    if (!Spdm_WriteNullCb_Id)
    {
        ALOGE("Spdm_WriteNullCb_Id Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Get the JMethodID for the spdm_writeCb.
    Spdm_WriteCb_Id = (*JniEnv)->GetMethodID(JniEnv, JniClass, "spdm_writeCb", "(I[BII)V");
    if (!Spdm_WriteCb_Id)
    {
        ALOGE("Spdm_WriteNullCb_Id Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Get the JMethodID for the Spdm_ReadCb_Id .
    Spdm_ReadCb_Id  = (*JniEnv)->GetMethodID(JniEnv, JniClass, "spdm_readCb", "(I[BII)V");
    if (!Spdm_WriteCb_Id)
    {
        ALOGE("spdm_readCb_Id Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Get the JMethodID for the spdm_exitNotifyCb.
    Spdm_ExitNotifyCb_Id = (*JniEnv)->GetMethodID(JniEnv, JniClass, "spdm_exitNotifyCb", "(I)V");
    if (!Spdm_ExitNotifyCb_Id)
    {
        ALOGE("Spdm_ExitNotifyCb_Id Init Failed. %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return FALSE;
    }

    IsDmJavaCbSucc = TRUE;

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    return IsDmJavaCbSucc;

}

void destroy()
{

    ALOGE(" %s %s %d ", __FILE__, __FUNCTION__, __LINE__);
    JniEnv                     = NULL;
    JniClass                   = NULL;
    JniObject                  = NULL;
    DmNativeInterface_Clazz    = NULL;
    Spdm_SendDataCb_Id         = NULL;
    Spdm_OpenDialogCb_Id       = NULL;
    Spdm_WriteNullCb_Id        = NULL;
    Spdm_WriteCb_Id            = NULL;
    Spdm_ReadCb_Id             = NULL;
    Spdm_ExitNotifyCb_Id       = NULL;
    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);

}
// Java Invoke C Method
static jboolean spdm_jni_start(JNIEnv *env, jobject thiz, jint type, jbyteArray msg_body, jint msg_size)
{

    ALOGE(" %s %s %d type=%d datalen=%d", __FILE__, __FUNCTION__, __LINE__, type, msg_size);
    bool  IsDmStartSucc = FALSE;
    //jbyte *dataBytes = NULL;
    char *p_data = NULL;
    int length = 0 ;   
    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);

    length = (*env)->GetArrayLength(env, msg_body);
    if (length < 0)
    {
        ALOGE("ERROR: android_spdm_start failed");
        return FALSE;
    }

    // Convert jbyteArray type data to char*.
    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__); 
    p_data = (char *)(*env)->GetByteArrayElements(env, msg_body, NULL);
    if (p_data == NULL)
    {
        ALOGE("ERROR: spdm_jni_start failed");
        return FALSE;
    }
    
    if (!init_javacb(env, thiz))
    {
        ALOGE("ERROR: C Invoke Java CallBack Init Failed.");
        return FALSE;        
    }
    
    ALOGE("C Invoke Java CallBack Init Success.");

    IsDmStartSucc = spdm_start(type, p_data, msg_size);
    ALOGE(" %s %s %d IsDmStartSucc=%d", __FILE__, __FUNCTION__, __LINE__, IsDmStartSucc);
    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);

    return IsDmStartSucc;

}

static jboolean spdm_jni_isDmRunning(JNIEnv *env, jobject thiz)
{

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    
    jboolean IsDmRunning = FALSE;
    IsDmRunning = spdm_isDmRunning();
    ALOGE(" %s %s %d IsDmRunning=%d", __FILE__, __FUNCTION__, __LINE__, IsDmRunning);

    return IsDmRunning;

}

static jboolean spdm_jni_transportDataToLib(JNIEnv *env, jobject thiz, jbyteArray data, jint datalen)
{

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    bool IsTransDataToLibSucc = FALSE;
    char *p_data = NULL;
    
    if (!data) 
    {
        ALOGE("ERROR: android_spdm_transportDataToLib failed");
        return FALSE;
    }

    // Convert jbyteArray type data to char* type data.
    //p_data = (*env)->GetStringUTFChars(env, data, NULL);
    p_data = (char *)(*env)->GetByteArrayElements(env, data, 0);
    if (!p_data)
    {
        ALOGE("ERROR: android_spdm_transportDataToLib failed");
        return FALSE;
    }
    
    IsTransDataToLibSucc = spdm_receiveData(p_data, datalen);
    ALOGE(" %s %s %d IsTransDataToLibSucc=%d  datalen =%d ", __FILE__, __FUNCTION__, __LINE__, IsTransDataToLibSucc, datalen);
    
    return IsTransDataToLibSucc;

}

static void spdm_jni_dialogConfirm(JNIEnv *env, jobject thiz,  jboolean retcode)
{

    ALOGE(" %s %s %d retcode=%d", __FILE__, __FUNCTION__, __LINE__, retcode);
    
    spdm_DialogconfirmCb(retcode);

}

static void spdm_jni_stopDm(JNIEnv *env, jclass thiz, jint reason)
{

    ALOGE(" %s %s %d reason=%d", __FILE__, __FUNCTION__, __LINE__, reason);
    if (spdm_isDmRunning())
    {
        (*env)->DeleteGlobalRef(env, JniObject);
        spdm_stopDm(reason);
        //destroy();
    }

    ALOGE(" %s %s %d reason=%d", __FILE__, __FUNCTION__, __LINE__, reason);

}

// Below Is C Invoke Java Method.

void spdm_sendDataCb(char *data, int datalen, int finishflag , char *uri)
{

    ALOGE(" %s %s %d uri=%s datalen=%d finishflag=%d", __FILE__, __FUNCTION__, __LINE__, uri, datalen, finishflag);

    int status;
    JNIEnv *env;
    int IsAttached = 0;
   
    status = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_4);
    if (status < 0)
    {
        ALOGD("Status < 0 Is Native Thread \n");
        status =  (*g_vm)->AttachCurrentThread(g_vm, &env, NULL);
        if (status < 0)
        {
              ALOGD("java_callback failed to attach current thread \n");
              return;
        }

        IsAttached = 1;
    }

    // Malloc Memory To Data.
    jbyteArray jarray = (*env)->NewByteArray(env, datalen);
    if (jarray == NULL)
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }

    // Take jbyteArray Memory First Address.
    jbyte* bytes = (*env)->GetByteArrayElements(env, jarray, NULL);
    if (bytes == NULL)
    {        
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__); 
        if (IsAttached)
        {         
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }

    // Copy Data To 'array'.
    memcpy(bytes, data, datalen);    
 

    // Malloc Memory To 'uri'.
    jbyteArray uri_jarray = (*env)->NewByteArray(env, strlen(uri));
    if (uri_jarray == NULL)
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
		
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }

    // Take uri_jarray Memory First Address.
    jbyte* uri_bytes = (*env)->GetByteArrayElements(env, uri_jarray, NULL);
    if (uri_bytes == NULL)
    {        
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__); 
        if (IsAttached)
        {         
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }		
        return ;
    }

    // Copy Data To 'uri_jarray'.
    memcpy(uri_bytes, uri, strlen(uri));    

    // Invoke Java Method, And Pass Data To Java App.
    (*env)->CallVoidMethod(env, JniObject, Spdm_SendDataCb_Id, jarray, datalen, finishflag, uri_jarray);
   (*env)->ReleaseByteArrayElements(env, jarray, bytes, 0);
    (*env)->ReleaseByteArrayElements(env, uri_jarray, uri_bytes, 0);
   
   // ALOGE(" %s %s %d ", __FILE__, __FUNCTION__, __LINE__, uri_jarray); 
    if (IsAttached)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

void spdm_openDialogCb(int type, char *message, char *title, int timeout)
{
    ALOGE(" %s %s %d message=%s title=%s timeout=%d", __FILE__, __FUNCTION__, __LINE__, message, title, timeout);
    int status;
    JNIEnv *env;
    int IsAttached = 0;
   
    status = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_4);
    if (status < 0)
    {
        ALOGD("Status < 0 Is Native Thread \n");
        status =  (*g_vm)->AttachCurrentThread(g_vm, &env, NULL);
        if (status < 0)
        {
              ALOGD("java_callback failed to attach current thread \n");
              return;
        }
        IsAttached = 1;
    }

    // Malloc Memory To jarray_message .
    jbyteArray jarray_message = (*env)->NewByteArray(env, strlen(message));
    if (jarray_message == NULL)
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }
    (*env)->SetByteArrayRegion(env, jarray_message, 0, strlen(message), (jbyte*)message);
    // Take jarray_message  Memory First Address.
    jbyte* mesbytes = (*env)->GetByteArrayElements(env, jarray_message, NULL);
    if (mesbytes == NULL)
    {        
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }
    
    // Copy mesbytes To 'jarray_message' And Free Memory.
  //  memset(mesbyte,0, strlen(message)+1);
  //  memcpy(mesbytes, message, strlen(message));

    // Malloc Memory To 'title_jarray'.
    jbyteArray title_jarray = (*env)->NewByteArray(env, strlen(title));
    if (title_jarray == NULL)
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }

    // Take title_jarray Memory First Address.
    jbyte* title_bytes = (*env)->GetByteArrayElements(env, title_jarray, NULL);
    if (title_bytes == NULL)
    {        
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }
    
    // Copy Data To 'title_jarray'.
    memcpy(title_bytes, title, strlen(title));
    (*env)->ReleaseByteArrayElements(env, title_jarray, title_bytes, 0);

    // Invoke Java Method, And Pass Data To Java App.
    (*env)->CallVoidMethod(env, JniObject, Spdm_OpenDialogCb_Id, type, jarray_message, title_jarray, timeout);
    (*env)->ReleaseByteArrayElements(env, jarray_message, mesbytes, 0);

    if (IsAttached)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

}

void spdm_writeNullCb(int handletype)
{

    ALOGE(" %s %s %d handletype=%d", __FILE__, __FUNCTION__, __LINE__, handletype);
    int status;
    JNIEnv *env;
    int IsAttached = 0;
   
    status = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_4);
    if (status < 0)
    {
        ALOGD("Status < 0 Is Native Thread \n");

        status =  (*g_vm)->AttachCurrentThread(g_vm, &env, NULL);
        if (status < 0)
        {
              ALOGD("java_callback failed to attach current thread \n");
              return;
        }

        IsAttached = 1;
    }

    (*env)->CallVoidMethod(env, JniObject, Spdm_WriteNullCb_Id, handletype);

    if (IsAttached)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

}

void spdm_writeCb(int handletype, char *data, int offset, int size)
{
    ALOGE(" %s %s %d handletype=%d data=%s  offset=%d size=%d", __FILE__, __FUNCTION__, __LINE__, handletype, data, offset, size);
    int status;
    JNIEnv *env;
    int IsAttached = 0;
   
    status = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_4);
    if (status < 0)
    {
        ALOGD("Status < 0 Is Native Thread \n");
        status =  (*g_vm)->AttachCurrentThread(g_vm, &env, NULL);
        if (status < 0)
        {
              ALOGD("java_callback failed to attach current thread \n");
              return;
        }

        IsAttached = 1;
    }

    // Malloc Memory To Data.
    jbyteArray jarray = (*env)->NewByteArray(env, size);
    if (jarray == NULL)
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }

    // Take jbyteArray Memory First Address.
    jbyte* bytes = (*env)->GetByteArrayElements(env, jarray, NULL);
    if (bytes == NULL)
    {        
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }
    
    // Copy Data To 'jarray'.
    memcpy(bytes, data, size);
    (*env)->ReleaseByteArrayElements(env, jarray, bytes, 0);

    // Invoke Java Method, And Pass Data To Java App.
    (*env)->CallVoidMethod(env, JniObject, Spdm_WriteCb_Id, handletype, jarray, offset, size);

    if (IsAttached)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

}

void spdm_readCb(int handletype, char *buf, int offset, int bufsize)
{
    
    // ALOGE(" %s %s %d handletype=%d buf=%s offset=%d bufsize=%d", __FILE__, __FUNCTION__, __LINE__, handletype, buf, offset, bufsize);
    ALOGE(" %s %s %d handletype=%d offset=%d bufsize=%d", __FILE__, __FUNCTION__, __LINE__, handletype, offset, bufsize);
    int status;
    JNIEnv *env;
    int IsAttached = 0;
   
    status = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_4);
    if (status < 0)
    {
        ALOGD("Status < 0 Is Native Thread \n");
        status =  (*g_vm)->AttachCurrentThread(g_vm, &env, NULL);
        if (status < 0)
        {
              ALOGD("java_callback failed to attach current thread \n");
              return;
        }

        IsAttached = 1;
    }

    // Malloc Memory To Data.
    jbyteArray jarray = (*env)->NewByteArray(env, bufsize);
    if (jarray == NULL)
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }

    // Invoke Java Method, And Pass Data To Java App.
    (*env)->CallVoidMethod(env, JniObject, Spdm_ReadCb_Id, handletype, jarray, offset, bufsize); 

    // Take jbyteArray Memory First Address.
    jbyte* bytes = (*env)->GetByteArrayElements(env, jarray, NULL);
    if (bytes == NULL)
    {        
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        if (IsAttached)
        {
            (*g_vm)->DetachCurrentThread(g_vm);
            return ;
        }
        return ;
    }
    
    // Copy Data To 'jarray'.
    memcpy(buf+offset, bytes + offset, bufsize - offset);
    (*env)->ReleaseByteArrayElements(env, jarray, bytes, 0);

    if (IsAttached)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

}

void spdm_exitNotifyCb(int reason)
{

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    int status;
    JNIEnv *env;
    int IsAttached = 0;
   
    status = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_4);
    if (status < 0)
    {
        ALOGD("Status < 0 Is Native Thread \n");
        status =  (*g_vm)->AttachCurrentThread(g_vm, &env, NULL);
        if (status < 0)
        {
              ALOGD("java_callback failed to attach current thread \n");
              return;
        }

        IsAttached = 1;
    }

    if (spdm_isDmRunning())
    {
        (*env)->CallVoidMethod(env, JniObject, Spdm_ExitNotifyCb_Id);
    }

    if (IsAttached)
    {
        (*g_vm)->DetachCurrentThread(g_vm);
    }

}

static JNINativeMethod methods[] = {

    //Java Invoke C.
    {"spdm_jni_start",              "(I[BI)Z",      (void*)spdm_jni_start},
    {"spdm_jni_isDmRunning",        "()Z",          (void*)spdm_jni_isDmRunning},
    {"spdm_jni_transportDataToLib", "([BI)Z",       (void*)spdm_jni_transportDataToLib},
    {"spdm_jni_dialogConfirm",      "(Z)V",         (void*)spdm_jni_dialogConfirm},
    {"spdm_jni_stopDm",             "(I)V",         (void*)spdm_jni_stopDm},
    
};

static int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numMethods)
{

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    jclass clazz;

    clazz = (*env)->FindClass(env, className);
    if (clazz == NULL) 
    {
        ALOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    //DmNativeInterface_Clazz = clazz;
    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0) 
    {
        ALOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    return JNI_TRUE;

}

static int registerNatives(JNIEnv* env)
{

    //ALOGI("registerNatives");
    ALOGE("registerNatives %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    if (!registerNativeMethods(env, classPathName, methods, sizeof(methods) / sizeof(methods[0]))) 
    {
        ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        return JNI_FALSE;
    }

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    return JNI_TRUE;

}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    
    //uenv.venv = NULL;
    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    jint result = -1;
    JNIEnv* env = NULL;
    g_vm = vm;

    ALOGI("JNI_OnLoad");

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) 
    {
        ALOGE("ERROR: GetEnv failed %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        //ALOGE("ERROR: GetEnv failed");
        goto bail;
    }

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    if (registerNatives(env) != JNI_TRUE)
    {
        ALOGE("ERROR: registerNatives failed %s %s %d", __FILE__, __FUNCTION__, __LINE__);
        goto bail;
    }

    ALOGE(" %s %s %d", __FILE__, __FUNCTION__, __LINE__);
    result = JNI_VERSION_1_4;
    
bail:
    return result;

}

