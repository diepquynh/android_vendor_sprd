#ifndef _ENG_PRODUCTDATA_H
#define _ENG_PRODUCTDATA_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int eng_read_productnvdata(char *databuf,  int data_len);
int eng_write_productnvdata(char *databuf,  int data_len);

#ifdef __cplusplus
}
#endif

#endif
