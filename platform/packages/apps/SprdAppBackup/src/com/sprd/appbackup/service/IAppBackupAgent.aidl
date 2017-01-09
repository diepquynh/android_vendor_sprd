package com.sprd.appbackup.service;

import com.sprd.appbackup.service.IAppBackupRepository;
import com.sprd.appbackup.service.IAppBackupRestoreObserver;
import com.sprd.appbackup.service.Category;
import com.sprd.appbackup.service.Account;

interface IAppBackupAgent {
    int onBackup(IAppBackupRepository repo, IAppBackupRestoreObserver observer, 
                 int categoryCode, in List<Account> accounts);
    int onRestore(IAppBackupRepository repo, IAppBackupRestoreObserver observer, int categoryCode);
    int onCancel(int categoryCode);
    String getBackupInfo(IAppBackupRepository repo);
    boolean isEnabled(int categoryCode);
    Category[] getCategory();
    List<Account> getAccounts(int categoryCode);
    int onDeduplicate(IAppBackupRestoreObserver observer, int categoryCode);
}
