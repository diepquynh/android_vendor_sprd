package com.sprd.appbackup.service;

import com.sprd.appbackup.service.IAppBackupRestoreObserver;
import com.sprd.appbackup.service.IScanAgentAndArchiveListener;
import com.sprd.appbackup.service.Archive;
import com.sprd.appbackup.service.Agent;
import com.sprd.appbackup.service.Category;
import com.sprd.appbackup.service.Account;

interface IAppBackupManager {
    int requestBackup(in Archive archive, in Agent agent, IAppBackupRestoreObserver observer, int code, in List<Account> account);
    int requestRestore(in Archive archive, in Agent agent, IAppBackupRestoreObserver observer, int code);
    int requestCancel(in Agent agent, int code);
    int requestDeduplicate(in Agent agent, IAppBackupRestoreObserver observer, int code);
    String getBackupInfo(in Archive archive, in Agent agent);
    Agent[] getAgents();
    boolean isEnabled(in Agent agent, int code);
    Category[] getCategory(in Agent agent);
    List<Account> getAccounts(in Agent agent, int code);
    void setScanCompleteListener(IScanAgentAndArchiveListener listener);
}
