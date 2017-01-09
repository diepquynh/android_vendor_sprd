#ifndef __SPRD_SPK_FM_INTERFACE__
#define  __SPRD_SPK_FM_INTERFACE__

#include "sprdFmTrack.h"


struct SprdFm_Info {
        const char *pCardName;
        int dwPortNo;
        struct pcm_config pcmConfig;
};
typedef void * fm_handle;
int NxpTfa_Fm_Start(fm_handle handle);
int NxpTfa_Fm_Stop( fm_handle handle);
fm_handle NxpTfa_Fm_Open(void);
int NxpTfa_Fm_Close(fm_handle handle);


#endif
