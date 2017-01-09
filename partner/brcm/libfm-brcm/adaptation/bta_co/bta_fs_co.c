/*****************************************************************************
**
**  Name:           bta_fs_co.c
**
**  Description:    This file contains the file system call-out
**                  function implementation for Insight.
**
**  Copyright (c) 2003-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

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
#include "gki.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"
#include "btui.h"
#include "btui_int.h"

#define FAT_FS 0x4d44
const int BT_UID= 1002;
const int BT_GID= 1002;


static int del_path (const char *path)
{
    DIR *dir;
    struct dirent *de;
    int ret = 0;
    char nameBuffer[PATH_MAX] = {0};
    struct stat statBuffer;
    APPL_TRACE_DEBUG1("in del_path for path:%s", path);
    dir = opendir(path);

    if (dir == NULL) {
        APPL_TRACE_DEBUG1("opendir failed on path:%s", path);
        return -1;
    }

    char *filenameOffset;

    strncpy(nameBuffer, path, PATH_MAX - 1);
    strcat(nameBuffer, "/");
    int nameLen = strlen(nameBuffer);
    filenameOffset = nameBuffer + nameLen;

    for (;;) {
        de = readdir(dir);

        if (de == NULL) {
            APPL_TRACE_DEBUG1("readdir failed for path:%s", path);
            //ret = -1;
            break;
        }

        if (0 == strcmp(de->d_name, ".") || 0 == strcmp(de->d_name, ".."))
           continue;

        if(strlen(de->d_name) > PATH_MAX - nameLen) {
            APPL_TRACE_DEBUG1("d_name len:%d is too big", strlen(de->d_name));
            ret = -1;
            break;
        }

        strcpy(filenameOffset, de->d_name);

        ret = lstat (nameBuffer, &statBuffer);

        if (ret != 0) {
            APPL_TRACE_DEBUG1("lstat failed for path:%s", nameBuffer);
            break;
        }

        if(S_ISDIR(statBuffer.st_mode)) {

            ret = del_path(nameBuffer);
            if(ret != 0)
                break;
        } else {
            ret = unlink(nameBuffer);
            if (ret != 0) {
                APPL_TRACE_DEBUG1("unlink failed for path:%s", nameBuffer);
                break;
            }
        }
    }

    closedir(dir);
    if(ret == 0) {
        ret = rmdir(path);
        APPL_TRACE_DEBUG2("rmdir return:%d for path:%s", ret, path);
    }

    return ret;

}

inline int getAccess(int accType, struct stat *buffer, char *p_path)
{

    struct statfs fsbuffer;
    int idType;

    if(! buffer)
	return BTA_FS_CO_FAIL;

    idType= (buffer->st_uid== BT_UID) ? 1 : (buffer->st_uid== BT_GID) ? 2 : 3;
    if(statfs(p_path, &fsbuffer)==0)
    {
        if(fsbuffer.f_type == FAT_FS)
	    return BTA_FS_CO_OK;
    }
    else {
        return BTA_FS_CO_FAIL;
    }

    switch(accType) {
        case 4:
	if(idType== 1) {	//Id is User Id
	   if(buffer-> st_mode & S_IRUSR)
	       return BTA_FS_CO_OK;
	}
	else if(idType==2) {   //Id is Group Id
	    if(buffer-> st_mode & S_IRGRP)
	       return BTA_FS_CO_OK;
	}
	else {			//Id is Others
	    if(buffer-> st_mode & S_IROTH)
	       return BTA_FS_CO_OK;
	}
	break;

	case 6:
	if(idType== 1) {	//Id is User Id
	   if((buffer-> st_mode & S_IRUSR) && (buffer-> st_mode & S_IWUSR))
	       return BTA_FS_CO_OK;
	}
	else if(idType==2) {   //Id is Group Id
	    if((buffer-> st_mode & S_IRGRP) && (buffer-> st_mode & S_IWGRP))
	       return BTA_FS_CO_OK;
	}
	else {			//Id is Others
	    if((buffer-> st_mode & S_IROTH) && (buffer-> st_mode & S_IWOTH))
	       return BTA_FS_CO_OK;
	}
	break;

	default:
	return BTA_FS_CO_OK;
    }
    APPL_TRACE_DEBUG0("*************FTP- Access Failed **********");
    return BTA_FS_CO_EACCES;
}


/*****************************************************************************
**  Function Declarations
*****************************************************************************/

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
** Function         bta_fs_co_open
**
** Description      This function is executed by BTA when a file is opened.
**                  The phone uses this function to open
**                  a file for reading or writing.
**
** Parameters       p_path  - Fully qualified path and file name.
**                  oflags  - permissions and mode (see constants above)
**                  size    - size of file to put (0 if unavailable or not applicable)
**                  evt     - event that must be passed into the call-in function.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, a file descriptor (int),
**                        if successful, and an error code (tBTA_FS_CO_STATUS)
**                        are returned in the call-in function, bta_fs_ci_open().
**
*******************************************************************************/
void bta_fs_co_open(const char *p_path, int oflags, UINT32 size, UINT16 evt,
                    UINT8 app_id)
{
    tBTA_FS_CO_STATUS  status;
    UINT32  file_size = 0;
    struct  stat file_stat;
    int fd;
    int err=0;

    /* Convert BTA oflags into MFS */
    oflags = bta_fs_convert_bta_oflags(oflags);

    if ((fd = open (p_path, oflags, 0666)) < 0)
    {
        err = errno;
        status = (err == EACCES) ? BTA_FS_CO_EACCES : BTA_FS_CO_FAIL;
    }
    else
    {
        status = BTA_FS_CO_OK;
        if(fstat(fd, &file_stat) == 0)
        {
            file_size = file_stat.st_size;
	    if (oflags & O_CREAT) {
			fchown(fd, BT_UID, BT_GID);
		APPL_TRACE_DEBUG0("\n ******CHANGED OWNERSHIP SUCCESSFULLY**********");
	    }
        }
    }
    APPL_TRACE_DEBUG4("[CO] bta_fs_co_open: handle:%d err:%d, flags:%x, app id:%d",
                      fd, err, oflags, app_id);
    APPL_TRACE_DEBUG1("file=%s", p_path);

    bta_fs_ci_open(fd, status, file_size, evt);
}

/*******************************************************************************
**
** Function         bta_fs_co_close
**
** Description      This function is called by BTA when a connection to a
**                  client is closed.
**
** Parameters       fd      - file descriptor of file to close.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                      [BTA_FS_CO_OK if successful],
**                      [BTA_FS_CO_FAIL if failed  ]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_close(int fd, UINT8 app_id)
{
    tBTA_FS_CO_STATUS status;
    int err;

    APPL_TRACE_DEBUG2("[CO] bta_fs_co_close: handle:%d, app id:%d",
        fd, app_id);
    if ((status = close (fd)) < 0)
    {
        err = errno;
        status = BTA_FS_CO_FAIL;
        APPL_TRACE_WARNING3("[CO] bta_fs_co_close: handle:%d error=%d app_id:%d", fd, err, app_id);
    }

    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_read
**
** Description      This function is called by BTA to read in data from the
**                  previously opened file on the phone.
**
** Parameters       fd      - file descriptor of file to read from.
**                  p_buf   - buffer to read the data into.
**                  nbytes  - number of bytes to read into the buffer.
**                  evt     - event that must be passed into the call-in function.
**                  ssn     - session sequence number. Ignored, if bta_fs_co_open
**							  was not called with BTA_FS_CO_RELIABLE.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, bta_fs_ci_read() is
**                        called with the buffer of data, along with the number
**                        of bytes read into the buffer, and a status.  The
**                        call-in function should only be called when ALL requested
**                        bytes have been read, the end of file has been detected,
**                        or an error has occurred.
**
*******************************************************************************/
void bta_fs_co_read(int fd, UINT8 *p_buf, UINT16 nbytes, UINT16 evt, UINT8 ssn, UINT8 app_id)
{
    tBTA_FS_CO_STATUS  status = BTA_FS_CO_OK;
    INT32   num_read;
    int     err;

    if ((num_read = read (fd, p_buf, nbytes)) < 0)
    {
        err = errno;
        status = BTA_FS_CO_FAIL;
        APPL_TRACE_WARNING3("[CO] bta_fs_co_read: handle:%d error=%d app_id:%d",
                            fd, err, app_id);
    }
    else if (num_read < nbytes)
        status = BTA_FS_CO_EOF;

    bta_fs_ci_read(fd, (UINT16)num_read, status, evt);
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
**                  evt     - event that must be passed into the call-in function.
**                  ssn     - session sequence number. Ignored, if bta_fs_co_open
**							  was not called with BTA_FS_CO_RELIABLE.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, bta_fs_ci_write() is
**                        called with the file descriptor and the status.  The
**                        call-in function should only be called when ALL requested
**                        bytes have been written, or an error has been detected,
**
*******************************************************************************/
void bta_fs_co_write(int fd, const UINT8 *p_buf, UINT16 nbytes, UINT16 evt,
                     UINT8 ssn, UINT8 app_id)
{
    tBTA_FS_CO_STATUS  status = BTA_FS_CO_OK;
    INT32   num_written;
    int     err=0;

    if ((num_written = write (fd, p_buf, nbytes)) < 0)
    {
        err = errno;
        status = BTA_FS_CO_FAIL;
    }
/*    APPL_TRACE_DEBUG3("[CO] bta_fs_co_write: handle:%d error=%d, num_written:%d", fd, err, num_written);*/

    bta_fs_ci_write(fd, status, evt);
}

/*******************************************************************************
**
** Function         bta_fs_co_seek
**
** Description      This function is called by io to move the file pointer
**                  of a previously opened file to the specified location for
**                  the next read or write operation.
**
** Parameters       fd      - file descriptor of file.
**                  offset  - Number of bytes from origin.
**                  origin  - Initial position.
**
** Returns          void
**
*******************************************************************************/
void bta_fs_co_seek (int fd, INT32 offset, INT16 origin, UINT8 app_id)
{
    lseek(fd, offset, origin);
}

/*******************************************************************************
**
** Function         bta_fs_co_access
**
** Description      This function is called to check the existence of
**                  a file or directory, and return whether or not it is a
**                  directory or length of the file.
**
** Parameters       p_path   - (input) file or directory to access (fully qualified path).
**                  mode     - (input) [BTA_FS_ACC_EXIST, BTA_FS_ACC_READ, or BTA_FS_ACC_RDWR]
**                  p_is_dir - (output) returns TRUE if p_path specifies a directory.
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                   [BTA_FS_CO_OK if it exists]
**                   [BTA_FS_CO_EACCES if permissions are wrong]
**                   [BTA_FS_CO_FAIL if it does not exist]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_access(const char *p_path, int mode, BOOLEAN *p_is_dir,
                                   UINT8 app_id)
{
    int err;
    int os_mode = 0;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;
    struct stat buffer;

    if (app_id == UI_PBS_ID)
    {
        /* the directory structure for PBAP at android is useless */
        *p_is_dir = TRUE;
        return (status);
    }

    *p_is_dir = FALSE;

    if (mode == BTA_FS_ACC_RDWR)
        os_mode = 6;
    else if (mode == BTA_FS_ACC_READ)
        os_mode = 4;

    if (stat(p_path, &buffer) == 0)
    {
	/* Determine if the object is a file or directory */
        if (S_ISDIR(buffer.st_mode))
            *p_is_dir = TRUE;
    }
    else
    {
	APPL_TRACE_DEBUG0("stat() failed! ");
        return BTA_FS_CO_FAIL;
    }

    status=getAccess (os_mode, &buffer, p_path);
    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_mkdir
**
** Description      This function is called to create a directory with
**                  the pathname given by path. The pathname is a null terminated
**                  string. All components of the path must already exist.
**
** Parameters       p_path   - (input) name of directory to create (fully qualified path).
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                  [BTA_FS_CO_OK if successful]
**                  [BTA_FS_CO_FAIL if unsuccessful]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_mkdir(const char *p_path, UINT8 app_id)
{
    int err;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;

    if ((mkdir (p_path, 0666)) != 0)
    {
        err = errno;
        status = BTA_FS_CO_FAIL;
        APPL_TRACE_WARNING3("[CO] bta_fs_co_mkdir: error=%d, path [%s] app_id:%d",
                            err, p_path, app_id);
    }
    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_rmdir
**
** Description      This function is called to remove a directory whose
**                  name is given by path. The directory must be empty.
**
** Parameters       p_path   - (input) name of directory to remove (fully qualified path).
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                      [BTA_FS_CO_OK if successful]
**                      [BTA_FS_CO_EACCES if read-only]
**                      [BTA_FS_CO_ENOTEMPTY if directory is not empty]
**                      [BTA_FS_CO_FAIL otherwise]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_rmdir(const char *p_path, UINT8 app_id)
{
    APPL_TRACE_DEBUG0("bta_fs_co_rmdir");
    int     err;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;
    struct stat buffer;
    char *dirName, *tmp=NULL;

    dirName= (char*) calloc(1, strlen(p_path));
    strncpy(dirName, p_path, strlen(p_path));

    if(tmp=strrchr(dirName, '/'))
    {
	*tmp='\0';
    }
    if (stat(dirName, &buffer) == 0)
    {
	status=getAccess (6, &buffer, dirName);
    }
    else
    {
        free(dirName);
        return BTA_FS_CO_FAIL;
    }

    free(dirName);
    if(status!= BTA_FS_CO_OK)
        return status;

    if (stat(p_path, &buffer) == 0)
    {
	status=getAccess (6, &buffer, p_path);
    }
    else
    {
        return BTA_FS_CO_FAIL;
    }

    if(status!= BTA_FS_CO_OK)
        return status;

    //if ((rmdir (p_path)) != 0)
    if(del_path(p_path) != 0)
    {
        err = errno;
        if (err == EACCES)
            status = BTA_FS_CO_EACCES;
        else if (err == ENOTEMPTY)
            status = BTA_FS_CO_ENOTEMPTY;
        else
            status = BTA_FS_CO_FAIL;
    }
    return (status);
}

/*******************************************************************************
**
** Function         bta_fs_co_unlink
**
** Description      This function is called to remove a file whose name
**                  is given by p_path.
**
** Parameters       p_path   - (input) name of file to remove (fully qualified path).
**                  app_id   - (input) application ID specified in the enable functions.
**                                     It can be used to identify which profile is the caller
**                                     of the call-out function.
**
** Returns          (tBTA_FS_CO_STATUS) status of the call.
**                      [BTA_FS_CO_OK if successful]
**                      [BTA_FS_CO_EACCES if read-only]
**                      [BTA_FS_CO_FAIL otherwise]
**
*******************************************************************************/
tBTA_FS_CO_STATUS bta_fs_co_unlink(const char *p_path, UINT8 app_id)
{
    APPL_TRACE_DEBUG0("bta_fs_co_unlink");
    int err;
    tBTA_FS_CO_STATUS status = BTA_FS_CO_OK;
    char *dirName, *tmp=NULL;
    struct stat buffer;

    if(! p_path)
        return BTA_FS_CO_FAIL;

    /* buffer needs to be NULL terminated - so add one more byte to be zero'd out */
#if 0
    dirName= (char*) calloc(1, strlen(p_path));  /* <--- this can cause problems  */
#else
    dirName= (char*) calloc(1, strlen(p_path) + 1);
#endif

    strncpy(dirName, p_path, strlen(p_path));
    if(tmp=strrchr(dirName, '/'))
    {
	*tmp='\0';
    }
    if (stat(dirName, &buffer) == 0)
    {
        status=getAccess (6, &buffer, dirName);
        free(dirName);
    }
    else
    {
        APPL_TRACE_DEBUG0("stat() failed! ");
        free(dirName);
        return BTA_FS_CO_FAIL;
    }

    if(status!= BTA_FS_CO_OK)
	return status;

    if ((unlink (p_path)) != 0)
    {
        err = errno;
        if (err == EACCES)
            status = BTA_FS_CO_EACCES;
        else
            status = BTA_FS_CO_FAIL;
    }
    return (status);

}

/*******************************************************************************
**
** Function         bta_fs_co_getdirentry
**
** Description      This function is called to get a directory entry for the
**                  specified p_path.  The first/next directory should be filled
**                  into the location specified by p_entry.
**
** Parameters       p_path      - directory to search (Fully qualified path)
**                  first_item  - TRUE if first search, FALSE if next search
**                                      (p_cur contains previous)
**                  p_entry (input/output) - Points to last entry data (valid when
**                                           first_item is FALSE)
**                  evt     - event that must be passed into the call-in function.
**                  app_id  - application ID specified in the enable functions.
**                            It can be used to identify which profile is the caller
**                            of the call-out function.
**
** Returns          void
**
**                  Note: Upon completion of the request, the status is passed
**                        in the bta_fs_ci_direntry() call-in function.
**                        BTA_FS_CO_OK is returned when p_entry is valid,
**                        BTA_FS_CO_EODIR is returned when no more entries [finished]
**                        BTA_FS_CO_FAIL is returned if an error occurred
**
*******************************************************************************/
void bta_fs_co_getdirentry(const char *p_path, BOOLEAN first_item,
                           tBTA_FS_DIRENTRY *p_entry, UINT16 evt, UINT8 app_id)
{
    tBTA_FS_CO_STATUS    co_status = BTA_FS_CO_FAIL;
    int                  status = -1;    /* '0' - success, '-1' - fail */
    struct tm           *p_tm;
    DIR *dir;
    struct dirent *dirent;
    struct stat buf;
    char fullname[500];

	APPL_TRACE_DEBUG0("Entered bta_fs_co_getdirentry");

    /* First item is to be retrieved */
    if (first_item)
    {
        APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: path = %s", p_path);

        dir = opendir(p_path);
        if(dir == NULL)
        {
			APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: dir is NULL so error out with errno=%d", errno);
			bta_fs_ci_direntry(co_status, evt);
	}
	APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: dir = %p", dir);
    if((dirent = readdir(dir)) != NULL)
    {
            p_entry->refdata = (UINT32) dir;     /* Save this for future searches */
            status = 0;
			APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: dirent = %p", dirent);
	}
	else
	{
	    APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: dirent = %p", dirent);
            /* Close the search if there are no more items */
            closedir( (long) p_entry->refdata);
            co_status = BTA_FS_CO_EODIR;
	 }
    }
    else    /* Get the next entry based on the p_ref data from previous search */
    {
        if ((dirent = readdir((long)p_entry->refdata))  == NULL)
        {
            /* Close the search if there are no more items */
            closedir( (long) p_entry->refdata);
            co_status = BTA_FS_CO_EODIR;
            APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: dirent = %p", dirent);
        }
	else
	{
            APPL_TRACE_DEBUG1("bta_fs_co_getdirentry: dirent = %p", dirent);
	    status = 0;
	}
    }

    if (status == 0)
    {
        APPL_TRACE_DEBUG0("bta_fs_co_getdirentry: status = 0");

        sprintf(fullname, "%s/%s", p_path,  dirent->d_name);

        /* Load new values into the return structure (refdata is left untouched) */
        if (stat(fullname, &buf) == 0) {
            p_entry->filesize = buf.st_size;
            p_entry->mode = 0; /* Default is normal read/write file access */

            if (S_ISDIR(buf.st_mode))
                p_entry->mode |= BTA_FS_A_DIR;
            else
                p_entry->mode |= BTA_FS_A_RDONLY;

            strcpy(p_entry->p_name, dirent->d_name);
#if 0
            fprintf(stderr, "bta_fs_co_getdirentry(): %s %9d %d\n",
                            dirent->d_name,
                            buf.st_size,
                            p_entry->mode);
#endif
            p_tm = localtime(&buf.st_mtime);
            if (p_tm != NULL)
            {
                sprintf(p_entry->crtime, "%04d%02d%02dT%02d%02d%02dZ",
                        p_tm->tm_year + 1900,   /* Base Year ISO 6201 */
                        p_tm->tm_mon,
                        p_tm->tm_mday,
                        p_tm->tm_hour,
                        p_tm->tm_min,
                        p_tm->tm_sec);
            }
            else
                p_entry->crtime[0] = '\0';  /* No valid time */
#if 0
            fprintf(stderr, "bta_fs_co_getdirentry(): %s %9d %d %s\n",
                            dirent->d_name,
                            p_entry->filesize,
                            p_entry->mode,
                            p_entry->crtime);
#endif
            co_status = BTA_FS_CO_OK;
        } else {
            APPL_TRACE_WARNING0("stat() failed! ");
            co_status = BTA_FS_CO_EACCES;
        }
    }
    APPL_TRACE_DEBUG0("bta_fs_co_getdirentry: calling bta_fs_ci_getdirentry");

    bta_fs_ci_direntry(co_status, evt);
}
