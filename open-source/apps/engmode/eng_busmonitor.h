#ifndef _ENG_BUSMONITOR_H_
#define _ENG_BUSMONITOR_H_

typedef struct
{
	unsigned int ReadCount;
	unsigned int WriteCount;
	unsigned int ReadLatency;
	unsigned int WriteLatency;
	unsigned int ReadBandWidth;
	unsigned int WriteBandWidth;
}__attribute__((packed)) BUSMONITOR_CHANNEL_DATA;

typedef struct
{
    char chan_name[16];
}__attribute__((packed)) BUSMONITOR_CHAN_NAME;

int eng_diag_busmonitor_enable(char *buf, int len, char *rsp, int rsplen);
int eng_diag_busmonitor_disable(char *buf, int len, char *rsp, int rsplen);
int eng_diag_busmonitor_get_chaninfo(char *buf, int len, char *rsp, int rsplen);
int eng_diag_busmonitor_get_rtctime(char *buf, int len, char *rsp, int rsplen);
int eng_diag_busmonitor_get_monitordata(char *buf, int len, char *rsp, int rsplen);
#endif

