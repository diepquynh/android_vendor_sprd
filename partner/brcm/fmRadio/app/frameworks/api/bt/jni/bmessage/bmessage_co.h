/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ************************************************************************************/
#ifndef BTA_MA_CO_H
#define BTA_MA_CO_H

#include <stdio.h>

/*****************************************************************************
**
** Utility functions for converting types to strings.
**
*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         bta_ma_co_open
**
** Description      Open a file.
**
** Parameters       p_path - Full path of file to open.
**                  oflags - file open flags.
**
** Returns          file descriptor.  BTA_FS_INVALID_FD if open fails.
**
*******************************************************************************/
    BTA_API extern int bta_ma_co_open(const char *p_path, int oflags);

/*******************************************************************************
**
** Function         bta_ma_co_write
**
** Description      Write data to file.
**
** Parameters       fd - file descriptor.
**                  buffer - data to write.
**                  size - size of data to write (in bytes).
**
** Returns          Number of bytes written.
**
*******************************************************************************/
    BTA_API extern  int bta_ma_co_write(int fd, const void *buffer, int size);

/*******************************************************************************
**
** Function         bta_ma_co_read
**
** Description      Read data from file.
**
** Parameters       fd - file descriptor.
**                  buffer - to receive data.
**                  size - amount of data to read (in bytes).
**
** Returns          Number of bytes read.
**
*******************************************************************************/
    BTA_API extern int bta_ma_co_read(int fd, void *buffer, int size);

/*******************************************************************************
**
** Function         bta_ma_co_close
**
** Description      Close the file.
**
** Parameters       fd - file descriptor.
**
** Returns          void
**
*******************************************************************************/
    BTA_API extern void bta_ma_co_close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* BTA_MA_FILE_H */


