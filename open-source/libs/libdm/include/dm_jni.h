

#ifndef _DM_JNI_H_
#define _DM_JNI_H_


void spdm_sendDataCb(char * data, int datalen, int finishflag, char *uri);

void spdm_openDialogCb(int type, char * message, char * title, int timeout); 

void spdm_writeNullCb(int handletype);

void spdm_writeCb(int handletype, char * data, int offset, int size);

void spdm_readCb(int handletype, char * buf, int offset, int bufsize);

void spdm_exitNotifyCb(int reason);

#endif







































