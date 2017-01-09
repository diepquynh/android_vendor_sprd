package com.sprd.appbackup.service;

import android.os.ParcelFileDescriptor;

interface IAppBackupRepository {
    ParcelFileDescriptor read(String fileName);
    ParcelFileDescriptor write(String fileName);
    boolean isOldVersionFile();
    String getOldVersionFilePath();
}

