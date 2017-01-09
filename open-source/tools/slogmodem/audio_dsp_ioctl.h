#ifndef _AUDIO_DSP_IOCTL_H
#define _AUDIO_DSP_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define DSPLOG_CMD_MARGIC 'X'

#define DSPLOG_CMD_LOG_ENABLE                           _IOW(DSPLOG_CMD_MARGIC, 0, int)
#define DSPLOG_CMD_LOG_PATH_SET		             _IOW(DSPLOG_CMD_MARGIC, 1, int)
#define DSPLOG_CMD_LOG_PACKET_ENABLE		_IOW(DSPLOG_CMD_MARGIC, 2, int)
#define DSPLOG_CMD_PCM_PATH_SET		             _IOW(DSPLOG_CMD_MARGIC, 3, int)
#define DSPLOG_CMD_PCM_ENABLE		             _IOW(DSPLOG_CMD_MARGIC, 4, int)
#define DSPLOG_CMD_PCM_PACKET_ENABLE	      _IOW(DSPLOG_CMD_MARGIC, 5, int)
#define DSPLOG_CMD_DSPASSERT		                    _IOW(DSPLOG_CMD_MARGIC, 6, int)
#endif
