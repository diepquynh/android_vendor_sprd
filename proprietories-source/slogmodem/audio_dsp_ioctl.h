/*
 *  audio_dsp_ioctl.h - The AG-DSP ioctl commands.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-2-15 Peter Huang
 *  Initial version.
 */
#ifndef _AUDIO_DSP_IOCTL_H
#define _AUDIO_DSP_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define DSPLOG_CMD_MAGIC 'X'

#define DSPLOG_CMD_LOG_ENABLE           _IOW(DSPLOG_CMD_MAGIC, 0, int)
#define DSPLOG_CMD_LOG_PATH_SET         _IOW(DSPLOG_CMD_MAGIC, 1, int)
#define DSPLOG_CMD_LOG_PACKET_ENABLE    _IOW(DSPLOG_CMD_MAGIC, 2, int)
#define DSPLOG_CMD_PCM_PATH_SET         _IOW(DSPLOG_CMD_MAGIC, 3, int)
#define DSPLOG_CMD_PCM_ENABLE           _IOW(DSPLOG_CMD_MAGIC, 4, int)
#define DSPLOG_CMD_PCM_PACKET_ENABLE    _IOW(DSPLOG_CMD_MAGIC, 5, int)
#define DSPLOG_CMD_DSPASSERT            _IOW(DSPLOG_CMD_MAGIC, 6, int)

#endif  //!_AUDIO_DSP_IOCTL_H
