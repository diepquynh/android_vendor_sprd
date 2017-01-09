package com.sprd.appbackup.service;

interface IAppBackupRestoreObserver {
    void onUpdate(int current, int total);
    void onUpdateWithUnit(int current, int total, String unit);
    void onResult(int resultCode);
}

