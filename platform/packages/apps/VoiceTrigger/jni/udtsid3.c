#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <math.h>
#include <time.h>
#include <android/log.h> 
#include <unistd.h>
#include "udtsid3.h"
#include "trulyhandsfree.h"
#include "sensorytypes.h"
#include <cpu-features.h>

#define FULL_DIAGNOSTICS 1

enum {
    MODE_UDT, MODE_SID
};

#define SAVELOG 0 // Write results to SD Card
#define SSDEBUG 0

#define MALLOCZ(s) memset(malloc(s),0,(s))

#if SAVELOG
#define PHRASESPOT_DELAY   (300) // NOTE: does not use sfsResetRecog; 0 will cause multiple triggers
#else
#define PHRASESPOT_DELAY   (75) //(PHRASESPOT_DELAY_ASAP)  // NOTE: this needs to become 0xFFFF for decoy/smart delay triggers
#endif

// ============================================================================
// Reports whether file exists
int fileExists(const char *fname) {
    FILE *fp;
    if ((fp = fopen(fname, "r"))) {
        fclose(fp);
        return (1);
    }
    return (0);
}

// ============================================================================
static char *formatExpirationDate(time_t expiration) {
    static char expdate[33];
    if (!expiration)
        return "never";
    strftime(expdate, 32, "%a, %b %d, %Y", gmtime(&expiration));
    return expdate;
}

// ============================================================================
char *checkFlagsText(int flags) {
    struct {
        int bitfield;
        char *text;
    }
            problems[] = {
            {CHECKRECORDING_BITFIELD_ENERGY_MIN,         "energy_min"},
            {CHECKRECORDING_BITFIELD_ENERGY_STD_DEV,     "energy_std_dev"},
            {CHECKRECORDING_BITFIELD_SIL_BEG_MSEC,       "sil_beg_msec"},
            {CHECKRECORDING_BITFIELD_SIL_END_MSEC,       "sil_end_msec"},
            {CHECKRECORDING_BITFIELD_SNR,                "snr"},
            {CHECKRECORDING_BITFIELD_RECORDING_VARIANCE, "recording_variance"},
            {CHECKRECORDING_BITFIELD_CLIPPING_PERCENT,   "clipping_percent"},
            {CHECKRECORDING_BITFIELD_CLIPPING_MSEC,      "clipping_msec"},
            {CHECKRECORDING_BITFIELD_POOR_RECORDINGS,    "poor_recordings"},
            {CHECKRECORDING_BITFIELD_SPOT,               "spot"},
            {CHECKRECORDING_BITFIELD_VOWEL_DUR,          "vowel_dur"},
            {CHECKRECORDING_BITFIELD_REPETITION,         "repetition"},
            {1 << 12,                                    "too_long_duration"},
            {1 << 13,                                    "critical_error"},
            {1 << 14,                                    "no_speech_detected"},
            {1 << 15,                                    "nothing_recognized"},
            {0,                                          NULL}
    };
    static char r[1024];
    r[0] = '\0';
    int i, n = 0;

    for (i = 0; flags && problems[i].bitfield; i++) {
        if (!(flags & problems[i].bitfield))
            continue;
        flags &= ~problems[i].bitfield;
        if (n > 0) {
            if (flags)
                strcat(r, ", ");
            else
                strcat(r, " and ");
        }
        n++;
        strcat(r, problems[i].text);
    }
    return r;
}

// ============================================================================
void cpuInfo() {
    AndroidCpuFamily family = android_getCpuFamily();
    uint64_t features = android_getCpuFeatures();
    LOG("CPU INFO...\n");
    if (family == ANDROID_CPU_FAMILY_X86)
        LOG(" - ANDROID_CPU_FAMILY_X86\n");
    if (family == ANDROID_CPU_FAMILY_ARM)
        LOG(" - ANDROID_CPU_FAMILY_ARM\n");
    if (family == ANDROID_CPU_FAMILY_MIPS)
        LOG(" - ANDROID_CPU_FAMILY_MIPS\n");
    if (family == ANDROID_CPU_FAMILY_UNKNOWN)
        LOG(" - ANDROID_CPU_FAMILY_UNKNOWN\n");

    if (family == ANDROID_CPU_FAMILY_X86) {
        if (features & ANDROID_CPU_X86_FEATURE_SSSE3)
            LOG(" - ANDROID_CPU_X86_FEATURE_SSSE3\n");
        if (features & ANDROID_CPU_X86_FEATURE_MOVBE)
            LOG(" - ANDROID_CPU_X86_FEATURE_MOVBE\n");
        if (features & ANDROID_CPU_X86_FEATURE_POPCNT)
            LOG(" - ANDROID_CPU_X86_FEATURE_POPCNT\n");
    } else if (family == ANDROID_CPU_FAMILY_ARM) {
        if (features & ANDROID_CPU_ARM_FEATURE_ARMv7)
            LOG(" - ANDROID_CPU_ARM_FEATURE_ARMv7\n");
        if (features & ANDROID_CPU_ARM_FEATURE_VFPv3)
            LOG(" - ANDROID_CPU_ARM_FEATURE_VFPv3\n");
        if (features & ANDROID_CPU_ARM_FEATURE_NEON)
            LOG(" - ANDROID_CPU_ARM_FEATURE_NEON\n");
        if (features & ANDROID_CPU_ARM_FEATURE_LDREX_STREX)
            LOG(" - ANDROID_CPU_ARM_FEATURE_LDREX_STREX\n");
    }
}

// ============================================================================
jlong Java_com_sprd_voicetrigger_nativeinterface_Recog_secondsUntilExpiration(
        JNIEnv *env, jobject job, jlong arg) {
    time_t tnow, texp;

    tnow = time(NULL);
    //LOG("Current time: (%lx): %s\n", tnow, formatExpirationDate(tnow));
    texp = thfGetLicenseExpiration();
    //LOG("Expiration: (%lx):  %s\n", texp, formatExpirationDate(texp));
    if (texp && tnow)
        return texp - tnow;
    else
        return 0x7FFFFFFF;
}

//com.sprd.voicetrigger.commands
// ============================================================================
jlong Java_com_sprd_voicetrigger_commands_Recog_secondsUntilExpiration(
        JNIEnv *env, jobject job, jlong arg) {
    time_t tnow, texp;

    tnow = time(NULL);
    //LOG("Current time: (%lx): %s\n", tnow, formatExpirationDate(tnow));
    texp = thfGetLicenseExpiration();
    //LOG("Expiration: (%lx):  %s\n", texp, formatExpirationDate(texp));
    if (texp && tnow)
        return texp - tnow;
    else
        return 0x7FFFFFFF;
}

// ============================================================================
jstring Java_com_sprd_voicetrigger_nativeinterface_Recog_getExpirationDate(
        JNIEnv *env, jobject job, jlong arg) {
    jstring res = NULL;
    res = (*env)->NewStringUTF(env, formatExpirationDate(thfGetLicenseExpiration()));

    return res;
}

// ============================================================================
jstring Java_com_sprd_voicetrigger_commands_Recog_getExpirationDate(
        JNIEnv *env, jobject job, jlong arg) {
    jstring res = NULL;
    res = (*env)->NewStringUTF(env,
                               formatExpirationDate(thfGetLicenseExpiration()));
    return res;
}

// ============================================================================
jstring Java_com_sprd_voicetrigger_nativeinterface_Recog_getSDKVersion(
        JNIEnv *env, jobject job, jlong arg) {
    jstring res = NULL;
    static char sdkVersion[8];
    sprintf(sdkVersion, "%d.%d.%d-%s", THF_VERSION_MAJOR, THF_VERSION_MINOR,
            THF_VERSION_PATCH, THF_VERSION_PRE);
    res = (*env)->NewStringUTF(env, sdkVersion);
    return res;
}

// ============================================================================
jstring Java_com_sprd_voicetrigger_commands_Recog_getSDKVersion(
        JNIEnv *env, jobject job, jlong arg) {
    jstring res = NULL;
    static char sdkVersion[8];
    sprintf(sdkVersion, "%d.%d.%d-%s", THF_VERSION_MAJOR, THF_VERSION_MINOR,
            THF_VERSION_PATCH, THF_VERSION_PRE);
    res = (*env)->NewStringUTF(env, sdkVersion);
    return res;
}

// ============================================================================
// Initializes main data structure (trig_t) and SDK session
jlong Java_com_sprd_voicetrigger_nativeinterface_Recog_initSession(JNIEnv *env,
                                                                   jobject job) {
    trig_t *t = MALLOCZ(sizeof(trig_t));
    const char *ewhere, *ewhat = NULL;
    time_t tnow, texp;
    //LOG("Create SDK session...");
    cpuInfo();
    //LOG("SDK Version=%s", thfVersion());
    tnow = time(NULL);
    //LOG("Current time: (%lx): %s\n", tnow, formatExpirationDate(tnow));
    texp = thfGetLicenseExpiration();
    //LOG("Expiration: (%lx):  %s\n", texp, formatExpirationDate(texp));
    if (texp && tnow && texp < tnow) {
        LOG("ERROR1: license expired");
        THROW("thfGetLicenseExpiration shows expired license");
    }

    if (!(t->thf = thfSessionCreate())) {
        LOG("ERROR2");
        LOG("thfSessionCreate error: %s", thfGetLastError(NULL));
        THROW("thfSessionCreate");
    }

    return (jlong)(long) t;

    error:
    if (t->thf)
        thfSessionDestroy(t->thf);
    t->thf = NULL;
    if (t)
        free(t);
    t = NULL;
    PANIC("Panic: initSession: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
// Initializes main data structure (trig_t) and SDK session
jlong Java_com_sprd_voicetrigger_commands_Recog_initSession(JNIEnv *env,
                                                            jobject job) {
    trig_t *t = MALLOCZ(sizeof(trig_t));
    const char *ewhere, *ewhat = NULL;
    time_t tnow, texp;
    //LOG("Create SDK session...");
    cpuInfo();
    //LOG("SDK Version=%s", thfVersion());
    tnow = time(NULL);
    //LOG("Current time: (%lx): %s\n", tnow, formatExpirationDate(tnow));
    texp = thfGetLicenseExpiration();
    //LOG("Expiration: (%lx):  %s\n", texp, formatExpirationDate(texp));
    if (texp && tnow && texp < tnow) {
        LOG("ERROR1: license expired");
        THROW("thfGetLicenseExpiration shows expired license");
    }

    if (!(t->thf = thfSessionCreate())) {
        LOG("ERROR2");
        LOG("thfSessionCreate error: %s", thfGetLastError(NULL));
        THROW("thfSessionCreate");
    }

    return (jlong)(long) t;

    error:
    if (t->thf)
        thfSessionDestroy(t->thf);
    t->thf = NULL;
    if (t)
        free(t);
    t = NULL;
    PANIC("Panic: initSession: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
jlong Java_com_sprd_voicetrigger_nativeinterface_Recog_isEnrolled(JNIEnv *env,
                                                                  jobject job, jlong arg) {
    trig_t *t = (trig_t *) (long) arg;
    if (t->searchTest_COMB)
        return 1;
    return 0;
}

// ============================================================================
// Initializes recognition.
// This UDT implementation utilizes three separate recognizers (r1, r2) with different configurations
// to perform the following roles :
// - r1 is used for speech detection and pronunciation extraction during enrollment
// - r2 is used for testing
//
jlong Java_com_sprd_voicetrigger_nativeinterface_Recog_initRecog(JNIEnv *env,
                                                                 jobject job, jlong arg,
                                                                 jstring jnetfile_UDT,
                                                                 jstring jenrollnetfile_UDT,
                                                                 jstring judtsvsidfile_UDT,
                                                                 jstring jphsearchfile_UDT,
                                                                 jfloat jparamAStart_UDT,
                                                                 jfloat jparamBStart_UDT,
                                                                 jfloat jcheckSNR_UDT,
                                                                 jstring jnetfile_EFT,
                                                                 jstring jsearchfile_EFT,
                                                                 jstring jantidatafile_EFT,
                                                                 jfloat jparamAOffset_EFT,
                                                                 jfloat jdurMinFactor_EFT,
                                                                 jfloat jdurMaxFactor_EFT,
                                                                 jfloat jtriggerSampPerCat_EFT,
                                                                 jfloat jsampPerCatWithin_EFT,
                                                                 jfloat jtargetSNR_EFT,
                                                                 jfloat jlearnRate_EFT,
                                                                 jfloat jlearnRateWithin_EFT,
                                                                 jfloat jdropoutWithin_EFT,
                                                                 jfloat jepqMin_EFT,
                                                                 jfloat juseFeat_EFT,
                                                                 jfloat jcheckSNR_EFT,
                                                                 jfloat jcheckVowelDur_EFT,
                                                                 jfloat jsvThresholdBase,
                                                                 jfloat jsvThresholdStep,
                                                                 jstring jsavedir,
                                                                 jshort numUsersXX,
                                                                 jshort numEnroll) {

    trig_t *t = (trig_t *) (long) arg;
    float lsil = SDET_LSILENCE, tsil = SDET_TSILENCE, maxR = SDET_MAXRECORD,
            minDur = SDET_MINDURATION, ltms_udt = SDET_LONGTERMMS_UDT;
    float ltms_EFT = SDET_LONGTERMMS_EFT;
    const char *ewhere, *ewhat = NULL, *tmp;
    int uIdx;

    if (!t) THROW("No SDK Session");

    tmp = (*env)->GetStringUTFChars(env, jsavedir, 0);
    t->savedir = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jsavedir, tmp);

//  if (numUsers > MAX_USERS)
//    THROW("Too many users");
//  t->maxUsers = numUsers;
    if (numEnroll > MAX_ENROLL) THROW("Too many enrolls");

    // EFT STUFF  ============================================

    t->numUsers_EFT = 0;

    tmp = (*env)->GetStringUTFChars(env, jnetfile_EFT, 0);
    t->netfileName_EFT = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jnetfile_EFT, tmp);
    if (!fileExists(t->netfileName_EFT)) THROW("jnetfile_EFT file does not exist");

    tmp = (*env)->GetStringUTFChars(env, jsearchfile_EFT, 0);
    t->searchfileName_EFT = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jsearchfile_EFT, tmp);
    if (!fileExists(t->searchfileName_EFT)) THROW("jsearchfile_EFT file does not exist");

    if (jantidatafile_EFT != NULL) {
        tmp = (*env)->GetStringUTFChars(env, jantidatafile_EFT, 0);
        t->antidatafileName_EFT = strdup(tmp);
        (*env)->ReleaseStringUTFChars(env, jantidatafile_EFT, tmp);
        if (!fileExists(t->antidatafileName_EFT)) THROW("jantidatafile_EFT file does not exist");
    } else {
        t->antidatafileName_EFT = NULL;
    }

    /* Phoneme Recognizer - used for speech detection of enrollment phrases */
    // load phoneme netfile
    LOG("recogInit:Initializing phoneme recognizer");
    if (!(t->recogEnrollSD_EFT = thfRecogCreateFromFile(t->thf,
                                                        t->netfileName_EFT, 0xFFFF, -1,
                                                        SDET))) THROW("thfRecogCreateFromFile");
    // configure recognizer
    if (!thfRecogConfigSet(t->thf, t->recogEnrollSD_EFT, REC_LSILENCE, lsil)) THROW(
            "thfRecogConfigSet:REC_LSILENCE");
    if (!thfRecogConfigSet(t->thf, t->recogEnrollSD_EFT, REC_LONGTERMMS,
                           ltms_EFT)) THROW("thfRecogConfigSet:REC_LONGTERMMS");
    if (!thfRecogConfigSet(t->thf, t->recogEnrollSD_EFT, REC_TSILENCE, tsil)) THROW(
            "thfRecogConfigSet:REC_TSILENCE");
    if (!thfRecogConfigSet(t->thf, t->recogEnrollSD_EFT, REC_MAXREC, maxR)) THROW(
            "thfRecogConfigSet:REC_MAXREC");
    if (!thfRecogConfigSet(t->thf, t->recogEnrollSD_EFT, REC_MINDUR, minDur)) THROW(
            "thfRecogConfigSet:REC_MINDUR");
    if (!thfRecogInit(t->thf, t->recogEnrollSD_EFT, NULL, RECOG_KEEP_WAVE)) THROW("thfRecogInit");

    // 2015-04-02 added this.
    if (!thfRecogConfigSet(t->thf, t->recogEnrollSD_EFT, REC_CHECK_SNR,
                           jcheckSNR_EFT)) THROW("thfRecogConfigSet:REC_CHECK_SNR");

    LOG("recogInit: Phrase Spot Recognizer");
    if (!(t->recogEnrollNoSD_EFT = thfRecogCreateFromFile(t->thf,
                                                          t->netfileName_EFT, 0xFFFF, -1,
                                                          NO_SDET))) THROW(
            "thfRecogCreateFromFile");
    if (!(t->searchEnroll_EFT = thfSearchCreateFromFile(t->thf,
                                                        t->recogEnrollNoSD_EFT,
                                                        t->searchfileName_EFT, NBEST))) THROW(
            "thfSearchCreateFromFile");
    if (!thfRecogInit(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                      RECOG_KEEP_WAVE_WORD_PHONEME)) // eba???
    THROW("thfRecogInit");

    if (t->antidatafileName_EFT != NULL) {
        if (!thfSpeakerCreateFromFile(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                      t->antidatafileName_EFT)) THROW("thfSpeakerCreateFromFile");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_DUR_MIN_FACTOR, jdurMinFactor_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_DUR_MIN_FACTOR");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_DUR_MAX_FACTOR, jdurMaxFactor_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_DUR_MAX_FACTOR");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_TRIGGER_SAMP_PER_CAT, jtriggerSampPerCat_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_TRIGGER_SAMP_PER_CAT");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_SAMP_PER_CAT_WITHIN, jsampPerCatWithin_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_SAMP_PER_CAT_WITHIN");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_TARGET_SNR, jtargetSNR_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_TARGET_SNR");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_LEARN_RATE, jlearnRate_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_LEARN_RATE");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_LEARN_RATE_WITHIN, jlearnRateWithin_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_LEARN_RATE_WITHIN");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_DROPOUT_WITHIN, jdropoutWithin_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_DROPOUT_WITHIN");
        if (!thfSpeakerConfigSet(t->thf, t->recogEnrollNoSD_EFT, MAX_EFT,
                                 SPEAKER_USE_FEAT, juseFeat_EFT)) THROW(
                "thfSpeakerConfigSet:SPEAKER_USE_FEAT");
    }
    if (!thfPhrasespotConfigSet(t->thf, t->recogEnrollNoSD_EFT,
                                t->searchEnroll_EFT, PS_DELAY, 150.0)) THROW(
            "thfPhrasespotConfigSet:PS_DELAY");

    if (!thfPhrasespotConfigSet(t->thf, t->recogEnrollNoSD_EFT,
                                t->searchEnroll_EFT, PS_PARAMA_OFFSET, jparamAOffset_EFT)) THROW(
            "thfPhrasespotConfigSet:PS_PARAMA_OFFSET");

    if (!thfRecogConfigSet(t->thf, t->recogEnrollNoSD_EFT, REC_EPQ_ENABLE,
                           DONT_USE_EPQ)) THROW("thfRecogConfigSet:REC_EPQ_MIN");

    if (!thfRecogConfigSet(t->thf, t->recogEnrollNoSD_EFT, REC_EPQ_MIN,
                           jepqMin_EFT)) THROW("thfRecogConfigSet:REC_EPQ_MIN");

    // 2015-04-02 added this.
    if (!thfRecogConfigSet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SNR,
                           jcheckSNR_EFT)) THROW("thfRecogConfigSet:REC_CHECK_SNR");

    //2015-04-16 added this
    if (!thfRecogConfigSet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_VOWEL_DUR,
                           300.f)) THROW("thfRecogConfigSet:REC_CHECK_VOWEL_DUR");

    t->checkSNR_EFT = jcheckSNR_EFT;
    t->checkVowelDur_EFT = jcheckVowelDur_EFT;

    //RFP - LOG("init CheckEnrollment recog, search");
    if (!(t->recogCheckEnroll_EFT = thfRecogCreateFromFile(t->thf,
                                                           t->netfileName_EFT, 0xFFFF, -1,
                                                           NO_SDET))) THROW(
            "thfRecogCreateFromFile");
    if (!(t->searchCheckEnroll_EFT = thfSearchCreateFromFile(t->thf,
                                                             t->recogCheckEnroll_EFT,
                                                             t->searchfileName_EFT, NBEST))) THROW(
            "thfSearchCreateFromFile Search 4");
    if (!thfRecogInit(t->thf, t->recogCheckEnroll_EFT, t->searchCheckEnroll_EFT,
                      RECOG_KEEP_WAVE)) THROW("thfRecogInit");
    if (!thfSpeakerCreateFromFile(t->thf, t->recogCheckEnroll_EFT, MAX_EFT,
                                  t->antidatafileName_EFT)) THROW("thfSpeakerCreateFromFile");
    if (!thfPhrasespotConfigSet(t->thf, t->recogCheckEnroll_EFT,
                                t->searchCheckEnroll_EFT, PS_DELAY, 150.0)) THROW(
            "thfPhrasespotConfigSet:PS_DELAY");

    // 2015-04-02 added this.
    if (!thfRecogConfigSet(t->thf, t->recogCheckEnroll_EFT, REC_CHECK_SNR,
                           t->checkSNR_EFT)) THROW("thfRecogConfigSet:REC_CHECK_SNR");

    //RFP - LOG("init 'CheckEnrollment' pseudo-user");
    t->userCheckEnroll_EFT = malloc(sizeof(thfUser_t));
    memset(t->userCheckEnroll_EFT, 0, sizeof(thfUser_t));
    t->userCheckEnroll_EFT[0].userName = malloc(sizeof(char) * MAXSTR);
    memset(t->userCheckEnroll_EFT[0].userName, 0, sizeof(char) * MAXSTR);
    sprintf(t->userCheckEnroll_EFT[0].userName, "EFT-1");
    t->userCheckEnroll_EFT[0].enroll = realloc(t->userCheckEnroll_EFT[0].enroll,
                                               sizeof(udtEnrollment_t));
    memset(&(t->userCheckEnroll_EFT[0].enroll[0]), 0, sizeof(udtEnrollment_t));
    t->userCheckEnroll_EFT[0].numEnroll = 1;
    LOG("recogInit: resetting users");

    /* create and initialize the user structure */
    t->user_EFT = malloc(sizeof(thfUser_t) * MAX_EFT);
    memset(t->user_EFT, 0, sizeof(thfUser_t) * MAX_EFT);
    for (uIdx = 0; uIdx < MAX_EFT; uIdx++) {
        t->user_EFT[uIdx].userName = malloc(sizeof(char) * MAXSTR);
        memset(t->user_EFT[uIdx].userName, 0, sizeof(char) * MAXSTR);
        sprintf(t->user_EFT[uIdx].userName, "EFT-%i", uIdx + 1);
        t->user_EFT[uIdx].numEnroll = 0;
        t->user_EFT[uIdx].enroll = NULL;
    }

    // UDT STUFF  ============================================

    t->numUsers_UDT = 0;

    // Check acoustic model file exists
    tmp = (*env)->GetStringUTFChars(env, jnetfile_UDT, 0);
    t->netfileName_UDT = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jnetfile_UDT, tmp);
    if (!fileExists(t->netfileName_UDT)) THROW("Net file does not exist");

    /* Enroll Recognizer - used for recording and pipelined pronunciation extraction of enrollment phrases */
    if (!(t->recogEnroll_UDT = thfRecogCreateFromFile(t->thf,
                                                      t->netfileName_UDT, 0xFFFF, -1, SDET))) THROW(
            "thfRecogCreateFromFile");

    t->paramAStart_UDT = jparamAStart_UDT;
    t->paramBStart_UDT = jparamBStart_UDT;
    t->checkSNR_UDT = jcheckSNR_UDT;

    // configure enroll recognizer
    if (!thfRecogConfigSet(t->thf, t->recogEnroll_UDT, REC_LSILENCE, lsil)) THROW(
            "thfRecogConfigSet:REC_LSILENCE");
    if (!thfRecogConfigSet(t->thf, t->recogEnroll_UDT, REC_TSILENCE, tsil)) THROW(
            "thfRecogConfigSet:REC_TSILENCE");
    if (!thfRecogConfigSet(t->thf, t->recogEnroll_UDT, REC_MAXREC, maxR)) THROW(
            "thfRecogConfigSet:REC_MAXREC");
    if (!thfRecogConfigSet(t->thf, t->recogEnroll_UDT, REC_MINDUR, minDur)) THROW(
            "thfRecogConfigSet:REC_MINDUR");
    if (!thfRecogConfigSet(t->thf, t->recogEnroll_UDT, REC_LONGTERMMS,
                           ltms_udt)) THROW("thfRecogConfigSet:REC_LONGTERMMS");

    // 2015-04-02 added this.
    if (!thfRecogConfigSet(t->thf, t->recogEnroll_UDT, REC_CHECK_SNR,
                           t->checkSNR_UDT)) THROW("thfRecogConfigSet:REC_CHECK_SNR");

    // Check small acoustic model file exists
    tmp = (*env)->GetStringUTFChars(env, jenrollnetfile_UDT, 0);
    t->enrollnetfileName_UDT = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jenrollnetfile_UDT, tmp);
    if (!fileExists(t->enrollnetfileName_UDT)) THROW("jenrollnetfile_UDT file does not exist");

    /* Enroll Recognizer - used for recording and pipelined pronunciation extraction of enrollment phrases */
    if (!(t->enrollrecogEnroll_UDT = thfRecogCreateFromFile(t->thf,
                                                            t->enrollnetfileName_UDT, 0xFFFF, -1,
                                                            SDET))) THROW("thfRecogCreateFromFile");

    // configure enroll recognizer
    if (!thfRecogConfigSet(t->thf, t->enrollrecogEnroll_UDT, REC_LSILENCE,
                           lsil)) THROW("thfRecogConfigSet:REC_LSILENCE");
    if (!thfRecogConfigSet(t->thf, t->enrollrecogEnroll_UDT, REC_TSILENCE,
                           tsil)) THROW("thfRecogConfigSet:REC_TSILENCE");
    if (!thfRecogConfigSet(t->thf, t->enrollrecogEnroll_UDT, REC_MAXREC, maxR)) THROW(
            "thfRecogConfigSet:REC_MAXREC");
    if (!thfRecogConfigSet(t->thf, t->enrollrecogEnroll_UDT, REC_MINDUR,
                           minDur)) THROW("thfRecogConfigSet:REC_MINDUR");
    if (!thfRecogConfigSet(t->thf, t->enrollrecogEnroll_UDT, REC_LONGTERMMS,
                           ltms_udt)) THROW("thfRecogConfigSet:REC_LONGTERMMS");

    tmp = (*env)->GetStringUTFChars(env, jphsearchfile_UDT, 0);
    t->phsearchfileName_UDT = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jphsearchfile_UDT, tmp);
    if (!fileExists(t->phsearchfileName_UDT)) THROW("jphsearchfile_UDT file does not exist");

    if (!(t->searchEnroll_UDT = thfPhonemeSearchCreateFromFile(t->thf,
                                                               t->recogEnroll_UDT,
                                                               t->phsearchfileName_UDT,
                                                               NBEST))) THROW(
            "thfPhonemeSearchCreateFromFile");

#if (1)
    tmp = (*env)->GetStringUTFChars(env, judtsvsidfile_UDT, 0);
    t->udtsvsidfileName_UDT = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, judtsvsidfile_UDT, tmp);
    if (!fileExists(t->udtsvsidfileName_UDT)) THROW("judtsvsidfile_UDT file does not exist");
#else
    LOG("DEBUG ONLY:  USING HARDCODED UDTSID FILE ON SDCARD!!!!!!!");
    t->udtsvsidfileName_UDT = "/sdcard/sensory/udtsid/svsid_enUS_2_0.raw";
#endif

    // initialize phoneme recognizer
    if (!thfPhonemeRecogInit(t->thf, t->recogEnroll_UDT, t->searchEnroll_UDT,
                             RECOG_KEEP_WAVE)) THROW("thfPhonemeRecogInit");

    /* create and initialize the user structure */
    t->user_UDT = malloc(sizeof(thfUser_t) * MAX_UDT);
    memset(t->user_UDT, 0, sizeof(thfUser_t) * MAX_UDT);
    for (uIdx = 0; uIdx < MAX_UDT; uIdx++) {
        t->user_UDT[uIdx].userName = malloc(sizeof(char) * MAXSTR);
        memset(t->user_UDT[uIdx].userName, 0, sizeof(char) * MAXSTR);
        sprintf(t->user_UDT[uIdx].userName, "UDT-%i", uIdx + 1);
        t->user_UDT[uIdx].numEnroll = 0;
        t->user_UDT[uIdx].enroll = NULL;
    }

    /* create and initialize the CheckEnrollment pseudo-user structure */
    t->userCheckEnroll_UDT = malloc(sizeof(thfUser_t));
    memset(t->userCheckEnroll_UDT, 0, sizeof(thfUser_t));
    t->userCheckEnroll_UDT[0].userName = malloc(sizeof(char) * MAXSTR);
    memset(t->userCheckEnroll_UDT[0].userName, 0, sizeof(char) * MAXSTR);
    sprintf(t->userCheckEnroll_UDT[0].userName, "PSEUDO_UDT-1");
    t->userCheckEnroll_UDT[0].enroll = realloc(t->userCheckEnroll_UDT[0].enroll,
                                               sizeof(udtEnrollment_t));
    memset(&(t->userCheckEnroll_UDT[0].enroll[0]), 0, sizeof(udtEnrollment_t));
    t->userCheckEnroll_UDT[0].numEnroll = 1;

    // COMMON STUFF  ============================================

    t->svThresholdBase = jsvThresholdBase;
    t->svThresholdStep = jsvThresholdStep;

    return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    if (t->savedir) {
        free(t->savedir);
        t->savedir = NULL;
    }
    if (t->recogEnroll_UDT) {
        thfRecogDestroy(t->recogEnroll_UDT);
        t->recogEnroll_UDT = NULL;
    }
    if (t->enrollrecogEnroll_UDT) {
        thfRecogDestroy(t->enrollrecogEnroll_UDT);
        t->enrollrecogEnroll_UDT = NULL;
    }
    if (t->udt) {
        thfUdtDestroy(t->udt);
        t->udt = NULL;
    }
    PANIC("Panic: initRecog: %s: %s\n\n", ewhere, ewhat);
    return 0;
}


//=======================================================================================================
void Java_com_sprd_voicetrigger_commands_Recog_initRecog(JNIEnv *env, jobject job, jlong arg,
                                                         jlong mode) {
    trig_p *t = (trig_p *) (
            long) arg;
    const char *ewhere, *ewhat = NULL;

    /* Initialize recognizer */
    if (mode == 0) {
        LOG("Initializing Trigger Recognizer\n");
        if (!thfRecogInit(t->thf, t->trig_r, t->trig_s, RECOG_KEEP_WAVE_WORD)) THROW(
                "thfRecogInit");
        if (!thfRecogInit(t->thf, t->comm_r, t->comm_s, RECOG_KEEP_WAVE_WORD)) THROW(
                "thfRecogInit");
        thfRecogReset(t->thf, t->comm_r);
        thfRecogReset(t->thf, t->trig_r);
    } else {
        LOG("Initializing Command Recognizer\n");
        //if (!thfRecogInit(t->thf,t->comm_r,t->comm_s,RECOG_KEEP_WAVE_WORD)) THROW("thfRecogInit");
        //thfRecogReset(t->thf,t->comm_r);
        //if (!thfRecogPrepSeq(t->thf, t->comm_r, t->trig_r)) THROW("thfRecogInit");
        thfRecogPrepSeq(t->thf, t->comm_r, t->trig_r);
    }

    //  need a separate function for this!!!!
    //  if (!thfRecogInit(t->thf,t->comm_r,t->comm_s,RECOG_KEEP_WAVE_WORD)) THROW("thfRecogInit");

    return;

    error:
    if (!ewhat && t->thf) ewhat = thfGetLastError(t->thf);
    if (t->trig_s) thfSearchDestroy(t->trig_s);
    if (t->trig_r) thfRecogDestroy(t->trig_r);
    if (t->comm_s) thfSearchDestroy(t->comm_s);
    if (t->comm_r) thfRecogDestroy(t->comm_r);
    LOG("Panic - %s: %s\n\n", ewhere, ewhat);
    printf("[ hit Enter to exit ]");
    getchar();
    if (t->thf) thfSessionDestroy(t->thf);
    exit(1);
    return;
}

//===============================================================================
jlong Java_com_sprd_voicetrigger_commands_Recog_initSDK(JNIEnv *env, jobject job,
                                                        jstring jlogfile) {
    trig_p *t = memset(malloc(sizeof(trig_p)), 0, sizeof(trig_p));
    //jlong x;
    //const char *ewhere, *ewhat=NULL;
    //char str[STRSZ];
    if (jlogfile)t->logfile = (*env)->GetStringUTFChars(env, jlogfile, 0);
    t->result = NULL;

    long texp;
    //tnow = time(NULL);
    texp = thfGetLicenseExpiration();
    //LOG("Expiration time=%lx",texp);
    //LOG("Current time   =%lx",tnow);
    if ((texp > 0) && (texp < time(NULL))) return (-1);

    t->thf = thfSessionCreate();
    LOG("THF SDK VERSION=%s", thfVersion());
    LOG("t->thf=%lx", (long) t->thf);

    return (jlong)(long)t;
    /*
  error:
    if(!ewhat && t->sfs) ewhat=sfsGetLastError(t->sfs);
    LOG("Panic - %s: %s\n\n",ewhere,ewhat);
    printf("[ hit Enter to exit ]");
    getchar();
    if(t->sfs)   sfsSessionDestroy(t->sfs);
    exit(1);
    //JavaI2L(NULL,x);
    //return x;
    */

}

//========================================================================================================
jstring Java_com_sprd_voicetrigger_commands_Recog_pipeRecog(JNIEnv *env, jobject job, jlong arg,
                                                            jobject buffer, jlong mode) {
    trig_p *t = (trig_p *) (long) arg;
    //char str[STRSZ];
    //int i;
    short *sdata = (short *) (*env)->GetDirectBufferAddress(env, buffer);
    unsigned long sz = (unsigned long) (jlong)(*env)->GetDirectBufferCapacity(env, buffer) / 2;
    unsigned short status;
    const char *ewhere, *ewhat = NULL;
    const char *rres;
    float score;
    jstring res = NULL;
    //int energy=0;


#if SSDEBUG  // DEBUG
    unsigned long rate, bufSz, chunk=40;
    short *buf;
    LOG("LOADING WAVEFILE");
    if (!thfWaveFromFile(t->thf,"/sdcard/sensory/computer8k.wav" ,&buf,&bufSz,&rate))
          THROW("thfWaveFromFile");
    for (i=0; i<bufSz;i+=chunk) {
        sdata=&buf[i];
        sz=chunk;
        if(i+chunk>=bufSz) sz=bufSz-i;
        LOG("CHUNK=%i I=%i, BUFSZ=%i",chunk, i, bufSz);

        if(rate!=(unsigned)thfRecogGetSampleRate(t->thf,t->r)) {
           short *cspeech=NULL;
           unsigned long clen;
           if(!thfRecogSampleConvert(t->thf,t->r,rate,sdata,sz,&cspeech,&clen))
             THROW("recogSampleConvert");
           sdata=cspeech;
           sz=clen;
           LOG("Converted samplerate: %d -> %d", (int)rate,(int)sthfRecogGetSampleRate(t->thf,t->r));
         }

  #if 0
      FILE *fp=NULL;
      fp=fopen(DEBUGFILE,"ab");
      LOG("SAVE BUFFER: sz=%i",sz);
      fwrite(sdata,sizeof(short),sz,fp);
      fclose(fp);
  #endif

#endif

    if (mode == 0) {
        if (!thfRecogPipe(t->thf, t->trig_r, sz, sdata, RECOG_ONLY, &status)) THROW("recogPipe");
    } else {
        if (!thfRecogPipe(t->thf, t->comm_r, sz, sdata, RECOG_ONLY, &status)) THROW("recogPipe");
    }

    if (status == RECOG_DONE) {
        const char *walign, *palign;
        if (mode == 0) {
            if (!thfRecogResult(t->thf, t->trig_r, &score, &rres, &walign, SAVELOG ? &palign : NULL,
                                NULL, NULL, NULL, NULL)) THROW("thfRecogResult");
        } else {
            if (!thfRecogResult(t->thf, t->comm_r, &score, &rres, &walign, SAVELOG ? &palign : NULL,
                                NULL, NULL, NULL, NULL)) THROW("thfRecogResult");
        }
        LOG("***RESULT=%s score=%f", rres, score);
        if (rres) {
            t->result = realloc(t->result, strlen(rres) + 1);
            strcpy(t->result, rres);
            res = (*env)->NewStringUTF(env, t->result);
        }
        if (walign) LOG("***WORDALIGN=%s", walign);

        if (SAVELOG && t->logfile) {
            FILE *fp = fopen(t->logfile, "at");
            if (rres) fprintf(fp, "\n---\nresult: %s\n", rres);
            if (walign) fprintf(fp, "words: %s", walign);
            if (palign) fprintf(fp, "phonemes: %s", palign);
            LOG("wrote logfile: %s", t->logfile);
            fclose(fp);
        }

        //if (mode == 0) {
        //  if(!SAVELOG) thfRecogReset(t->thf,t->trig_r); // NOTE: this wipes result!
        //} else {
        //  if(!SAVELOG) thfRecogReset(t->thf,t->comm_r); // NOTE: this wipes result!
        //}
    }
    return res;
    error:
    if (!ewhat && t->thf) ewhat = thfGetLastError(t->thf);
    if (t->trig_s) thfSearchDestroy(t->trig_s);
    if (t->trig_r) thfRecogDestroy(t->trig_r);
    LOG("Panic - %s: %s\n\n", ewhere, ewhat);
    LOG("[ hit Enter to exit ]");
    getchar();
    if (t->thf) thfSessionDestroy(t->thf);
    exit(1);
    return NULL;
}


//===============================================================================================
void Java_com_sprd_voicetrigger_commands_Recog_setupRecog(JNIEnv *env, jobject job, jlong arg,
                                                          jstring jtrignetfile,
                                                          jstring jtrigsearchfile,
                                                          jstring jcommnetfile,
                                                          jstring jcommsearchfile) {
    trig_p *t = (trig_p *) (
            long) arg;
    const char *ewhere, *ewhat = NULL;
    //unsigned short set=PHRASESPOT_SET_DELAY;
    //unsigned short set=PHRASESPOT_SET_DELAY|PHRASESPOT_SET_LONGPEN;
    float delay = PHRASESPOT_DELAY;
    //float longpen=PHRASESPOT_LONGPEN;
    const char *trignetfile = (*env)->GetStringUTFChars(env, jtrignetfile, 0);
    LOG("trignetfile=%s", trignetfile);
    const char *trigsearchfile = (*env)->GetStringUTFChars(env, jtrigsearchfile, 0);
    LOG("trigsearchfile=%s", trigsearchfile);
    const char *commnetfile = (*env)->GetStringUTFChars(env, jcommnetfile, 0);
    LOG("commnetfile=%s", commnetfile);
    const char *commsearchfile = (*env)->GetStringUTFChars(env, jcommsearchfile, 0);
    LOG("commsearchfile=%s", commsearchfile);

#if SSDEBUG
    FILE *fp=fopen(DEBUGFILE,"wb"); fclose(fp);
#endif

    if (!t || !t->thf) THROW("Must call initSDK first");
    /* nuke existing data */
    if (t->trig_r) thfRecogDestroy(t->trig_r);
    t->trig_r = NULL;
    if (t->trig_s) thfSearchDestroy(t->trig_s);
    t->trig_s = NULL;
    if (t->comm_r) thfRecogDestroy(t->comm_r);
    t->comm_r = NULL;
    if (t->comm_s) thfSearchDestroy(t->comm_s);
    t->comm_s = NULL;

    // only need to do this one times
    printf("Loading recognizer...");
    if (!(t->trig_r = thfRecogCreateFromFile(t->thf, trignetfile, 0xFFFF, -1, SDET))) THROW(
            "thfRecogCreateFromFile");
    if (!(t->comm_r = thfRecogCreateFromFile(t->thf, commnetfile, 0xFFFF, -1, SDET))) THROW(
            "thfRecogCreateFromFile");

    /* Create search */
    printf("Loaded search...");
    LOG("%s\n", trigsearchfile);
    if (!(t->trig_s = thfSearchCreateFromFile(t->thf, t->trig_r, trigsearchfile, NBEST))) THROW(
            "thfSearchCreateFromFile");
    LOG("%s\n", commsearchfile);
    if (!(t->comm_s = thfSearchCreateFromFile(t->thf, t->comm_r, commsearchfile, NBEST))) THROW(
            "thfSearchCreateFromFile");

    /* Configure trig search */
    printf("Configuring trig phrasespot...");
    /*
    if (!sfsPhrasespotConfigure(t->sfs,t->trig_r,t->trig_s,set,NULL,NULL,NULL,&delay,&longpen))
        THROW("sfsPhrasespotConfigure");
     LOG("delay=%f",delay);
     LOG("longpen=%f",longpen);
    */
    if (!thfPhrasespotConfigSet(t->thf, t->trig_r, t->trig_s, PS_DELAY, delay)) THROW(
            "thfPhrasespotConfigure");
    //if (!sfsPhrasespotConfigSet(t->sfs, t->trig_r, t->trig_s, PS_LONGPEN, longpen))
    //  THROW("sfsPhrasespotConfigure");

    printf("Configuring trig phrasespot for swquential buffering...");
    if (!thfPhrasespotConfigSet(t->thf, t->trig_r, t->trig_s, PS_SEQ_BUFFER, 500)) THROW(
            "thfPhrasespotConfigSet");

    /* Configure comm search */
    /*
    printf("Configuring comm phrasespot...");
    if (!sfsPhrasespotConfigure(t->sfs,t->comm_r,t->comm_s,set,NULL,NULL,NULL,&delay,&longpen))
      THROW("sfsPhrasespotConfigure");
    LOG("delay=%f",delay);
    LOG("longpen=%f",longpen);
    */
    if (!thfPhrasespotConfigSet(t->thf, t->comm_r, t->comm_s, PS_DELAY, delay)) THROW(
            "sfsPhrasespotConfigure");
    //if (!sfsPhrasespotConfigSet(t->sfs, t->comm_r, t->comm_s, PS_LONGPEN, longpen))
    //  THROW("sfsPhrasespotConfigure");

    if (SAVELOG) {
        FILE *fp = fopen(t->logfile, "wt");
        LOG("Wiping log file: %s", t->logfile);
        fclose(fp);
    }
    return;

    error:
    if (!ewhat && t->thf) ewhat = thfGetLastError(t->thf);
    if (t->trig_s) thfSearchDestroy(t->trig_s);
    if (t->trig_r) thfRecogDestroy(t->trig_r);
    if (t->comm_s) thfSearchDestroy(t->comm_s);
    if (t->comm_r) thfRecogDestroy(t->comm_r);
    LOG("Panic - %s: %s\n\n", ewhere, ewhat);
    printf("[ hit Enter to exit ]");
    getchar();
    if (t->thf) thfSessionDestroy(t->thf);
    exit(1);
    return;
}


// ============================================================================
// Configure UDT/SID mode which determines whether speaker verification gets performed
int Java_com_sprd_voicetrigger_nativeinterface_Recog_setMode(JNIEnv *env,
                                                             jobject job, jlong arg,
                                                             jshort udtMode) {
    trig_t *t = (trig_t *) (long) arg;
    t->udtMode = udtMode;
    t->svMode = udtMode;
    return 1;
}

// ============================================================================
// Loads test recognizer from file
int loadTestRecog(trig_t *t) {
    char *filename1 = malloc(MAXFILENAME);
    char *filename2 = malloc(MAXFILENAME);
    const char *ewhere, *ewhat = NULL;

    if (t->recogTest_COMB)
        thfRecogDestroy(t->recogTest_COMB);
    t->recogTest_COMB = NULL;
    if (t->searchTest_COMB)
        thfSearchDestroy(t->searchTest_COMB);
    t->searchTest_COMB = NULL;

    // Read in SID-specific recognizer and search from file
    sprintf(filename2, "%s%s", t->savedir, RECOGFILE);
    if (!(t->recogTest_COMB = thfRecogCreateFromFile(t->thf, filename2, 0xFFFF,
                                                     -1, NO_SDET))) THROW("thfRecogCreateFromFile");

    sprintf(filename1, "%s%s", t->savedir, SEARCHFILE);
    if (!(t->searchTest_COMB = thfSearchCreateFromFile(t->thf,
                                                       t->recogTest_COMB, filename1, NBEST))) THROW(
            "thfSearchCreateFromFile");

    if (!thfRecogConfigSet(t->thf, t->recogTest_COMB, REC_EPQ_ENABLE,
                           DONT_USE_EPQ)) THROW("thfRecogConfigSet:REC_EPQ_ENABLE");

    thfPhrasespotConfigSet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_DELAY, 150.);

#if (FULL_DIAGNOSTICS > 0)
    float temp;
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_PARAMA_OFFSET, &temp);
    LOG("SCH_PRUNING=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_BEAM, &temp);
    LOG("PS_BEAM=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_ABSBEAM, &temp);
    LOG("PS_ABSBEAM=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_DELAY, &temp);
    LOG("PS_DELAY=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_LONGPEN, &temp);
    LOG("PS_LONGPEN=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_SEQ_BUFFER, &temp);
    LOG("PS_SEQ_BUFFER=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_SEQ_SPOT_OPTIONAL, &temp);
    LOG("PS_SEQ_SPOT_OPTIONAL=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_SEQ_SPOT_INCLUDED, &temp);
    LOG("PS_SEQ_SPOT_INCLUDED=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_SEQ_SAMPLES, &temp);
    LOG("PS_SEQ_SAMPLES=%f\n", temp);
#endif

    if (!thfRecogInit(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                      RECOG_KEEP_WAVE)) THROW("thfRecogInit");
    free(filename1);
    free(filename2);
    return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    free(filename1);
    free(filename2);
    PANIC("Panic: loadRecog: %s: %s\n\n", ewhere, ewhat);
    return (0);
}

// ============================================================================
void Java_com_sprd_voicetrigger_nativeinterface_Recog_setEPQ(JNIEnv *env,
                                                             jobject job, jlong arg,
                                                             jshort jstate) {
    trig_t *t = (trig_t *) (long) arg;
    char *filename1 = malloc(MAXFILENAME);
    char *filename2 = malloc(MAXFILENAME);
    const char *ewhere, *ewhat = NULL;

    // disable EPQ 1-22-14 EBA
//  if (jstate) {
//    if (!thfRecogConfigSet(t->thf, t->recogTest_COMB, REC_EPQ_ENABLE, USE_EPQ))
//      THROW("thfRecogConfigSet: epq:true");
//    if (!thfRecogConfigSet(t->thf, t->recogTest_COMB, REC_EPQ_MIN, EPQ_DEFAULT_MIN))
//      THROW("thfRecogConfigSet: epq:true");
//    LOG("setEPQ: epq = on");
//  } else {
    if (!thfRecogConfigSet(t->thf, t->recogTest_COMB, REC_EPQ_ENABLE,
                           DONT_USE_EPQ)) THROW("thfRecogConfigSet:REC_EPQ_ENABLE");
    LOG("setEPQ: epq = off");
//  }
    return;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    free(filename1);
    free(filename2);
    PANIC("Panic: setEPQ: %s: %s\n\n", ewhere, ewhat);
    return;
}

// ============================================================================
// Enrollment phase: Feeds supplied audio buffer thru the recognition pipeline.
// Returned state indicates whether a recognition result is available.
jint Java_com_sprd_voicetrigger_nativeinterface_Recog_pipeRecogEnroll(
        JNIEnv *env, jobject job, jlong arg, jobject buffer, jshort uIdx) {
    trig_t *t = (trig_t *) (long) arg;
    short *sdata = (short *) (*env)->GetDirectBufferAddress(env, buffer);
    unsigned long sz = (unsigned long) (jlong)(*env)->GetDirectBufferCapacity(
            env, buffer) / 2;
    unsigned short status;
    const char *ewhere, *ewhat = NULL;

    if (uIdx <= MAX_EFT) {
        if (!t->recogEnrollSD_EFT) THROW("No recogEnrollSD_EFT recog exists");
        if (!thfRecogPipe(t->thf, t->recogEnrollSD_EFT, sz, sdata, SDET_ONLY,
                          &status)) THROW("thfRecogPipe:EFT");
    } else {
        if (!t->recogEnroll_UDT) THROW("No recogEnroll_UDT recog exists");
        if (!thfRecogPipe(t->thf, t->recogEnroll_UDT, sz, sdata, SDET_RECOG,
                          &status)) THROW("thfRecogPipe:UDT");
    }
    if (status == RECOG_DONE || status == RECOG_SILENCE
        || status == RECOG_MAXREC || status == RECOG_IGNORE)
        return 2; // recog result available
    else
        return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    PANIC("Panic: pipeRecogEnroll: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
// Test phase: Feeds supplied audio buffer thru the recognition pipeline.
// Returned state indicates whether a recognition result is available.
jint Java_com_sprd_voicetrigger_nativeinterface_Recog_pipeRecogTest(JNIEnv *env,
                                                                    jobject job, jlong arg,
                                                                    jobject buffer) {
    trig_t *t = (trig_t *) (long) arg;
    short *sdata = (short *) (*env)->GetDirectBufferAddress(env, buffer);
    unsigned long sz = (unsigned long) (jlong)(*env)->GetDirectBufferCapacity(
            env, buffer) / 2;
    unsigned short status;
    const char *ewhere, *ewhat = NULL;

    if (!t->recogTest_COMB) THROW("recogTest_COMB is null");
    if (!thfRecogPipe(t->thf, t->recogTest_COMB, sz, sdata, RECOG_ONLY,
                      &status)) THROW("thfRecogPipe");
    if (status == RECOG_DONE || status == RECOG_SILENCE
        || status == RECOG_MAXREC || status == RECOG_IGNORE)
        return 2; // recog result available
    else
        return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    PANIC("Panic: pipeRecogTest: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
// Enrollment phase: retrieves the recognition result (i.e., input audio) and saves data to file.
// Brian
// Success - return 0
// No speech - return 0x8000
// Recording not equal to FT - return 0x4000
// Other error - return 0xFFFF
// Recording quality issue return 
// ENERGY_MIN         (1 << 0)
// ENERGY_STD_DEV     (1 << 1)
// SIL_BEG_MSEC       (1 << 2)
// SIL_END_MSEC       (1 << 3)
// SNR                (1 << 4)
// RECORDING_VARIANCE (1 << 5)
// CLIPPING_PERCENT   (1 << 6)
// CLIPPING_MSEC      (1 << 7)
// POOR_RECORDINGS    (1 << 8)
// SPOT               (1 << 9)
// VOWEL_DUR          (1 << 10)
// REPETITION         (1 << 11)

int Java_com_sprd_voicetrigger_nativeinterface_Recog_getResultEnroll(
        JNIEnv *env, jobject job, jlong arg, jshort eIdx, jshort uIdx) {
    trig_t *t = (trig_t *) (long) arg;
    const char *ewhere, *ewhat = NULL;
    const char *rres = NULL;
    float score;
    char *filename = malloc(MAXFILENAME);
    const short *inSpeech, *sdetSpeech;
    unsigned long inSpeechLen, sdetSpeechLen, inLen;
    unsigned short status;
    int currE, newE, j;
    unsigned int blockSize = BLOCKSZ;
    unsigned long globalFeedback;

    LOG("getResultEnroll:initial entry:user=%i, eIdx=%i", uIdx, eIdx);

    if (uIdx <= MAX_EFT) {

        // EFT STUFF  ============================================

        uIdx--;
        eIdx--;
        LOG("getResultEnroll: user=%i, eIdx=%i", uIdx, eIdx);
        if (!t->recogEnrollSD_EFT) THROW("recogEnrollSD_EFT is null");
        if (!thfRecogResult(t->thf, t->recogEnrollSD_EFT, &score, &rres, NULL,
                            NULL, &inSpeech, &inSpeechLen, &sdetSpeech, &sdetSpeechLen)) THROW(
                "thfRecogResult");

        LOG("thfRecogResult:score = %i, rres = %s, sdetSpeechLen = %li\n",
            score, rres, sdetSpeechLen);

        if (sdetSpeechLen > 0) {

            // check individual recording. Provides per recording feedback.
            if (!thfCheckRecording(t->thf, t->recogEnrollSD_EFT,
                                   (short *) sdetSpeech, sdetSpeechLen, &globalFeedback)) THROW(
                    "thfCheckRecording");
            //RFP - LOG("thfCheckRecording: global feedback = %lx \n",globalFeedback);

            if (globalFeedback) {
                if (!thfRecogReset(t->thf, t->recogEnrollSD_EFT)) THROW(
                        "thfRecogReset:globalfeedback");
                LOG("getResultEnroll:enroll error=%i (%s)\n",
                    (int) globalFeedback, checkFlagsText(globalFeedback));
                free(filename);
                return globalFeedback;
            }

            // make sure we have memory enough to store in index eIdx
            if (t->user_EFT[uIdx].numEnroll <= eIdx) {
                t->user_EFT[uIdx].enroll = realloc(t->user_EFT[uIdx].enroll,
                                                   sizeof(udtEnrollment_t) * (eIdx + 1));
                currE = t->user_EFT[uIdx].numEnroll;
                newE = (eIdx + 1) - currE;
                memset(&(t->user_EFT[uIdx].enroll[currE]), 0,
                       sizeof(udtEnrollment_t) * newE);
                t->user_EFT[uIdx].numEnroll = eIdx + 1;
            }

            // Feed buffer through phrase spotter
            rres = NULL;
            for (j = 0; j < sdetSpeechLen; j += blockSize) {
                if ((inLen = sdetSpeechLen - j) > blockSize)
                    inLen = blockSize;
                if (!thfRecogPipe(t->thf, t->recogEnrollNoSD_EFT, inLen,
                                  (short *) &sdetSpeech[j], RECOG_ONLY, &status)) THROW(
                        "thfRecogPipe");
                if (status == RECOG_DONE) {
                    if (!thfRecogResult(t->thf, t->recogEnrollNoSD_EFT, &score,
                                        &rres, NULL, NULL, NULL, NULL, NULL, NULL)) THROW(
                            "thfRecogResult");
                    LOG("***ENROLL: res=%s (score=%f)\n", rres, score);
                    break;
                }
            }

            if (!rres) {
                LOG("***ENROLL: Incorrect phrase (say 'hello blue genie')\n");
                if (!thfRecogReset(t->thf, t->recogEnrollSD_EFT)) THROW(
                        "thfRecogReset:incorrect_phrase:t->recogEnrollSD_EFT");
                if (!thfRecogReset(t->thf, t->recogEnrollNoSD_EFT)) THROW(
                        "thfRecogReset:incorrect_phrase:t->recogEnrollNoSD_EFT");
                free(filename);
                return 1 << 15;
            }

            // Copy and save audio
            t->user_EFT[uIdx].enroll[eIdx].audio = realloc(
                    t->user_EFT[uIdx].enroll[eIdx].audio,
                    sdetSpeechLen * sizeof(short));
            memcpy(t->user_EFT[uIdx].enroll[eIdx].audio, sdetSpeech,
                   sdetSpeechLen * sizeof(short));
            t->user_EFT[uIdx].enroll[eIdx].audioLen = sdetSpeechLen;
            if (sdetSpeech && t->savedir) {
                sprintf(filename, "%seft%i_%i.wav", t->savedir, uIdx, eIdx);
                if (!thfWaveSaveToFile(t->thf, filename, (short *) sdetSpeech,
                                       sdetSpeechLen, 16000)) THROW(
                        "thfWaveSaveToFile:user#_#.wav");
                LOG("SAVED: samples=%li file=%s", sdetSpeechLen, filename);
            }

            // check for too long vowel duration (EFT only!)
            //========================================================================================================
            //========================================================================================================
            //========================================================================================================
            //RFP - LOG("getResultsEnroll:check too long vowel duration");
            float tempVD[1] = {0};

            //RFP - LOG("getResultsEnroll:Feed buffer through phrase spotter");

            // Feed buffer through phrase spotter
            //thfRecogInit(t->thf, t->recogCheckEnroll_EFT, t->searchCheckEnroll_EFT, RECOG_KEEP_WAVE);
            rres = NULL;
            for (j = 0; j < sdetSpeechLen; j += blockSize) {
                if ((inLen = sdetSpeechLen - j) > blockSize)
                    inLen = blockSize;
                if (!thfRecogPipe(t->thf, t->recogCheckEnroll_EFT, inLen,
                                  (short *) &sdetSpeech[j], RECOG_ONLY, &status)) THROW(
                        "thfRecogPipe");
                if (status == RECOG_DONE)
                    break;
            }
            if (!thfRecogReset(t->thf, t->recogCheckEnroll_EFT)) THROW(
                    "thfRecogReset:long_vowels:t->recogCheckEnroll_EFT:pre_check");

            //RFP - LOG("getResultsEnroll:Copy and save audio");

            // Copy and save audio
            t->userCheckEnroll_EFT[0].enroll[0].audio = realloc(
                    t->userCheckEnroll_EFT[0].enroll[0].audio,
                    sdetSpeechLen * sizeof(short));
            memcpy(t->userCheckEnroll_EFT[0].enroll[0].audio, sdetSpeech,
                   sdetSpeechLen * sizeof(short));
            t->userCheckEnroll_EFT[0].enroll[0].audioLen = sdetSpeechLen;

            //RFP - LOG("getResultsEnroll:Check Enrollments");

            if (t->feedback.details)
                free(t->feedback.details);
            t->feedback.details = NULL;
            t->feedback.details = malloc(sizeof(unsigned long));
            if (!(thfSpeakerCheckEnrollments(t->thf, t->recogCheckEnroll_EFT,
                                             t->userCheckEnroll_EFT, 0, &t->feedback.general,
                                             t->feedback.details,
                                             &t->feedback.phraseQuality))) THROW(
                    "thfSpeakerCheckEnrollments");

            if (!thfRecogReset(t->thf, t->recogCheckEnroll_EFT)) THROW(
                    "thfRecogReset:long_vowels:t->recogCheckEnroll_EFT:post_check");

            if (tempVD[0] > t->checkVowelDur_EFT) {
                if (!thfRecogReset(t->thf, t->recogEnrollSD_EFT)) THROW(
                        "thfRecogReset:long_vowels:t->recogEnrollSD_EFT");
                if (!thfRecogReset(t->thf, t->recogEnrollNoSD_EFT)) THROW(
                        "thfRecogReset:long_vowels:t->recogEnrollNoSD_EFT");

                LOG("getResultEnroll:enroll error=0x4000\n");
                free(filename);
                return CHECKRECORDING_BITFIELD_VOWEL_DUR;
            }

            //========================================================================================================
            //========================================================================================================
            //========================================================================================================

        } else {
            LOG("WARNING: NO SPEECH DETECTED");
            free(filename);
            if (!thfRecogReset(t->thf, t->recogEnrollSD_EFT)) THROW(
                    "thfRecogReset:no_speech:t->recogEnrollSD_EFT");
            if (!thfRecogReset(t->thf, t->recogEnrollNoSD_EFT)) THROW(
                    "thfRecogReset:no_speech:t->recogEnrollNoSD_EFT");
            //RFP - LOG("getResultEnroll:enroll error=0x8000\n");
            return 1 << 14;

        }

        if (!thfRecogReset(t->thf, t->recogEnrollSD_EFT)) THROW(
                "thfRecogReset:success:t->recogEnrollSD_EFT");
        if (!thfRecogReset(t->thf, t->recogEnrollNoSD_EFT)) THROW(
                "thfRecogReset:success:t->recogEnrollNoSD_EFT");
        free(filename);
        return 0;

    } else {

        // UDT STUFF  ============================================

        uIdx -= MAX_EFT;
        uIdx--;
        eIdx--;
        LOG("getResultEnroll:UDT entry:user=%i, eIdx=%i", uIdx, eIdx);
        if (!t->recogEnroll_UDT) THROW("recogEnroll_UDT is null");
        if (!thfRecogResult(t->thf, t->recogEnroll_UDT, &score, &rres, NULL,
                            NULL, &inSpeech, &inSpeechLen, &sdetSpeech, &sdetSpeechLen)) THROW(
                "thfRecogResult");
        LOG("getResultEnroll:rres=%s\n", rres);

        if (rres && sdetSpeechLen > 0) {

            // check individual recording. Provides per recording feedback.
            if (!thfCheckRecording(t->thf, t->recogEnroll_UDT,
                                   (short *) sdetSpeech, sdetSpeechLen, &globalFeedback)) THROW(
                    "thfCheckRecording");

            //if (globalFeedback) {
            //  LOG("getResultEnroll:enroll error=%i (%s)\n", (int)globalFeedback, checkFlagsText(globalFeedback));
            //  if (!thfRecogReset(t->thf, t->recogEnroll_UDT))
            //    THROW("thfRecogReset:globalfeedback");
            //  free(filename);
            //  return globalFeedback;
            //}

            // make sure we have memory enough to store in index eIdx
            if (t->user_UDT[uIdx].numEnroll <= eIdx) {
                t->user_UDT[uIdx].enroll = realloc(t->user_UDT[uIdx].enroll,
                                                   sizeof(udtEnrollment_t) * (eIdx + 1));
                currE = t->user_UDT[uIdx].numEnroll;
                newE = (eIdx + 1) - currE;
                memset(&(t->user_UDT[uIdx].enroll[currE]), 0,
                       sizeof(udtEnrollment_t) * newE);
                t->user_UDT[uIdx].numEnroll = eIdx + 1;
                t->user_UDT[uIdx].enroll[eIdx].pronun = NULL;
                t->user_UDT[uIdx].enroll[eIdx].reserved1 = NULL;
            }

#if (0)
            // Store pipelined pronunciation to save recomputing during enrollment
            // we don't do this anymore
            t->user_UDT[uIdx].enroll[eIdx].pronun = malloc(sizeof(char) * (strlen(rres)+1));
            strcpy(t->user_UDT[uIdx].enroll[eIdx].pronun, rres);
#endif

            // Copy and save audio
            t->user_UDT[uIdx].enroll[eIdx].audio = realloc(
                    t->user_UDT[uIdx].enroll[eIdx].audio,
                    sdetSpeechLen * sizeof(short));
            memcpy(t->user_UDT[uIdx].enroll[eIdx].audio, sdetSpeech,
                   sdetSpeechLen * sizeof(short));
            t->user_UDT[uIdx].enroll[eIdx].audioLen = sdetSpeechLen;

            if (sdetSpeech && t->savedir) {
                sprintf(filename, "%sudt%i_%i.wav", t->savedir, uIdx, eIdx);
                if (!thfWaveSaveToFile(t->thf, filename, (short *) sdetSpeech,
                                       sdetSpeechLen, 16000)) THROW(
                        "thfWaveSaveToFile: user#_#.wav");
                LOG("SAVED: samples=%li file=%s", sdetSpeechLen, filename);
            }

#if (FULL_DIAGNOSTICS > 0)
            sprintf(filename, "%sudt%i_%i.wav", t->savedir, uIdx + 1, eIdx + 1);
            if (!thfWaveSaveToFile(t->thf, filename, (short *) sdetSpeech,
                                   sdetSpeechLen, 16000))THROW("thfWaveSaveToFile: user#_#.wav");
            LOG("UDT SAVED TO SDCARD: samples=%li file=%s",
                t->user_UDT[uIdx].enroll[eIdx].audioLen, filename);
#endif

            //========================================================================================================
            //========================================================================================================
            //========================================================================================================

            // Copy and save audio
            t->userCheckEnroll_UDT[0].enroll[0].audio = realloc(
                    t->userCheckEnroll_UDT[0].enroll[0].audio,
                    sdetSpeechLen * sizeof(short));
            memcpy(t->userCheckEnroll_UDT[0].enroll[0].audio, sdetSpeech,
                   sdetSpeechLen * sizeof(short));
            t->userCheckEnroll_UDT[0].enroll[0].audioLen = sdetSpeechLen;

            if (t->udtCheckEnroll_UDT) {
                thfUdtDestroy(t->udtCheckEnroll_UDT);
                t->udtCheckEnroll_UDT = NULL;
            }

            t->udtCheckEnroll_UDT = thfUdtCreate(t->thf,
                                                 t->enrollnetfileName_UDT, t->phsearchfileName_UDT,
                                                 t->udtsvsidfileName_UDT, 1, SAMPLERATE);
            if (t->udtCheckEnroll_UDT == NULL) THROW("udtCheckEnroll_UDT is null");

            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT,
                                 UDT_PARAM_A_START, t->paramAStart_UDT)) THROW(
                    "thfUdtConfigSet:UDT_PARAM_A_START");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT,
                                 UDT_PARAM_B_START, t->paramBStart_UDT)) THROW(
                    "thfUdtConfigSet:UDT_PARAM_B_START");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT,
                                 UDT_SAMP_PER_CAT, 128)) THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT,
                                 UDT_SAMP_PER_CAT_WITHIN, 64)) THROW(
                    "thfUdtConfigSet:UDT_SAMP_PER_CAT_WITHIN");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT,
                                 UDT_NOISE_PERCENT, 5.0)) THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT, UDT_SIL_PERCENT,
                                 50.0)) THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT, UDT_USE_FEAT,
                                 1)) THROW("thfUdtConfigSet:UDT_USE_FEAT");
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT, UDT_IGNORE_TYPE,
                                 0)) THROW("thfUdtConfigSet:UDT_IGNORE_TYPE");

            // 2015-04-02 added this.
            if (!thfUdtConfigSet(t->thf, t->udtCheckEnroll_UDT, UDT_CHECK_SNR,
                                 t->checkSNR_UDT)) THROW("thfUdtConfigSet:UDT_CHECK_SNR");

            /* check enrollment for the user in question */
            if (t->feedback.details)
                free(t->feedback.details);
            t->feedback.details = NULL;
            t->feedback.details = malloc(sizeof(unsigned long));

            if (!(thfUdtCheckEnrollments(t->thf, t->udtCheckEnroll_UDT,
                                         t->userCheckEnroll_UDT, 0, &t->feedback.general,
                                         t->feedback.details, &t->feedback.phraseQuality))) THROW(
                    "thfUdtCheckEnrollments");

#if (FULL_DIAGNOSTICS > 0)
            float temp[1];
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_ENERGY_MIN, &temp[0], 1);
            LOG("UDT_CHECK_ENERGY_MIN=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_ENERGY_STD_DEV, &temp[0], 1);
            LOG("UDT_CHECK_ENERGY_STD_DEV=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_SIL_BEG_MSEC, &temp[0], 1);
            LOG("UDT_CHECK_SIL_BEG_MSEC=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_SIL_END_MSEC, &temp[0], 1);
            LOG("UDT_CHECK_SIL_END_MSEC=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT, UDT_CHECK_SNR,
                                &temp[0], 1);
            LOG("UDT_CHECK_SNR=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_RECORDING_VARIANCE, &temp[0], 1);
            LOG("UDT_CHECK_RECORDING_VARIANCE=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_CLIPPING_PERCENT, &temp[0], 1);
            LOG("UDT_CHECK_CLIPPING_PERCENT=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_CLIPPING_MSEC, &temp[0], 1);
            LOG("UDT_CHECK_CLIPPING_MSEC=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_POOR_RECORDINGS, &temp[0], 1);
            LOG("UDT_CHECK_POOR_RECORDINGS=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_VOWEL_DUR, &temp[0], 1);
            LOG("UDT_CHECK_VOWEL_DUR=%f\n", temp[0]);
            thfUdtCheckValueGet(t->thf, t->udtCheckEnroll_UDT,
                                UDT_CHECK_REPETITION, &temp[0], 1);
            LOG("UDT_CHECK_REPETITION=%f\n", temp[0]);
#endif

            thfUdtDestroy(t->udtCheckEnroll_UDT);
            t->udtCheckEnroll_UDT = NULL;

            if (t->feedback.general) {
                LOG("getResultEnroll:feedback.general=%i (%s)\n",
                    (int) t->feedback.general,
                    checkFlagsText(t->feedback.general));
                if (!thfRecogReset(t->thf, t->recogEnroll_UDT)) THROW(
                        "thfRecogReset:feedback.general");
                free(filename);
                return t->feedback.general;
            }

            //========================================================================================================
            //========================================================================================================
            //========================================================================================================

        } else {
            LOG("WARNING: NO SPEECH DETECTED");
            free(filename);
            if (!thfRecogReset(t->thf, t->recogEnroll_UDT)) THROW("thfRecogReset:no_speech");
            return 1 << 14;
        }

        if (!thfRecogReset(t->thf, t->recogEnroll_UDT)) THROW("thfRecogReset:success");
        free(filename);
        return 0;

    }

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    free(filename);
    PANIC("Panic: getResultEnroll: %s: %s\n\n", ewhere, ewhat);
    return 1 << 13;
}

// ============================================================================
// Test phase: retrieves the recognition result and performs speaker verification if needed.
jstring Java_com_sprd_voicetrigger_nativeinterface_Recog_getResultTest(
        JNIEnv *env, jobject job, jlong arg, jint sensitivityIdx,
        jstring jappDir) {
    trig_t *t = (trig_t *) (long) arg;
    const char *ewhere, *ewhat = NULL;
    const char *rres = NULL;
    const char *appDir;
    const char *tmp;
    float score;
    jstring res = NULL;
    const short *inSpeech, *sdetSpeech;
    unsigned long inSpeechLen, sdetSpeechLen;
    int i;
    char *filename = malloc(MAXFILENAME);
    int userIdx = 0;
    static int recogIdx = 0;
    float svLevel[7]; // Sensitivity levels

    tmp = (*env)->GetStringUTFChars(env, jappDir, 0);
    appDir = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jappDir, tmp);

    for (i = 0; i < 7; i++) {
        svLevel[i] = t->svThresholdBase + (t->svThresholdStep * (i - 3));
        // LOG("svLevel[%i]=%f", i, svLevel[i]);
    }

    if (!t->recogTest_COMB) THROW("recogTest_COMB is null");
    if (!thfRecogResult(t->thf, t->recogTest_COMB, &score, &rres, NULL, NULL,
                        &inSpeech, &inSpeechLen, &sdetSpeech, &sdetSpeechLen)) THROW(
            "thfRecogResult");
    LOG("RESULT=%s score=%f", rres, score);
    if (t->udtMode == MODE_UDT) { // UDT
        if (rres) {
            res = (*env)->NewStringUTF(env, "trigger");
        }
    } else { // SID
        char str[32];
        float svScore = thfRecogSVscore(t->thf, t->recogTest_COMB);
        LOG("SV: score=%f, %f, threshold=%f", svScore, 8192.f * svScore,
            svLevel[sensitivityIdx]);
        if (svScore >= svLevel[sensitivityIdx]) {
            sprintf(str, "Accept\n%s", rres);
            LOG("ACCEPTED %s\n", rres);
        } else {
            sprintf(str, "Reject\n%s", rres);
            LOG("REJECTED %s\n", rres);
        }
        LOG("str=%s", str);
        res = (*env)->NewStringUTF(env, str);
    }

    // save first 100 recordings for debugging
    if (inSpeech != NULL && inSpeechLen > 0 && appDir != NULL
        && recogIdx < 100) {
        userIdx = 0;
        sscanf(rres, "User-%d", &userIdx);
        sprintf(filename, "%s/recog%i_%i.wav", appDir, userIdx, recogIdx);
        recogIdx += 1;
        if (!thfWaveSaveToFile(t->thf, filename, (short *) inSpeech, inSpeechLen,
                               16000)) THROW("thfWaveSaveToFile:user#_#.wav");
        LOG("SAVED TEST WAVEFORM TO SDCARD: samples=%li file=%s", inSpeechLen,
            filename);
    }

    thfRecogReset(t->thf, t->recogTest_COMB);
    free(filename);
    return res;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    free(filename);
    PANIC("Panic: getResultTest: %s: %s\n\n", ewhere, ewhat);
    return (*env)->NewStringUTF(env, "panic");
}

// ============================================================================
int Java_com_sprd_voicetrigger_nativeinterface_Recog_configTriggerLevel(
        JNIEnv *env, jobject job, jlong arg, jshort pos, jshort type) {
    trig_t *t = (trig_t *) (long) arg;
    int i;
    const char *ewhere, *ewhat = NULL;
    float triggerLevel[7]; // Sensitivity levels
    float curOffset = 0.f;
    //const char *appDir = NULL;
    //const char *dspTarget = NULL;
    //char *filename1 = malloc(MAXFILENAME);
    //char *filename2 = malloc(MAXFILENAME);

    if (type == 0) {
        for (i = 0; i < 7; i++) {
            triggerLevel[i] = EFT_PARAMAOFFSET
                              - (EFT_PARAMAOFFSET_STEP * (i - 3));
        }
    } else {
        for (i = 0; i < 7; i++) {
            triggerLevel[i] = UDT_PARAMAOFFSET
                              - (UDT_PARAMAOFFSET_STEP * (i - 3));
        }
    }

    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_PARAMA_OFFSET, &curOffset);

    // NOTE: offset is accumulative so adjusts by the difference
    LOG("thfPhrasespotConfigGet: Current=%f, want=%f, diff=%f", curOffset,
        triggerLevel[pos], triggerLevel[pos] - curOffset);
    if (!thfPhrasespotConfigSet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                                PS_PARAMA_OFFSET, triggerLevel[pos] - curOffset)) THROW(
            "thfPhrasespotConfigSet");

    thfPhrasespotConfigGet(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                           PS_PARAMA_OFFSET, &curOffset);
    LOG("trigger level = %f\n", curOffset);

    return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    PANIC("Panic: configTriggerLevel: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
int Java_com_sprd_voicetrigger_nativeinterface_Recog_configEnroll(JNIEnv *env,
                                                                  jobject job, jlong arg,
                                                                  jshort requireSil) {
    trig_t *t = (trig_t *) (long) arg;
    const char *ewhere, *ewhat = NULL;

    if (!t->udt)
        return 1;

    // requireSilence is used by thfCheckRecordings and() thfUdtEnroll()
    if (!thfUdtConfigSet(t->thf, t->udt, UDT_REQUIRE_SILENCE, requireSil)) THROW("thfUdtConfigSet");
    return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    PANIC("Panic: configEnroll: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
// Check newly made recordings. Generates feedback details. Call getFeedbackDetails() and getPhraseQuality() to retrieve.
int Java_com_sprd_voicetrigger_nativeinterface_Recog_checkNewRecordings(
        JNIEnv *env, jobject job, jlong arg, jshort uIdx) {
    trig_t *t = (trig_t *) (long) arg;
    const char *ewhere, *ewhat = NULL;
    short tempIdx;
    //struct timeb tm1, tm2;
    struct timeval tm1, tm2;
    struct timezone tz;

    LOG("checkNewRecordings:entry\n");

    if (uIdx < MAX_EFT) {

        // EFT STUFF  ============================================

#if (FULL_DIAGNOSTICS > 0)
        short eIdx;
        LOG("t->user_EFT[%i].userName=%s\n", uIdx, t->user_EFT[uIdx].userName);
        LOG("t->user_EFT[%i].tag=%s\n", uIdx, t->user_EFT[uIdx].tag);
        LOG("t->user_EFT[%i].numEnroll=%i\n", uIdx, t->user_EFT[uIdx].numEnroll);
        LOG("t->user_EFT[%i].reserved1=%s\n", uIdx, t->user_EFT[uIdx].reserved1);
        LOG("t->user_EFT[%i].reserved2=%s\n", uIdx, t->user_EFT[uIdx].reserved2);
        for (eIdx = 0; eIdx < t->user_EFT[uIdx].numEnroll; eIdx++) {
            LOG("t->user_EFT[%i].enroll[%i].audio[0]=%i\n", uIdx, eIdx,
                t->user_EFT[uIdx].enroll[eIdx].audio[0]);
            LOG("t->user_EFT[%i].enroll[%i].audioLen=%li\n", uIdx, eIdx,
                t->user_EFT[uIdx].enroll[eIdx].audioLen);
            LOG("t->user_EFT[%i].enroll[%i].audioSignature=%li\n", uIdx, eIdx,
                t->user_EFT[uIdx].enroll[eIdx].audioSignature);
            LOG("t->user_EFT[%i].enroll[%i].filename=%s\n", uIdx, eIdx,
                t->user_EFT[uIdx].enroll[eIdx].filename);
            LOG("t->user_EFT[%i].enroll[%i].reserved1=%s\n", uIdx, eIdx,
                t->user_EFT[uIdx].enroll[eIdx].reserved1);
            LOG("t->user_EFT[%i].enroll[%i].pronun=%s\n", uIdx, eIdx,
                t->user_EFT[uIdx].enroll[eIdx].pronun);
        }
#endif

        if (t->feedback.details)
            free(t->feedback.details);
        t->feedback.details = NULL;
        t->feedback.details = malloc(sizeof(unsigned long) * t->user_EFT[uIdx].numEnroll);
        //ftime(&tm1);
        gettimeofday(&tm1, &tz);
        if (!(thfSpeakerCheckEnrollments(t->thf, t->recogEnrollNoSD_EFT,
                                         t->user_EFT, uIdx, &t->feedback.general,
                                         t->feedback.details,
                                         &t->feedback.phraseQuality)))
                                          THROW("thfSpeakerCheckEnrollments");
        //ftime(&tm2);
        gettimeofday(&tm2, &tz);
        if (uIdx >= t->numUsers_EFT)
            t->numUsers_EFT = uIdx + 1; // added for v3.12.0
        //LOG("CHECK ENROLL TIME=%li", (tm2.time-tm1.time)*1000 + (tm2.millitm-tm1.millitm));
        LOG("CHECK ENROLL TIME=%li", (tm2.tv_sec - tm1.tv_sec) * 1000 + (tm2.tv_usec - tm1.tv_usec) / 1000);

#if (FULL_DIAGNOSTICS > 0)
        unsigned short n = t->user_EFT[uIdx].numEnroll;
        float temp[n];
        //int eIdx;

        LOG("global feedback = %li \n", t->feedback.general);
        for (eIdx = 0; eIdx < t->user_EFT[uIdx].numEnroll; eIdx++) {
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_ENERGY_MIN, &temp[0], n);
            LOG("REC_CHECK_ENERGY_MIN[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_ENERGY_STD_DEV, &temp[0], n);
            LOG("REC_CHECK_ENERGY_STD_DEV[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SIL_BEG_MSEC, &temp[0], n);
            LOG("REC_CHECK_SIL_BEG_MSEC[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SIL_END_MSEC, &temp[0], n);
            LOG("REC_CHECK_SIL_END_MSEC[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SNR, &temp[0], n);
            LOG("REC_CHECK_SNR[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_RECORDING_VARIANCE, &temp[0], n);
            LOG("REC_CHECK_RECORDING_VARIANCE[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_CLIPPING_PERCENT, &temp[0], n);
            LOG("REC_CHECK_CLIPPING_PERCENT[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_CLIPPING_MSEC, &temp[0], n);
            LOG("REC_CHECK_CLIPPING_MSEC[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_POOR_RECORDINGS, &temp[0], n);
            LOG("REC_CHECK_POOR_RECORDINGS[%i]=%f\n", eIdx, temp[eIdx]);
            thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_VOWEL_DUR, &temp[0], n);
            LOG("REC_CHECK_VOWEL_DUR[%i]=%f\n", eIdx, temp[eIdx]);
            // thfSpeakerCheckValueGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_REPETITION, &temp[0], n);
            // LOG("REC_CHECK_REPETITION[%i]=%f\n", eIdx, temp[eIdx]);
            //t->feedback.details[eIdx] &= ~CHECKRECORDING_BITFIELD_SPOT;
            // LOG("user %d enroll %d has feedback '%i'\n", uIdx + 1, eIdx + 1,
            //    (unsigned int) t->feedback.details[eIdx]);
        }
#endif
        LOG("phrase quality feedback = %f \n", t->feedback.phraseQuality);
        return 1;

    } else {
        // UDT STUFF  ============================================
        uIdx -= MAX_EFT;
        if (t->udt) {
            thfUdtDestroy(t->udt);
            t->udt = NULL;
        }

        // t->udt = thfUdtCreate(t->thf, t->netfileName_UDT, t->phsearchfileName_UDT,
        //                    t->udtsvsidfileName_UDT, MAX_ENROLL, SAMPLERATE);
        t->udt = thfUdtCreate(t->thf, t->enrollnetfileName_UDT, t->phsearchfileName_UDT,
                              t->udtsvsidfileName_UDT, MAX_ENROLL, SAMPLERATE);
        if (t->udt == NULL) THROW("t->udt is null");

        if (!thfUdtConfigSet(t->thf, t->udt, UDT_PARAM_A_START, t->paramAStart_UDT))
                             THROW("thfUdtConfigSet:UDT_PARAM_A_START");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_PARAM_B_START, t->paramBStart_UDT))
                             THROW("thfUdtConfigSet:UDT_PARAM_B_START");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_SAMP_PER_CAT, 128))
                             THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_SAMP_PER_CAT_WITHIN, 64))
                             THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT_WITHIN");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_NOISE_PERCENT, 5.0))
                             THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_SIL_PERCENT, 50.0))
                             THROW("thfUdtConfigSet:UDT_SAMP_PER_CAT");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_USE_FEAT, 1))
                             THROW("thfUdtConfigSet:UDT_USE_FEAT");
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_IGNORE_TYPE, 0))
                             THROW("thfUdtConfigSet:UDT_IGNORE_TYPE");

        // 2015-04-02 added this.
        if (!thfUdtConfigSet(t->thf, t->udt, UDT_CHECK_SNR, t->checkSNR_UDT))
                             THROW("thfUdtConfigSet:UDT_CHECK_SNR");

        /* recheck enrollments for everyone else */
        for (tempIdx = 0; tempIdx < MAX_UDT; tempIdx++) {
            if ((tempIdx != uIdx) && (t->user_UDT[tempIdx].numEnroll > 0)) {
                LOG("checkNewRecordings:check enrollments for %d\n", tempIdx);
                if (t->feedback.details)
                    free(t->feedback.details);
                t->feedback.details = NULL;
                t->feedback.details = malloc(sizeof(unsigned long) * t->user_UDT[tempIdx].numEnroll);
                if (!(thfUdtCheckEnrollments(t->thf, t->udt, t->user_UDT, tempIdx, &t->feedback.general,
                                             t->feedback.details, &t->feedback.phraseQuality)))
                                              THROW("thfUdtCheckEnrollments");
            }
        }

#if (FULL_DIAGNOSTICS > 0)
        short eIdx;
        LOG("t->user_UDT[%i].userName=%s\n", uIdx, t->user_UDT[uIdx].userName);
        LOG("t->user_UDT[%i].tag=%s\n", uIdx, t->user_UDT[uIdx].tag);
        LOG("t->user_UDT[%i].numEnroll=%i\n", uIdx, t->user_UDT[uIdx].numEnroll);
        LOG("t->user_UDT[%i].reserved1=%s\n", uIdx, t->user_UDT[uIdx].reserved1);
        LOG("t->user_UDT[%i].reserved2=%s\n", uIdx, t->user_UDT[uIdx].reserved2);
        for (eIdx = 0; eIdx < t->user_UDT[uIdx].numEnroll; eIdx++) {
            LOG("t->user_UDT[%i].enroll[%i].audio[0]=%i\n", uIdx, eIdx, t->user_UDT[uIdx].enroll[eIdx].audio[0]);
            LOG("t->user_UDT[%i].enroll[%i].audioLen=%li\n", uIdx, eIdx, t->user_UDT[uIdx].enroll[eIdx].audioLen);
            LOG("t->user_UDT[%i].enroll[%i].audioSignature=%li\n", uIdx, eIdx, t->user_UDT[uIdx].enroll[eIdx].audioSignature);
            LOG("t->user_UDT[%i].enroll[%i].filename=%s\n", uIdx, eIdx, t->user_UDT[uIdx].enroll[eIdx].filename);
            LOG("t->user_UDT[%i].enroll[%i].reserved1=%s\n", uIdx, eIdx, t->user_UDT[uIdx].enroll[eIdx].reserved1);
            LOG("t->user_UDT[%i].enroll[%i].pronun=%s\n", uIdx, eIdx, t->user_UDT[uIdx].enroll[eIdx].pronun);
        }
#endif

        /* check enrollment for the user in question */
        if (t->feedback.details)
            free(t->feedback.details);
        t->feedback.details = NULL;
        t->feedback.details = malloc(sizeof(unsigned long) * t->user_UDT[uIdx].numEnroll);
        //ftime(&tm1);
        gettimeofday(&tm1, &tz);
        if (!(thfUdtCheckEnrollments(t->thf, t->udt, t->user_UDT, uIdx,
                                     &t->feedback.general, t->feedback.details,
                                     &t->feedback.phraseQuality))) THROW("thfUdtCheckEnrollments");
        //ftime(&tm2);
        gettimeofday(&tm2, &tz);
        if (uIdx >= t->numUsers_UDT)
            t->numUsers_UDT = uIdx + 1; // added for v3.12.0
        //LOG("CHECK ENROLL TIME=%li", (tm2.time-tm1.time)*1000 + (tm2.millitm-tm1.millitm));
        LOG("CHECK ENROLL TIME=%li", (tm2.tv_sec - tm1.tv_sec) * 1000 + (tm2.tv_usec - tm1.tv_usec) / 1000);
        LOG("global feedback = %li \n", t->feedback.general);

#if (FULL_DIAGNOSTICS > 0)
        unsigned short n = t->user_UDT[uIdx].numEnroll;
        float temp[n];
        for (eIdx = 0; eIdx < t->user_UDT[uIdx].numEnroll; eIdx++) {

            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_ENERGY_MIN, &temp[0], n);
            LOG("UDT_CHECK_ENERGY_MIN[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_ENERGY_STD_DEV, &temp[0], n);
            LOG("UDT_CHECK_ENERGY_STD_DEV[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_SIL_BEG_MSEC, &temp[0], n);
            LOG("UDT_CHECK_SIL_BEG_MSEC[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_SIL_END_MSEC, &temp[0], n);
            LOG("UDT_CHECK_SIL_END_MSEC[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_SNR, &temp[0], n);
            LOG("UDT_CHECK_SNR[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_RECORDING_VARIANCE, &temp[0], n);
            LOG("UDT_CHECK_RECORDING_VARIANCE[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_CLIPPING_PERCENT, &temp[0], n);
            LOG("UDT_CHECK_CLIPPING_PERCENT[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_CLIPPING_MSEC, &temp[0], n);
            LOG("UDT_CHECK_CLIPPING_MSEC[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_POOR_RECORDINGS, &temp[0], n);
            LOG("UDT_CHECK_POOR_RECORDINGS[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_VOWEL_DUR, &temp[0], n);
            LOG("UDT_CHECK_VOWEL_DUR[%i]=%f\n", eIdx, temp[eIdx]);
            thfUdtCheckValueGet(t->thf, t->udt, UDT_CHECK_REPETITION, &temp[0], n);
            LOG("UDT_CHECK_REPETITION[%i]=%f\n", eIdx, temp[eIdx]);

            if (t->feedback.details[eIdx] != 0)
                LOG("Warning: user %d, enroll %d: detected poor recording: code=0x%x (%s)\n",
                        uIdx + 1, eIdx + 1,
                        (unsigned int) t->feedback.details[eIdx],
                        checkFlagsText(t->feedback.details[eIdx]));
        }
        LOG("phrase quality feedback = %f \n", t->feedback.phraseQuality);
#endif
        return 1;
    }

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    PANIC("Panic: checkNewRecordings: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
// Returns feedback details. Call checkNewRecordings() to first to generate feedback.
jlongArray Java_com_sprd_voicetrigger_nativeinterface_Recog_getFeedbackDetails(
        JNIEnv *env, jobject job, jlong arg, jshort uIdx) {
    trig_t *t = (trig_t *) (long) arg;
    jlongArray result;
    int i;

    if (uIdx < MAX_EFT) {

        // EFT STUFF  ============================================

        if (t->user_EFT[uIdx].numEnroll == 0)
            return NULL;
        result = (*env)->NewLongArray(env, t->user_EFT[uIdx].numEnroll);
        if (result == NULL)
            return NULL; /* out of memory error */
        // NOTE: cast & copy each element of array (jlong = 64bit, long=32bit)
        for (i = 0; i < t->user_EFT[uIdx].numEnroll; i++) {
            jlong tmp = (jlong) t->feedback.details[i];
            (*env)->SetLongArrayRegion(env, result, (jsize) i, 1,
                                       (jlong * ) & tmp);
        }
        return result;

    } else {

        // UDT STUFF  ============================================

        uIdx -= MAX_EFT;

        if (t->user_UDT[uIdx].numEnroll == 0)
            return NULL;
        result = (*env)->NewLongArray(env, t->user_UDT[uIdx].numEnroll);
        if (result == NULL)
            return NULL; /* out of memory error */
        // NOTE: cast & copy each element of array (jlong = 64bit, long=32bit)
        for (i = 0; i < t->user_UDT[uIdx].numEnroll; i++) {
            jlong tmp = (jlong) t->feedback.details[i];
            (*env)->SetLongArrayRegion(env, result, (jsize) i, 1,
                                       (jlong * ) & tmp);
        }
        return result;

    }
}

// ============================================================================
// Returns phrase quality feedback. Call checkNewRecordings() to first to generate feedback.
float Java_com_sprd_voicetrigger_nativeinterface_Recog_getPhraseQuality(
        JNIEnv *env, jobject job, jlong arg, jshort uIdx) {
    trig_t *t = (trig_t *) (long) arg;
    return t->feedback.phraseQuality;
}

// ============================================================================
// Enroll users. This creates relevant recognition data structures and dumps deeply embedded data files.
int Java_com_sprd_voicetrigger_nativeinterface_Recog_doEnroll(JNIEnv *env,
                                                              jobject job, jlong arg,
                                                              jstring jappDir, jstring jFTnetfile,
                                                              jstring jFTsearchfile,
                                                              jstring jdspTarget) {
    trig_t *t = (trig_t *) (long) arg;
    const char *ewhere, *ewhat = NULL;
    short uIdx;
    char *filename = malloc(MAXFILENAME);
    char *filename1 = malloc(MAXFILENAME);
    char *filename2 = malloc(MAXFILENAME);
    short eIdx;
    searchs_t *searchSV = NULL;
    recog_t *recogSV = NULL;
    searchs_t *UDTsearch = NULL;
    recog_t *UDTrecog = NULL;
    searchs_t *fixedSearch = NULL;
    recog_t *fixedRecog = NULL;
    searchs_t *combinedSearch = NULL;
    recog_t *combinedRecog = NULL;
    FILE *fp = NULL;
    const char *appDir = NULL;
    const char *FTNetFile = NULL, *FTSearchFile = NULL;
    const char *dspTarget = NULL;
    //struct timeb tm1, tm2;
    struct timeval tm1, tm2;
    struct timezone tz;
    float compressionRatio;

    if (jappDir)
        appDir = (*env)->GetStringUTFChars(env, jappDir, 0);

    if (jFTnetfile)
        FTNetFile = (*env)->GetStringUTFChars(env, jFTnetfile, 0);

    if (jFTsearchfile)
        FTSearchFile = (*env)->GetStringUTFChars(env, jFTsearchfile, 0);

    if (jdspTarget)
        dspTarget = (*env)->GetStringUTFChars(env, jdspTarget, 0);

    // EFT STUFF  ============================================

    // save wave data to SD card
    if (appDir != NULL) {
        for (uIdx = 0; uIdx < t->numUsers_EFT; uIdx++) {
            for (eIdx = 0; eIdx < t->user_EFT[uIdx].numEnroll; eIdx++) {
                sprintf(filename, "%s/eft%i_%i.wav", appDir, uIdx + 1,
                        eIdx + 1);
                if ((fp = fopen(filename, "wb"))) {
                    if (!thfWaveSaveToFile(t->thf, filename,
                                           (short *) t->user_EFT[uIdx].enroll[eIdx].audio,
                                           t->user_EFT[uIdx].enroll[eIdx].audioLen, 16000)) THROW(
                            "thfWaveSaveToFile: eft#_#.wav");
                    fclose(fp);
                    LOG("EFT SAVED TO SDCARD: samples=%li file=%s",
                        t->user_EFT[uIdx].enroll[eIdx].audioLen, filename);
                }
            }
        }
    }

    for (uIdx = 0; uIdx < t->numUsers_EFT; uIdx++) {
        LOG("enrolling EFT user %d = '%s'\n", uIdx, t->user_EFT[uIdx].userName);
    }

    //float temp;
    //thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_REPETITION, &temp);         LOG("REC_CHECK_REPETITION=%f\n", temp);

#if (FULL_DIAGNOSTICS > 0)
    float temp;

    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_LSILENCE, &temp);
    LOG("REC_LSILENCE=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_TSILENCE, &temp);
    LOG("REC_TSILENCE=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_MAXREC, &temp);
    LOG("REC_MAXREC=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_MINDUR, &temp);
    LOG("REC_MINDUR=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_ESILENCET, &temp);
    LOG("REC_ESILENCET=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_ESPEECHT, &temp);
    LOG("REC_ESPEECHT=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SHORTTERMMS, &temp);
    LOG("REC_SHORTTERMMS=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_LONGTERMMS, &temp);
    LOG("REC_LONGTERMMS=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_BACKOFF, &temp);
    LOG("REC_BACKOFF=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_THI, &temp);
    LOG("REC_THI=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SDET_EDIFF, &temp);
    LOG("REC_SDET_EDIFF=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SDET_INITIAL_DELAY,
                      &temp);
    LOG("REC_SDET_INITIAL_DELAY=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SDET_STEM, &temp);
    LOG("REC_SDET_STEM=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SDET_LTEM, &temp);
    LOG("REC_SDET_LTEM=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_KEEP_SDET_HISTORY,
                      &temp);
    LOG("REC_KEEP_SDET_HISTORY=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_KEEP_FEATURE_HISTORY,
                      &temp);
    LOG("REC_KEEP_FEATURE_HISTORY=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_KEEP_FEATURES, &temp);
    LOG("REC_KEEP_FEATURES=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_EPQ_ENABLE, &temp);
    LOG("REC_EPQ_ENABLE=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_EPQ_MIN, &temp);
    LOG("REC_EPQ_MIN=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_EPQ_NOISE, &temp);
    LOG("REC_EPQ_NOISE=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SV_ADJUST_METHOD,
                      &temp);
    LOG("REC_SV_ADJUST_METHOD=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SV_ADJUST1, &temp);
    LOG("REC_SV_ADJUST1=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SV_ADJUST2, &temp);
    LOG("REC_SV_ADJUST2=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SV_ADJUST3, &temp);
    LOG("REC_SV_ADJUST3=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_SV_ADJUST4, &temp);
    LOG("REC_SV_ADJUST4=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_PARAMA_ADJUST_METHOD,
                      &temp);
    LOG("REC_PARAMA_ADJUST_METHOD=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_PARAMA_ADJUST1,
                      &temp);
    LOG("REC_PARAMA_ADJUST1=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_PARAMA_ADJUST2,
                      &temp);
    LOG("REC_PARAMA_ADJUST2=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_PARAMA_ADJUST3,
                      &temp);
    LOG("REC_PARAMA_ADJUST3=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_PARAMA_ADJUST4,
                      &temp);
    LOG("REC_PARAMA_ADJUST4=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT,
                      REC_PHONEMEREC_SEARCH_TYPE, &temp);
    LOG("REC_PHONEMEREC_SEARCH_TYPE=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_USE_FEAT, &temp);
    LOG("REC_USE_FEAT=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_ENERGY_MIN,
                      &temp);
    LOG("REC_CHECK_ENERGY_MIN=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_ENERGY_STD_DEV,
                      &temp);
    LOG("REC_CHECK_ENERGY_STD_DEV=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SIL_BEG_MSEC,
                      &temp);
    LOG("REC_CHECK_SIL_BEG_MSEC=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SIL_END_MSEC,
                      &temp);
    LOG("REC_CHECK_SIL_END_MSEC=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT,
                      REC_CHECK_RECORDING_VARIANCE, &temp);
    LOG("REC_CHECK_RECORDING_VARIANCE=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT,
                      REC_CHECK_CLIPPING_PERCENT, &temp);
    LOG("REC_CHECK_CLIPPING_PERCENT=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_CLIPPING_MSEC,
                      &temp);
    LOG("REC_CHECK_CLIPPING_MSEC=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_POOR_RECORDINGS,
                      &temp);
    LOG("REC_CHECK_POOR_RECORDINGS=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_SPOT, &temp);
    LOG("REC_CHECK_SPOT=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_VOWEL_DUR,
                      &temp);
    LOG("REC_CHECK_VOWEL_DUR=%f\n", temp);
    //thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_CHECK_REPETITION, &temp);         LOG("REC_CHECK_REPETITION=%f\n", temp);
    thfRecogConfigGet(t->thf, t->recogEnrollNoSD_EFT, REC_EARLYSTOP_NBEST,
                      &temp);
    LOG("REC_EARLYSTOP_NBEST=%f\n", temp);

    thfSearchConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                       SCH_PRUNING, &temp);
    LOG("SCH_PRUNING=%f\n", temp);
    thfSearchConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                       SCH_BEAM, &temp);
    LOG("SCH_BEAM=%f\n", temp);
    thfSearchConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                       SCH_GARBAGE, &temp);
    LOG("SCH_GARBAGE=%f\n", temp);
    thfSearchConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                       SCH_ANY, &temp);
    LOG("SCH_GARBAGE=%f\n", temp);
    thfSearchConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                       SCH_NOTA, &temp);
    LOG("SCH_NOTA=%f\n", temp);
    thfSearchConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                       SCH_LONGPEN, &temp);
    LOG("SCH_LONGPEN=%f\n", temp);

    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_PARAMA_OFFSET, &temp);
    LOG("PS_PARAMA_OFFSET=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_BEAM, &temp);
    LOG("PS_BEAM=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_ABSBEAM, &temp);
    LOG("PS_ABSBEAM=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_DELAY, &temp);
    LOG("PS_DELAY=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_LONGPEN, &temp);
    LOG("PS_LONGPEN=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_SEQ_BUFFER, &temp);
    LOG("PS_SEQ_BUFFER=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_SEQ_SPOT_OPTIONAL, &temp);
    LOG("PS_SEQ_SPOT_OPTIONAL=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_SEQ_SPOT_INCLUDED, &temp);
    LOG("PS_SEQ_SPOT_INCLUDED=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, t->recogEnrollNoSD_EFT, t->searchEnroll_EFT,
                           PS_SEQ_SAMPLES, &temp);
    LOG("PS_SEQ_SAMPLES=%f\n", temp);

    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_WANT_NUM_RECORDINGS, &temp);
    LOG("SPEAKER_WANT_NUM_RECORDINGS=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_HAVE_NUM_RECORDINGS, &temp);
    LOG("SPEAKER_HAVE_NUM_RECORDINGS=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_DONE_ADAPTATION, &temp);
    LOG("SPEAKER_DONE_ADAPTATION=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_RECOG_WITH_ADAPTED, &temp);
    LOG("SPEAKER_RECOG_WITH_ADAPTED=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_SPEED_ACCURACY, &temp);
    LOG("SPEAKER_SPEED_ACCURACY=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0, SPEAKER_SAMP_PER_CAT,
                        &temp);
    LOG("SPEAKER_SAMP_PER_CAT=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_TRIGGER_SAMP_PER_CAT, &temp);
    LOG("SPEAKER_TRIGGER_SAMP_PER_CAT=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_SAMP_PER_CAT_WITHIN, &temp);
    LOG("SPEAKER_SAMP_PER_CAT_WITHIN=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0, SPEAKER_TARGET_SNR,
                        &temp);
    LOG("SPEAKER_TARGET_SNR=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_DUR_MIN_FACTOR, &temp);
    LOG("SPEAKER_DUR_MIN_FACTOR=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_DUR_MAX_FACTOR, &temp);
    LOG("SPEAKER_DUR_MAX_FACTOR=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0, SPEAKER_USE_FEAT,
                        &temp);
    LOG("SPEAKER_USE_FEAT=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_TRAIN_ITERATIONS, &temp);
    LOG("SPEAKER_TRAIN_ITERATIONS=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_TRAIN_ITERATIONS_WITHIN, &temp);
    LOG("SPEAKER_TRAIN_ITERATIONS_WITHIN=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0, SPEAKER_LEARN_RATE,
                        &temp);
    LOG("SPEAKER_LEARN_RATE=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_LEARN_RATE_WITHIN, &temp);
    LOG("SPEAKER_LEARN_RATE_WITHIN=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_DROPOUT_WITHIN, &temp);
    LOG("SPEAKER_DROPOUT_WITHIN=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0, SPEAKER_ADAPT_TYPE,
                        &temp);
    LOG("SPEAKER_ADAPT_TYPE=%f", temp);
    thfSpeakerConfigGet(t->thf, t->recogEnrollNoSD_EFT, 0,
                        SPEAKER_DSP_SV_THRESH, &temp);
    LOG("SPEAKER_DSP_SV_THRESH=%f", temp);
#endif

    if (t->numUsers_EFT > 0) {
        //ftime(&tm1);
        gettimeofday(&tm1, &tz);
        if (!thfSpeakerEnroll(t->thf, t->recogEnrollNoSD_EFT, NULL,
                              "IGNORE_SV_DEFAULT", (unsigned short) t->numUsers_EFT,
                              t->user_EFT, &searchSV, &recogSV)) THROW("thfSpeakerEnroll");
        //ftime(&tm2);
        gettimeofday(&tm2, &tz);
        //LOG("CHECK ENROLL TIME=%li", (tm2.time-tm1.time)*1000 + (tm2.millitm-tm1.millitm));
        LOG("EFT ENROLL TIME=%li",
            (tm2.tv_sec - tm1.tv_sec) * 1000
            + (tm2.tv_usec - tm1.tv_usec) / 1000);
    }

    // UDT STUFF  ============================================

    // save wave data to SD card
    if (appDir != NULL) {
        for (uIdx = 0; uIdx < t->numUsers_UDT; uIdx++) {
            for (eIdx = 0; eIdx < t->user_UDT[uIdx].numEnroll; eIdx++) {
                sprintf(filename, "%sudt%i_%i.wav", appDir, uIdx + 1,
                        eIdx + 1);
                if ((fp = fopen(filename, "wb"))) {
                    if (!thfWaveSaveToFile(t->thf, filename,
                                           (short *) t->user_UDT[uIdx].enroll[eIdx].audio,
                                           t->user_UDT[uIdx].enroll[eIdx].audioLen, 16000)) THROW(
                            "thfWaveSaveToFile: udt#_#.wav");
                    fclose(fp);
                    LOG("UDT SAVED TO SDCARD: samples=%li file=%s",
                        t->user_UDT[uIdx].enroll[eIdx].audioLen, filename);
                }
            }
        }
    }

    for (uIdx = 0; uIdx < t->numUsers_UDT; uIdx++) {
        LOG("enrolling UDT user %d = '%s'\n", uIdx, t->user_UDT[uIdx].userName);
    }

    //thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_REPETITION, &temp);         LOG("UDT_CHECK_REPETITION=%f", temp);

#if (FULL_DIAGNOSTICS > 0)
    //float temp;
    thfUdtConfigGet(t->thf, t->udt, UDT_SAMP_PER_CAT, &temp);
    LOG("UDT_SAMP_PER_CAT=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_A_START, &temp);
    LOG("UDT_PARAM_A_START=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_A_NUM, &temp);
    LOG("UDT_PARAM_A_NUM=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_A_STEP, &temp);
    LOG("UDT_PARAM_A_STEP=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_B_START, &temp);
    LOG("UDT_PARAM_B_START=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_B_NUM, &temp);
    LOG("UDT_PARAM_B_NUM=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_B_STEP, &temp);
    LOG("UDT_PARAM_B_STEP=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_MAX_PARAM_D, &temp);
    LOG("UDT_MAX_PARAM_D=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_NUM_CHUNKS, &temp);
    LOG("UDT_NUM_CHUNKS=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_MAX_MEMORY, &temp);
    LOG("UDT_MAX_MEMORY=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_ABSBEAM, &temp);
    LOG("UDT_ABSBEAM=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_REQUIRE_SILENCE, &temp);
    LOG("UDT_REQUIRE_SILENCE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_SIMILARITY_THRESH, &temp);
    LOG("UDT_SIMILARITY_THRESH=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_E, &temp);
    LOG("UDT_PARAM_E=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_F, &temp);
    LOG("UDT_PARAM_F=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PARAM_G, &temp);
    LOG("UDT_PARAM_G=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_SAMP_PER_CAT_WITHIN, &temp);
    LOG("UDT_SAMP_PER_CAT_WITHIN=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_IGNORE_TYPE, &temp);
    LOG("UDT_IGNORE_TYPE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_SIL_PERCENT, &temp);
    LOG("UDT_SIL_PERCENT=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_NOISE_PERCENT, &temp);
    LOG("UDT_NOISE_PERCENT=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_PHONEMEREC_SEARCH_TYPE, &temp);
    LOG("UDT_PHONEMEREC_SEARCH_TYPE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_SANITIZE, &temp);
    LOG("UDT_SANITIZE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_DUR_MIN_FACTOR, &temp);
    LOG("UDT_DUR_MIN_FACTOR=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_DUR_MAX_FACTOR, &temp);
    LOG("UDT_DUR_MAX_FACTOR=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_POP_THRESH, &temp);
    LOG("UDT_POP_THRESH=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_DBA_FILTER, &temp);
    LOG("UDT_DBA_FILTER=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_TARGET_SNR, &temp);
    LOG("UDT_TARGET_SNR=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_USE_FEAT, &temp);
    LOG("UDT_USE_FEAT=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_ENERGY_MIN, &temp);
    LOG("UDT_CHECK_ENERGY_MIN=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_ENERGY_STD_DEV, &temp);
    LOG("UDT_CHECK_ENERGY_STD_DEV=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_SIL_BEG_MSEC, &temp);
    LOG("UDT_CHECK_SIL_BEG_MSEC=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_SIL_END_MSEC, &temp);
    LOG("UDT_CHECK_SIL_END_MSEC=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_SNR, &temp);
    LOG("UDT_CHECK_SNR=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_RECORDING_VARIANCE, &temp);
    LOG("UDT_CHECK_RECORDING_VARIANCE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_CLIPPING_PERCENT, &temp);
    LOG("UDT_CHECK_CLIPPING_PERCENT=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_CLIPPING_MSEC, &temp);
    LOG("UDT_CHECK_CLIPPING_MSEC=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_POOR_RECORDINGS, &temp);
    LOG("UDT_CHECK_POOR_RECORDINGS=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_VOWEL_DUR, &temp);
    LOG("UDT_CHECK_VOWEL_DUR=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_CHECK_REPETITION, &temp);
    LOG("UDT_CHECK_REPETITION=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_TRAIN_ITERATIONS, &temp);
    LOG("UDT_TRAIN_ITERATIONS=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_TRAIN_ITERATIONS_WITHIN, &temp);
    LOG("UDT_TRAIN_ITERATIONS_WITHIN=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_LEARN_RATE, &temp);
    LOG("UDT_LEARN_RATE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_LEARN_RATE_WITHIN, &temp);
    LOG("UDT_LEARN_RATE_WITHIN=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_DROPOUT_WITHIN, &temp);
    LOG("UDT_DROPOUT_WITHIN=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_ADAPT_TYPE, &temp);
    LOG("UDT_ADAPT_TYPE=%f", temp);
    thfUdtConfigGet(t->thf, t->udt, UDT_DEBUG_LEVEL, &temp);
    LOG("UDT_DEBUG_LEVEL=%f", temp);
#endif

    if (t->numUsers_UDT > 0) {
        //ftime(&tm1);
        gettimeofday(&tm1, &tz);
        if (!thfUdtEnroll(t->thf, t->udt, t->numUsers_UDT, t->user_UDT,
                          &UDTsearch, &UDTrecog, NULL, NULL)) THROW("thfUdtEnroll");
        //ftime(&tm2);
        gettimeofday(&tm2, &tz);
        //LOG("CHECK ENROLL TIME=%li", (tm2.time-tm1.time)*1000 + (tm2.millitm-tm1.millitm));
        LOG("UDT ENROLL TIME=%li",
            (tm2.tv_sec - tm1.tv_sec) * 1000
            + (tm2.tv_usec - tm1.tv_usec) / 1000);
    }

    // Common STUFF  ============================================

    //combine with fixed trigger
    // Read in SID-specific recognizer and search from file
    // possible outcomes
    if (t->numUsers_UDT > 0) {
        if (t->numUsers_EFT > 0) {
            if (!(thfRecogCombine(t->thf, -1, UDTrecog, recogSV, UDTsearch,
                                  searchSV, NULL, NULL, &combinedRecog, &combinedSearch,
                                  &compressionRatio))) THROW("thfRecogCombine:1");
        } else if (FTNetFile != NULL) {
            if (!(fixedRecog = thfRecogCreateFromFile(t->thf, FTNetFile, 0xFFFF,
                                                      -1, NO_SDET))) THROW(
                    "thfRecogCreateFromFile");
            if (!(fixedSearch = thfSearchCreateFromFile(t->thf, fixedRecog,
                                                        FTSearchFile, NBEST))) THROW(
                    "thfSearchCreateFromFile");
            if (!(thfRecogCombine(t->thf, -1, UDTrecog, fixedRecog, UDTsearch,
                                  fixedSearch, NULL, NULL, &combinedRecog, &combinedSearch,
                                  &compressionRatio))) THROW("thfRecogCombine:2");
        } else {
            combinedRecog = UDTrecog;
            combinedSearch = UDTsearch;
        }
    } else if (t->numUsers_EFT > 0) {
        combinedRecog = recogSV;
        combinedSearch = searchSV;
    }

    // disable EPQ
    if (!thfRecogConfigSet(t->thf, combinedRecog, REC_EPQ_ENABLE, DONT_USE_EPQ)) THROW(
            "thfRecogConfigSet:REC_EPQ_ENABLE");

    //thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_REPETITION, &temp);         LOG("REC_CHECK_REPETITION=%f\n", temp);

#if (FULL_DIAGNOSTICS > 0)
    thfRecogConfigGet(t->thf, combinedRecog, REC_LSILENCE, &temp);
    LOG("REC_LSILENCE=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_TSILENCE, &temp);
    LOG("REC_TSILENCE=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_MAXREC, &temp);
    LOG("REC_MAXREC=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_MINDUR, &temp);
    LOG("REC_MINDUR=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_ESILENCET, &temp);
    LOG("REC_ESILENCET=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_ESPEECHT, &temp);
    LOG("REC_ESPEECHT=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SHORTTERMMS, &temp);
    LOG("REC_SHORTTERMMS=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_LONGTERMMS, &temp);
    LOG("REC_LONGTERMMS=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_BACKOFF, &temp);
    LOG("REC_BACKOFF=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_THI, &temp);
    LOG("REC_THI=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SDET_EDIFF, &temp);
    LOG("REC_SDET_EDIFF=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SDET_INITIAL_DELAY, &temp);
    LOG("REC_SDET_INITIAL_DELAY=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SDET_STEM, &temp);
    LOG("REC_SDET_STEM=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SDET_LTEM, &temp);
    LOG("REC_SDET_LTEM=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_KEEP_SDET_HISTORY, &temp);
    LOG("REC_KEEP_SDET_HISTORY=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_KEEP_FEATURE_HISTORY, &temp);
    LOG("REC_KEEP_FEATURE_HISTORY=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_KEEP_FEATURES, &temp);
    LOG("REC_KEEP_FEATURES=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_EPQ_ENABLE, &temp);
    LOG("REC_EPQ_ENABLE=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_EPQ_MIN, &temp);
    LOG("REC_EPQ_MIN=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_EPQ_NOISE, &temp);
    LOG("REC_EPQ_NOISE=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SV_ADJUST_METHOD, &temp);
    LOG("REC_SV_ADJUST_METHOD=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SV_ADJUST1, &temp);
    LOG("REC_SV_ADJUST1=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SV_ADJUST2, &temp);
    LOG("REC_SV_ADJUST2=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SV_ADJUST3, &temp);
    LOG("REC_SV_ADJUST3=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_SV_ADJUST4, &temp);
    LOG("REC_SV_ADJUST4=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_PARAMA_ADJUST_METHOD, &temp);
    LOG("REC_PARAMA_ADJUST_METHOD=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_PARAMA_ADJUST1, &temp);
    LOG("REC_PARAMA_ADJUST1=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_PARAMA_ADJUST2, &temp);
    LOG("REC_PARAMA_ADJUST2=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_PARAMA_ADJUST3, &temp);
    LOG("REC_PARAMA_ADJUST3=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_PARAMA_ADJUST4, &temp);
    LOG("REC_PARAMA_ADJUST4=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_PHONEMEREC_SEARCH_TYPE, &temp);
    LOG("REC_PHONEMEREC_SEARCH_TYPE=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_USE_FEAT, &temp);
    LOG("REC_USE_FEAT=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_ENERGY_MIN, &temp);
    LOG("REC_CHECK_ENERGY_MIN=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_ENERGY_STD_DEV, &temp);
    LOG("REC_CHECK_ENERGY_STD_DEV=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_SIL_BEG_MSEC, &temp);
    LOG("REC_CHECK_SIL_BEG_MSEC=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_SIL_END_MSEC, &temp);
    LOG("REC_CHECK_SIL_END_MSEC=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_RECORDING_VARIANCE,
                      &temp);
    LOG("REC_CHECK_RECORDING_VARIANCE=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_CLIPPING_PERCENT, &temp);
    LOG("REC_CHECK_CLIPPING_PERCENT=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_CLIPPING_MSEC, &temp);
    LOG("REC_CHECK_CLIPPING_MSEC=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_POOR_RECORDINGS, &temp);
    LOG("REC_CHECK_POOR_RECORDINGS=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_SPOT, &temp);
    LOG("REC_CHECK_SPOT=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_VOWEL_DUR, &temp);
    LOG("REC_CHECK_VOWEL_DUR=%f\n", temp);
    //thfRecogConfigGet(t->thf, combinedRecog, REC_CHECK_REPETITION, &temp);         LOG("REC_CHECK_REPETITION=%f\n", temp);
    thfRecogConfigGet(t->thf, combinedRecog, REC_EARLYSTOP_NBEST, &temp);
    LOG("REC_EARLYSTOP_NBEST=%f\n", temp);

    thfSearchConfigGet(t->thf, combinedRecog, combinedSearch, SCH_PRUNING,
                       &temp);
    LOG("SCH_PRUNING=%f\n", temp);
    thfSearchConfigGet(t->thf, combinedRecog, combinedSearch, SCH_BEAM, &temp);
    LOG("SCH_BEAM=%f\n", temp);
    thfSearchConfigGet(t->thf, combinedRecog, combinedSearch, SCH_GARBAGE,
                       &temp);
    LOG("SCH_GARBAGE=%f\n", temp);
    thfSearchConfigGet(t->thf, combinedRecog, combinedSearch, SCH_ANY, &temp);
    LOG("SCH_GARBAGE=%f\n", temp);
    thfSearchConfigGet(t->thf, combinedRecog, combinedSearch, SCH_NOTA, &temp);
    LOG("SCH_NOTA=%f\n", temp);
    thfSearchConfigGet(t->thf, combinedRecog, combinedSearch, SCH_LONGPEN,
                       &temp);
    LOG("SCH_LONGPEN=%f\n", temp);

    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch,
                           PS_PARAMA_OFFSET, &temp);
    LOG("PS_PARAMA_OFFSET=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch, PS_BEAM,
                           &temp);
    LOG("PS_BEAM=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch, PS_ABSBEAM,
                           &temp);
    LOG("PS_ABSBEAM=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch, PS_DELAY,
                           &temp);
    LOG("PS_DELAY=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch, PS_LONGPEN,
                           &temp);
    LOG("PS_LONGPEN=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch, PS_SEQ_BUFFER,
                           &temp);
    LOG("PS_SEQ_BUFFER=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch,
                           PS_SEQ_SPOT_OPTIONAL, &temp);
    LOG("PS_SEQ_SPOT_OPTIONAL=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch,
                           PS_SEQ_SPOT_INCLUDED, &temp);
    LOG("PS_SEQ_SPOT_INCLUDED=%f\n", temp);
    thfPhrasespotConfigGet(t->thf, combinedRecog, combinedSearch,
                           PS_SEQ_SAMPLES, &temp);
    LOG("PS_SEQ_SAMPLES=%f\n", temp);
#endif

    if (!thfPhrasespotConfigSet(t->thf, combinedRecog, combinedSearch, PS_DELAY,
                                150.0)) THROW("doEnroll:thfPhrasespotConfigSet: delay");

    if (!thfRecogConfigSet(t->thf, combinedRecog, REC_EPQ_ENABLE, DONT_USE_EPQ)) THROW(
            "thfRecogConfigSet:REC_EPQ_ENABLE");

    //save to sdcard
    sprintf(filename1, "%s/%s", appDir, SEARCHFILE);
    sprintf(filename2, "%s/%s", appDir, RECOGFILE);
//  if (!thfUdtSaveToFile(t->thf, UDTsearch, UDTrecog, filename1, filename2))
    if (!thfUdtSaveToFile(t->thf, combinedSearch, combinedRecog, filename1,
                          filename2)) THROW("thfUdtSaveToFile:global");

    sprintf(filename1, "%s%s", t->savedir, SEARCHFILE);
    sprintf(filename2, "%s%s", t->savedir, RECOGFILE);
//  if (!thfUdtSaveToFile(t->thf, UDTsearch, UDTrecog, filename1, filename2))
    if (!thfUdtSaveToFile(t->thf, combinedSearch, combinedRecog, filename1,
                          filename2)) THROW("thfUdtSaveToFile:internal");

    // NOTE: cannot use s2 directly (requires adapted net); Must load r2/s2 from file instead
    loadTestRecog(t);

    // Save embedded data files
    if (dspTarget && strlen(dspTarget)) {
//    sprintf(filename1, "%s%s", t->savedir, EMBEDDED_RECOGFILE);
//    sprintf(filename2, "%s%s", t->savedir, EMBEDDED_SEARCHFILE);
        sprintf(filename1, "%s/%s", appDir, EMBEDDED_RECOGFILE);
        sprintf(filename2, "%s/%s", appDir, EMBEDDED_SEARCHFILE);
        LOG("Saving embedded files: '%s' and '%s'\n", filename1, filename2);
        LOG("DSP Target: %s\n", dspTarget);
        if (!thfSaveEmbedded(t->thf, t->recogTest_COMB, t->searchTest_COMB,
                             dspTarget, filename1, filename2, 0, 1)) THROW("thfSaveEmbedded");
        LOG("SAVED EMBEDDED");
    } else {
        LOG("SKIPPING SAVE EMBEDDED; No dsp target specified.");
    }

    free(filename);
    free(filename1);
    free(filename2);
    return 1;

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    free(filename);
    free(filename1);
    free(filename2);
    PANIC("Panic: doEnroll: %s: %s\n\n", ewhere, ewhat);
    return 0;
}

// ============================================================================
// Loads data from file for current users
// Return codes:
// 	0 = error
// 	1 = no users exist
// 	2 = unenrolled user exist (i.e., recordings and pronunciations only)
// 	3 = enrolled users exist
int Java_com_sprd_voicetrigger_nativeinterface_Recog_loadUsers(JNIEnv *env,
                                                               jobject job, jlong arg) {
    trig_t *t = (trig_t *) (long) arg;
    const char *ewhere, *ewhat = NULL;
    char *filename1 = malloc(MAXFILENAME);
    char *filename2 = malloc(MAXFILENAME);
    short uIdx, eIdx;
    short *samples;
    unsigned long samplesLen, srate;
    int currE, newE;
    int flag = 0;

    // EFT STUFF  ============================================

    uIdx = 0;

    /* refresh the user name... this should not be necessary */
    t->user_EFT[uIdx].userName = realloc(t->user_EFT[uIdx].userName,
                                         sizeof(char) * MAXSTR);
    memset(t->user_EFT[uIdx].userName, 0, sizeof(char) * MAXSTR);
    sprintf(t->user_EFT[uIdx].userName, "EFT-%i", uIdx + 1);

    t->numUsers_EFT = 0;
    t->user_EFT[uIdx].numEnroll = 0;
    for (eIdx = 0; eIdx < MAX_ENROLL; eIdx++) {
        /* Load wave files */
        sprintf(filename1, "%seft%i_%i.wav", t->savedir, uIdx, eIdx);
        if (fileExists(filename1)) {
            t->numUsers_EFT = uIdx + 1;
            if (!thfWaveFromFile(t->thf, (const char *) filename1, &samples,
                                 &samplesLen, &srate)) THROW("thfWaveFromFile");
            // Copy audio into memory we alloc and free SDK memory.

            // make sure we have memory enough to store in index eIdx
            if (t->user_EFT[uIdx].numEnroll <= eIdx) {
                t->user_EFT[uIdx].enroll = realloc(t->user_EFT[uIdx].enroll,
                                                   sizeof(udtEnrollment_t) * (eIdx + 1));
                currE = t->user_EFT[uIdx].numEnroll;
                newE = (eIdx + 1) - currE;
                memset(&(t->user_EFT[uIdx].enroll[currE]), 0,
                       sizeof(udtEnrollment_t) * newE);
                t->user_EFT[uIdx].numEnroll = eIdx + 1;
            }

            t->user_EFT[uIdx].enroll[eIdx].audio = realloc(
                    t->user_EFT[uIdx].enroll[eIdx].audio,
                    samplesLen * sizeof(short));
            memcpy(t->user_EFT[uIdx].enroll[eIdx].audio, samples,
                   samplesLen * sizeof(short));
            t->user_EFT[uIdx].enroll[eIdx].audioLen = samplesLen;

            thfFree(samples);
            LOG("Loaded wave file: EFT %i, samples=%li", uIdx, samplesLen);

            flag = 1;
        }
    }

    // UDT STUFF  ============================================

    for (uIdx = 0; uIdx < MAX_UDT; uIdx++) {

        /* refresh the user name... this should not be necessary */
        t->user_UDT[uIdx].userName = realloc(t->user_UDT[uIdx].userName,
                                             sizeof(char) * MAXSTR);
        memset(t->user_UDT[uIdx].userName, 0, sizeof(char) * MAXSTR);
        sprintf(t->user_UDT[uIdx].userName, "UDT-%i", uIdx + 1);

        t->numUsers_UDT = 0;
        t->user_UDT[uIdx].numEnroll = 0;
        for (eIdx = 0; eIdx < MAX_ENROLL; eIdx++) {
            /* Load wave files */
            sprintf(filename1, "%sudt%i_%i.wav", t->savedir, uIdx, eIdx);
            if (fileExists(filename1)) {
                t->numUsers_UDT = uIdx + 1;
                if (!thfWaveFromFile(t->thf, (const char *) filename1, &samples,
                                     &samplesLen, &srate)) THROW("thfWaveFromFile");
                // Copy audio into memory we alloc and free SDK memory.

                // make sure we have memory enough to store in index eIdx
                if (t->user_UDT[uIdx].numEnroll <= eIdx) {
                    t->user_UDT[uIdx].enroll = realloc(t->user_UDT[uIdx].enroll,
                                                       sizeof(udtEnrollment_t) * (eIdx + 1));
                    currE = t->user_UDT[uIdx].numEnroll;
                    newE = (eIdx + 1) - currE;
                    memset(&(t->user_UDT[uIdx].enroll[currE]), 0,
                           sizeof(udtEnrollment_t) * newE);
                    t->user_UDT[uIdx].numEnroll = eIdx + 1;
                }

                t->user_UDT[uIdx].enroll[eIdx].audio = realloc(
                        t->user_UDT[uIdx].enroll[eIdx].audio,
                        samplesLen * sizeof(short));
                memcpy(t->user_UDT[uIdx].enroll[eIdx].audio, samples,
                       samplesLen * sizeof(short));
                t->user_UDT[uIdx].enroll[eIdx].audioLen = samplesLen;

                thfFree(samples);
                LOG("Loaded wave file: UDT %i, samples=%li", uIdx, samplesLen);

                flag = 1;
            }
        }
    }

    if (!flag) {
        free(filename1);
        free(filename2);
        return (1); // STATE = no users
    }

    sprintf(filename1, "%s%s", t->savedir, SEARCHFILE);
    sprintf(filename2, "%s%s", t->savedir, RECOGFILE);
    if (fileExists(filename1) && fileExists(filename2)) {
        loadTestRecog(t);
        free(filename1);
        free(filename2);
        return (3); // RETURN = users enrolled
    }
    free(filename1);
    free(filename2);
    return (2); // STATE = users but not enrolled

    error:
    if (!ewhat && t->thf)
        ewhat = thfGetLastError(t->thf);
    free(filename1);
    free(filename2);
    PANIC("Panic: svLoadFromFile: %s: %s\n\n", ewhere, ewhat);
    return (0);
}

// ============================================================================
// Deletes data (ie., data structures and files) associated with a specific user.
int Java_com_sprd_voicetrigger_nativeinterface_Recog_deleteUser(JNIEnv *env,
                                                                jobject job, jlong arg,
                                                                jshort uIdx) {
    trig_t *t = (trig_t *) (long) arg;
    char *filename = malloc(MAXFILENAME);
    short eIdx;
    // const char *ewhere, *ewhat=NULL;

    --uIdx;

    if (uIdx < MAX_EFT) {

        // EFT STUFF  ============================================

        LOG("delete EFT user: %i", uIdx);
        for (eIdx = 0; eIdx < t->user_EFT[uIdx].numEnroll; eIdx++) {
            if (t->user_EFT[uIdx].enroll) {
                // Remove wave
                if (t->user_EFT[uIdx].enroll[eIdx].audio) {
                    free(t->user_EFT[uIdx].enroll[eIdx].audio);
                    t->user_EFT[uIdx].enroll[eIdx].audio = NULL;
                    t->user_EFT[uIdx].enroll[eIdx].audioLen = 0;
                }
                sprintf(filename, "%seft%i_%i.wav", t->savedir, uIdx, eIdx);
                remove(filename);
            }

            free(t->user_EFT[uIdx].enroll);
            t->user_EFT[uIdx].numEnroll = 0;
            t->user_EFT[uIdx].enroll = NULL;
        }

    } else {

        // UDT STUFF  ============================================

        uIdx -= MAX_EFT;

        LOG("delete UDT user: %i", uIdx);
        for (eIdx = 0; eIdx < t->user_UDT[uIdx].numEnroll; eIdx++) {
            if (t->user_UDT[uIdx].enroll) {
                // Remove wave
                if (t->user_UDT[uIdx].enroll[eIdx].audio) {
                    free(t->user_UDT[uIdx].enroll[eIdx].audio);
                    t->user_UDT[uIdx].enroll[eIdx].audio = NULL;
                    t->user_UDT[uIdx].enroll[eIdx].audioLen = 0;
                }
                if (t->user_UDT[uIdx].enroll[eIdx].pronun)
                    free(t->user_UDT[uIdx].enroll[eIdx].pronun);
                t->user_UDT[uIdx].enroll[eIdx].pronun = NULL;
                sprintf(filename, "%sudt%i_%i.wav", t->savedir, uIdx, eIdx);
                remove(filename);
            }

            free(t->user_UDT[uIdx].enroll);
            t->user_UDT[uIdx].numEnroll = 0;
            t->user_UDT[uIdx].enroll = NULL;
        }
    }

    free(filename);
    return 1;

}

// ============================================================================
// Resets all recognizers
void Java_com_sprd_voicetrigger_nativeinterface_Recog_resetRecog(JNIEnv *env,
                                                                 jobject job, jlong arg) {
    trig_t *t = (trig_t *) (long) arg;

    if (t->recogEnrollSD_EFT)
        thfRecogReset(t->thf, t->recogEnrollSD_EFT);
    if (t->recogEnrollNoSD_EFT)
        thfRecogReset(t->thf, t->recogEnrollNoSD_EFT);
    if (t->recogEnroll_UDT)
        thfRecogReset(t->thf, t->recogEnroll_UDT);
    if (t->enrollrecogEnroll_UDT)
        thfRecogReset(t->thf, t->enrollrecogEnroll_UDT);
    if (t->recogTest_COMB)
        thfRecogReset(t->thf, t->recogTest_COMB);
    return;
}

// ============================================================================
// Close down recognizer and SDK session and frees associated memory
void Java_com_sprd_voicetrigger_nativeinterface_Recog_closeRecog(JNIEnv *env,
                                                                 jobject job, jlong arg) {
    int i, j;
    trig_t *t = (trig_t *) (long) arg;

    // EFT STUFF  ============================================

    if (t->recogEnrollSD_EFT)
        thfRecogDestroy(t->recogEnrollSD_EFT);
    if (t->recogEnrollNoSD_EFT)
        thfRecogDestroy(t->recogEnrollNoSD_EFT);
    if (t->user_EFT) {
        for (i = 0; i < t->numUsers_UDT; i++) {
            if (t->user_EFT[i].userName)
                free(t->user_EFT[i].userName);
            if (t->user_EFT[i].tag)
                free(t->user_EFT[i].tag);
            if (t->user_EFT[i].enroll) {
                for (j = 0; j < t->user_EFT[i].numEnroll; j++) {
                    if (t->user_EFT[i].enroll[j].audio)
                        free(t->user_EFT[i].enroll[j].audio);
                    if (t->user_EFT[i].enroll[j].filename)
                        free(t->user_EFT[i].enroll[j].filename);
                    if (t->user_EFT[i].enroll[j].pronun)
                        free(t->user_EFT[i].enroll[j].pronun);
                }
                free(t->user_EFT[i].enroll);
            }
        }
        free(t->user_EFT);
    }

    // UDT STUFF  ============================================

    if (t->recogEnroll_UDT)
        thfRecogDestroy(t->recogEnroll_UDT);
    if (t->enrollrecogEnroll_UDT)
        thfRecogDestroy(t->enrollrecogEnroll_UDT);
    if (t->udt)
        thfUdtDestroy(t->udt);
    if (t->user_UDT) {
        for (i = 0; i < t->numUsers_UDT; i++) {
            if (t->user_UDT[i].userName)
                free(t->user_UDT[i].userName);
            if (t->user_UDT[i].tag)
                free(t->user_UDT[i].tag);
            if (t->user_UDT[i].enroll) {
                for (j = 0; j < t->user_UDT[i].numEnroll; j++) {
                    if (t->user_UDT[i].enroll[j].audio)
                        free(t->user_UDT[i].enroll[j].audio);
                    if (t->user_UDT[i].enroll[j].filename)
                        free(t->user_UDT[i].enroll[j].filename);
                    if (t->user_UDT[i].enroll[j].pronun)
                        free(t->user_UDT[i].enroll[j].pronun);
                }
                free(t->user_UDT[i].enroll);
            }
        }
        free(t->user_UDT);
    }

    // Common STUFF  ============================================

    if (t->recogTest_COMB)
        thfRecogDestroy(t->recogTest_COMB);
    if (t->searchTest_COMB)
        thfSearchDestroy(t->searchTest_COMB);
    if (t->savedir)
        free(t->savedir);
    if (t->thf)
        thfSessionDestroy(t->thf);
    if (t)
        free(t);
}


// ============================================================================
// Close down recognizer and SDK session and frees associated memory
void Java_com_sprd_voicetrigger_commands_Recog_closeRecog(JNIEnv *env,
                                                          jobject job, jlong arg) {
    trig_p *t = (trig_p *) (
            long) arg;
    thf_t *thf = t->thf;
    recog_t *trigr = t->trig_r;
    thfRecogDestroy(trigr);
    trigr = NULL;
    searchs_t *trigs = t->trig_s;
    thfSearchDestroy(trigs);
    trigs = NULL;
    recog_t *commr = t->trig_r;
    thfRecogDestroy(commr);
    commr = NULL;
    searchs_t *comms = t->trig_s;
    thfSearchDestroy(comms);
    comms = NULL;
    thfSessionDestroy(thf);
    thf = NULL;
    if (t->result)free(t->result);
    free(t);
    t = NULL;
    return;
}
