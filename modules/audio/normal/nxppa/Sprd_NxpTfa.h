#ifndef __SPRD_NXPTFA9890_INTERFACE__
#define  __SPRD_NXPTFA9890_INTERFACE__

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <utils/Log.h>
#include <tinyalsa/asoundlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/ioctl.h>



struct NxpTpa_Info {
        const char *pCardName;
        int dev;
        struct pcm_config pcmConfig;
};

typedef void* nxp_pa_handle;


nxp_pa_handle NxpTfa_Open(struct NxpTpa_Info *info);
int NxpTfa_Write( nxp_pa_handle handle , const void *data, unsigned int count);
void  NxpTfa_Close( nxp_pa_handle handle );

#endif //__SPRD_NXPTFA9890_INTERFACE__





