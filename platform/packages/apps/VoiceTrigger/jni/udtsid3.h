typedef unsigned long u32;

#include "trulyhandsfree.h"

#define THROW(a) { ewhere=(a); goto error; }
#define THROW2(a, b) {ewhere=(a); ewhat=(b); goto error; }
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "udtsid",__VA_ARGS__)
#define PANIC(...) __android_log_print(ANDROID_LOG_ERROR, "udtsid",__VA_ARGS__)
#define printf(a) LOG(a)

#define NBEST               (1)
#define MAXSTR              (1024)
#define MAXFILENAME         (1024)
#define MAX_EFT             (1)             /* Maximum number of EFT passphrases */
#define MAX_UDT             (6)             /* Maximum number of UDT passphrases */
#define MAX_USERS           (MAX_EFT+MAX_UDT) /* Maximum number of total passphrases */
#define MAX_ENROLL          (5)             /* Maximum number of enrollment phrases (per user) */
#define PHRASESPOT_DELAY_SINGLE_USER (15)   /* Phrasespot delay : UDT mode */
#define PHRASESPOT_DELAY_MULTI_USER  (250)  /* Phrasespot delay : SID mode ( NOTE: SV requires a minimum of 250ms ) */
#define DONT_USE_EPQ        (0.f)           /* Don't Use energy post qualification */
#define USE_EPQ             (1.f)           /* Use energy post qualification */
#define SDET_LSILENCE       (60000.f)       /* Speech detector leading silence */
#define SDET_TSILENCE       (500.f)         /* Speech detector trailing silence */
#define SDET_MAXRECORD      (60000.f)       /* Speech detector max record duration (ms) */
#define SDET_MINDURATION    (350.f)         /* Speech detector min duration (ms) */
#define SDET_LONGTERMMS_UDT (600.f)         /* Speech detector long-term energy window (ms) */
#define SDET_LONGTERMMS_EFT (1000.f)        /* Speech detector long term window (ms)*/
#define SAMPLERATE          (16000)         /* Audio input sample rate */
#define UDT_PARAMAOFFSET    (-1800.f)       /* UDT ParamA Offset level */
#define UDT_PARAMAOFFSET_STEP (600.f)       /* UDT ParamA Offset level step size */
#define EFT_PARAMAOFFSET    (0.f)           /* EFT ParamA Offset level */
#define EFT_PARAMAOFFSET_STEP (100.f)       /* EFT ParamA Offset level step size */
#define SEARCHFILE          "search.raw"    /* Combined search for all users */
#define RECOGFILE           "recog.raw"     /* Combined acoustic model for all users */
#define EMBEDDED_SEARCHFILE "gram.bin"      /* Deeply embedded search */
#define EMBEDDED_RECOGFILE  "net.bin"       /* Deeply embedded acoustic model */
#define BLOCKSZ              (2400)         /* 10 frames at 16kHz   */

typedef struct feedback_s {
    unsigned long general; // Indicates if there was a problem with any of the recordings
    unsigned long *details; // Indicates which specific problems with specific recording
    float phraseQuality; // Quality indicates whether phrase is likely to be a good trigger (e.g., short phrases make poor triggers).
} feedback_t;

typedef struct trig_s {
    thf_t *thf;                        // SDK session
    recog_t *recogEnroll_UDT;          // UDT phonemerec net
    recog_t *enrollrecogEnroll_UDT;    // UDT phonemerec net
    phsearch_t *searchEnroll_UDT;      // UDT phonemerec search
    udt_t *udt;                        // UDT udt object
    thfUser_t *user_UDT;               // UDT users array
    udt_t *udtCheckEnroll_UDT; // UDT udt struct used for checking enrollment during checkRecording stage
    thfUser_t *userCheckEnroll_UDT; // UDT temp user for checking enrollment during checkRecording stage
    short numUsers_UDT;                // UDT num users
    feedback_t feedback;
    recog_t *recogEnrollSD_EFT; // EFT net used for enrollment w/ speech detector
    recog_t *recogEnrollNoSD_EFT; // EFT net used for enrollment w/o speech detector
    searchs_t *searchEnroll_EFT;       // EFT enrollment search
    thfUser_t *user_EFT;               // EFT users array
    recog_t *recogCheckEnroll_EFT; // EFT net used for checking enrollment during checkRecording stage
    searchs_t *searchCheckEnroll_EFT; // EFT search used for checking enrollment during checkRecording stage
    thfUser_t *userCheckEnroll_EFT; // EFT temp user for checking enrollment during checkRecording stage
    short numUsers_EFT;                // EFT num users
    recog_t *recogTest_COMB;           // Combined test net
    searchs_t *searchTest_COMB;        // Combined test search
    char userName[MAX_USERS][MAXSTR];  // user name
    short maxUsers;                    // maximum number of users
    char *savedir;                     // save directory
    float svThresholdBase;             // SV threshold base value
    float svThresholdStep;             // SV threshold step value
    char *ftNetFile;                   // fixed trigger net file
    char *ftSearchFile;                // fixed trigger search file
    char *netfileName_UDT; // UDT acoustic model, ie nn_en_us_mfcc_16k_15_big_250_v5.1.1.raw
    char *enrollnetfileName_UDT;       // UDT acoustic model, for enrollment
    char *phsearchfileName_UDT; // UDT phoneme search file, ie phonemeSearch_enUS_2_0.raw
    char *udtsvsidfileName_UDT;      // UDT udtsvsid file, ie svsid_enUS_2_0.raw
    float paramAStart_UDT;             // UDT starting paramA
    float paramBStart_UDT;             // UDT starting paramB
    float checkSNR_UDT;                // UDT SNR threshold
    short udtMode;                     // UDT 0=trigger only, 1=trigger + SV
    char *netfileName_EFT; // EFT acoustic model, ie sensory_demo_hbg_en_us_thf_delivery04_pruned_am.raw
    char *searchfileName_EFT; // EFT search file, ie sensory_demo_hbg_en_us_thf_delivery04_pruned_search_9.raw
    char *antidatafileName_EFT;  // EFT anti data file, ie hbg_antiData_v6_0.raw
    float checkSNR_EFT;                // EFT SNR threshold
    float checkVowelDur_EFT;           // EFT Vowel Duration threshold
    short svMode;                      // EFT 0=trigger only, 1=trigger + SV
} trig_t;

typedef struct trig_y {
    thf_t *thf;
    recog_t *trig_r;
    searchs_t *trig_s;
    recog_t *comm_r;
    searchs_t *comm_s;
    char *result;
    const char *logfile;
} trig_p;
