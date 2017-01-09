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
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>

#include "bmessage/bmessage_support.h"
#include "bmessage/bmessage_co.h"

const int BT_UID= 1002;
const int BT_GID= 1002;


/*******************************************************************************
**
** Function         bta_fs_convert_oflags
**
** Description      This function converts the open flags from BTA into MFS.
**
** Returns          BTA FS status value.
**
*******************************************************************************/
int bta_fs_convert_bta_oflags(int bta_oflags)
{
    int oflags = 0; /* Initially read only */

    /* Only one of these can be set: Read Only, Read/Write, or Write Only */
    if (bta_oflags & BTA_FS_O_RDWR)
        oflags |= O_RDWR;
    else if (bta_oflags & BTA_FS_O_WRONLY)
        oflags |= O_WRONLY;

    /* OR in any other flags that are set by BTA */
    if (bta_oflags & BTA_FS_O_CREAT)
        oflags |= O_CREAT;

    if (bta_oflags & BTA_FS_O_EXCL)
        oflags |= O_EXCL;

    if (bta_oflags & BTA_FS_O_TRUNC)
        oflags |= O_TRUNC;

    return (oflags);
}

/*******************************************************************************
**
** Function         bta_ma_co_open
**
** Description      This function is executed by BTA BMessage parser/builder
**                  when a file is opened. The phone uses this function to open
**                  a file for reading or writing.
**
** Parameters       p_path  - Fully qualified path and file name.
**                  oflags  - permissions and mode (see constants above)
**                            of the call-out function.
**
** Returns          int - a file descriptor, if successful, or BTA_FS_INVALID_FD
**
*******************************************************************************/

int bta_ma_co_open(const char *p_path, int oflags) {
    struct stat file_stat;
    int fd;
    /* Convert BTA oflags into MFS */
    oflags = bta_fs_convert_bta_oflags(oflags);


    if ((fd = open (p_path, oflags | O_NONBLOCK, 0666)) < 0)
    {
      APPL_TRACE_ERROR2("%s: Error opening file: error code %d", __FUNCTION__, fd);
      return BTA_FS_INVALID_FD;
    }
    else
    {
      if(fstat(fd, &file_stat) == 0)
	{
	  if (oflags & O_CREAT) {
	    fchown(fd, BT_UID, BT_GID);
	    APPL_TRACE_DEBUG0("\n ******CHANGED OWNERSHIP SUCCESSFULLY**********");
	  }
	}
      return fd;
    }
}



/*******************************************************************************
**
** Function         bta_fs_co_write
**
** Description      This function is called by io to send file data to the
**                  phone.
**
** Parameters       fd      - file descriptor of file to write to.
**                  p_buf   - buffer to read the data from.
**                  nbytes  - number of bytes to write out to the file.
**
** Returns          Number of bytes written
**
**
*******************************************************************************/
int  bta_ma_co_write(int fd, const void *p_buf, int nbytes)
{

  int   num_written;

  if ((num_written = write (fd, p_buf, nbytes)) < 0)
  {
    APPL_TRACE_ERROR3("%s: Error writing file to fd:%d. error=%d",
		      __FUNCTION__,fd,errno);
    return 0;
  }

  return num_written;
}




/*******************************************************************************
**
** Function         bta_ma_co_read
**
** Description      This function is called by BTA to read in data from the
**                  previously opened file on the phone.
**
** Parameters       fd      - file descriptor of file to read from.
**                  p_buf   - buffer to read the data into.
**                  nbytes  - number of bytes to read into the buffer.
**
** Returns          Number of bytes read
*******************************************************************************/

int bta_ma_co_read(int fd, void *p_buf, int nbytes)
{

  INT32   num_read;
  if ((num_read = read (fd, p_buf, nbytes)) < 0)
  {
    APPL_TRACE_ERROR3("%s: Error read file from fd:%d. error=%d",
		      __FUNCTION__,fd,errno);
    return 0;
  }

  else
  {
    return num_read;
  }

}


/*******************************************************************************
**
** Function         bta_fs_co_close
**
** Description      This function is called by BTA when a connection to a
**                  client is closed.
**
** Parameters       fd      - file descriptor of file to close.
**
** Returns          void
*******************************************************************************/
void bta_ma_co_close(int fd)
{
    INT32 status;
    APPL_TRACE_DEBUG2("%s: fd:%d",__FUNCTION__,fd);
    if ((status = close (fd)) < 0) {
      APPL_TRACE_ERROR3("%s: fd:%d error=%d", __FUNCTION__, fd, errno);
    }
}
