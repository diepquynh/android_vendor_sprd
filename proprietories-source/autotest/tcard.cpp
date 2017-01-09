// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <dirent.h>
#include <fcntl.h>

#include "type.h"
#include "tcard.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_tcard {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

static const char TCARD_VOLUME_NAME[]  = "sdcard";
static char TCARD_DEV_NAME[128];
static const char SYS_BLOCK_PATH[]     = "/sys/block";
static const char TCARD_FILE_NAME[]    = "/mnt/sdcard/sciautotest";
static const char TCARD_TEST_CONTENT[] = "SCI TCard: 2012-11-12";
//------------------------------------------------------------------------------
int tcard_get_dev_path(char*path)
{
	DIR *dir;
	struct dirent *de;
	int found = 0;
	char dirpath_open[128] = "/sys/devices";
	char tcard_name[16];

	sprintf(path, "../devices");

	dir = opendir(dirpath_open);
	if (dir == NULL) {
		DBGMSG("%s open fail\n", dirpath_open);
		return -1;
	}
	/*try to find dir: sdio_sd, dt branch*/
	/* sdio_sd in 5.0/5.1  replaces the sprd-sdhci.0 in sprdroid4.4*/
	while((de = readdir(dir))) {
		if(strncmp(de->d_name, "sdio_sd", strlen("sdio_sd"))) {
			continue;
		}
		sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
		sprintf(path, "%s/%s", path, de->d_name);
		found = 1;
		break;
	}
	/*try to find dir: sdio_sd,no dt branch*/
	if(!found) {
		sprintf(dirpath_open, "%s/platform", dirpath_open);
		closedir(dir);
		dir = opendir(dirpath_open);
		if(dir == NULL) {
			DBGMSG("%s open fail\n", dirpath_open);
			return -1;
		}
		while((de = readdir(dir))) {
			if(strncmp(de->d_name, "sdio_sd", strlen("sdio_sd"))) {
				continue;
			}
			sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
			sprintf(path, "%s/platform/%s", path, de->d_name);
			found = 1;
			break;
		}
	}
	closedir(dir);
	if(!found) {
		DBGMSG("sprd-sdhci.0 is not found \n");
		return -1;
	}
	found = 0;
	sprintf(dirpath_open, "%s/mmc_host", dirpath_open);
	dir = opendir(dirpath_open);
	if(dir == NULL) {
		DBGMSG("%s open fail\n", dirpath_open);
		return -1;
	}
	/*may be mmc0 or mmc1*/
	while((de = readdir(dir))) {
		if(strstr(de->d_name, "mmc") != NULL) {
			sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
			sprintf(path, "%s/mmc_host/%s", path, de->d_name);
			break;
		}
	}
	strncpy(tcard_name, de->d_name, sizeof(tcard_name)-1);
	if(strlen(de->d_name) < 16) {
		tcard_name[strlen(de->d_name)] = 0;
	} else {
		tcard_name[15] = 0;
	}
	closedir(dir);
	dir = opendir(dirpath_open);/*open dir: sprd-sdhci.0/mmc_host/mmc0 or mmc1*/
	if(dir == NULL) {
		DBGMSG("%s open fail\n", dirpath_open);
		return -1;
	}
	/* try to find: sprd-sdhci.0/mmc_host/mmcx/mmcx:xxxx,  if can find this dir ,T card work well */
	while((de = readdir(dir))) {
		if(strstr(de->d_name, tcard_name) == NULL) {
			continue;
		}
		sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
		sprintf(path, "%s/%s", path, de->d_name);
		found = 1;
		break;
	}
	closedir(dir);
	sprintf(dirpath_open, "%s/block",dirpath_open);
	dir = opendir(dirpath_open);
	if(dir == NULL) {
		DBGMSG("%s open fail\n", dirpath_open);
		return -1;
	}
	while((de = readdir(dir)))/*may be mmcblk0 or mmcblk1*/
	{
		if(strstr(de->d_name, "mmcblk") != NULL)
		{
			sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
			sprintf(path, "%s/block/%s", path, de->d_name);
			break;
		}
	}
	closedir(dir);
	if (found) {
		DBGMSG("the tcard dir is %s \n",path);
		return 0;
	}else {
		DBGMSG("the tcard dir is not found \n");
		return -1;
	}
}



//------------------------------------------------------------------------------
int tcardOpen( void )
{
	int ret = 0;
	ret = tcard_get_dev_path(TCARD_DEV_NAME);
	DBGMSG("ret %d \n",ret);
	return ret;
}

//------------------------------------------------------------------------------
int tcardIsPresent( void )
{
    DIR * dirBlck = opendir(SYS_BLOCK_PATH);
    if( NULL == dirBlck ) {
        ERRMSG("opendir '%s' fail: %s(%d)\n", SYS_BLOCK_PATH, strerror(errno), errno);
        return -1;
    }
    int present = -2;
    
    char realName[256];
    char linkName[128];
    strncpy(linkName, SYS_BLOCK_PATH, sizeof(linkName) - 1);
    char * pname = linkName + strlen(linkName);
    *pname++ = '/';
    
    struct dirent *de;
    while( (de = readdir(dirBlck)) ) {
        if (de->d_name[0] == '.' || DT_LNK != de->d_type )
            continue;

        strncpy(pname, de->d_name, 64);
    
        int len = readlink(linkName, realName, sizeof(realName)-1);
        if( len < 0 ) {
            ERRMSG("readlink error: %s(%d)\n", strerror(errno), errno);
            continue;
        }
		if(len < 256) {
			realName[len] = 0;
		} else {
			realName[255] = 0;
		}
        
        DBGMSG("link name = %s, real name = %s, TCARD_DEV_NAME=%s\n", linkName, realName,TCARD_DEV_NAME);
        if(strstr(realName, TCARD_DEV_NAME) != NULL) {
            present = 1;
            DBGMSG("TCard is present.\n");
            break;
        }
    }
    
    closedir(dirBlck);
    return present;
}

int tcardIsMount( void )
{
    char device[256];
    char mount_path[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        ERRMSG("Error opening /proc/mounts (%s)", strerror(errno));
        return -1;
    }

    int mount = 0;
    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s\n", device, mount_path);
        DBGMSG("dev = %s, mount = %s\n", device, mount_path);
        if( NULL != strstr(mount_path, TCARD_VOLUME_NAME)) {
            mount = 1;
            INFMSG("TCard mount path: '%s'\n", mount_path);
            break;
        }
    }

    fclose(fp);
    return mount;
}

int tcardRWTest( void )
{
    FILE * fp = fopen(TCARD_FILE_NAME, "w");
    if( NULL == fp ) {
        ERRMSG("open '%s'(rw) fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        return -1;
    }
    
    if( fwrite(TCARD_TEST_CONTENT, 1, sizeof(TCARD_TEST_CONTENT), fp) != sizeof(TCARD_TEST_CONTENT) ) {
        ERRMSG("write '%s' fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        fclose(fp);
        return -2;
    }
    fclose(fp);
    
    fp = fopen(TCARD_FILE_NAME, "r");
    if( NULL == fp ) {
        ERRMSG("open '%s'(ronly) fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        return -3;
    }
    
    AT_ASSERT( sizeof(TCARD_TEST_CONTENT) < 128 );
    
    char buf[128];
    
    if( fread(buf, 1, sizeof(TCARD_TEST_CONTENT), fp) != sizeof(TCARD_TEST_CONTENT) ) {
        ERRMSG("read '%s' fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        fclose(fp);
        return -4;
    }
    fclose(fp);
    
    unlink(TCARD_FILE_NAME);
    
    if( strncmp(buf, TCARD_TEST_CONTENT, sizeof(TCARD_TEST_CONTENT) - 1) ) {
        ERRMSG("read = %s, dst = %s\n", buf, TCARD_TEST_CONTENT);
        return -5;
    }
    
    INFMSG("TFlash Card rw OK.\n");
    return 0;
}

//------------------------------------------------------------------------------
int tcardClose( void )
{
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
