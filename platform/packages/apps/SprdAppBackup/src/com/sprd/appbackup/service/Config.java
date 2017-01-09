package com.sprd.appbackup.service;

import static com.sprd.appbackup.utils.StorageUtil.*;
import android.os.SystemProperties;

public class Config {
    //0 default externl,1 is internal storage, 2 is user define path
    public static int USE_STORAGE = 0;
    public static final int USE_EXTERNAL_STORAGE = 0;
    public static final int USE_INTERNAL_STORAGE = 1;
    public static final int USE_DEFINED_PATH = 2;
    public static boolean IS_SELECT_EXTERNAL_STORAGE = true;
    public static final boolean IS_NAND = "1".equals(SystemProperties.get("ro.device.support.nand"));

    public static final String EXTERNAL_STORAGE_PATH = getExternalStorage().getAbsolutePath();
    public static final String EXTERNAL_ARCHIVE_ROOT_PATH = EXTERNAL_STORAGE_PATH+"/backup/Data/";
    public static final String EXTERNAL_APP_BACKUP_PATH = EXTERNAL_STORAGE_PATH+"/backup/App/";

    public static final String OLD_VERSION_DATA_PATH_EXTERNAL = EXTERNAL_STORAGE_PATH + "/.backup/";

    public static final String INTERNAL_STORAGE_PATH = getInternalStorage().getAbsolutePath();
    public static final String INTERNAL_ARCHIVE_ROOT_PATH = INTERNAL_STORAGE_PATH+"/backup/Data/";
    public static final String INTERNAL_APP_BACKUP_PATH = INTERNAL_STORAGE_PATH+"/backup/App/";

    public static final String OLD_VERSION_DATA_PATH_INTERNAL = INTERNAL_STORAGE_PATH + "/.backup/";

//    public static final String[] ARCHIVE_ROOT_PATH_ARRAY = {INTERNAL_ARCHIVE_ROOT_PATH, EXTERNAL_ARCHIVE_ROOT_PATH};
//    public static final String[] APP_BACKUP_PATH_ARRAY = {INTERNAL_APP_BACKUP_PATH, EXTERNAL_APP_BACKUP_PATH};

    public static final String SUFFIX_APK = ".apk";

    /* SPRD: 445202 save defined backup path @{ */
    public static final String SHAREPREFERENCE_FILE_NAME = "backup_restore";
    public static final String DEFINED_BACKUP_PATH_KEY = "defined_backup_path";
    /* @} */
}
