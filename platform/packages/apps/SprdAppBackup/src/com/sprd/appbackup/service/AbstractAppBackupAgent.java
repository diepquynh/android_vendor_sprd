package com.sprd.appbackup.service;
import java.util.List;

import android.os.RemoteException;

public abstract class AbstractAppBackupAgent extends IAppBackupAgent.Stub{

    public int onBackup(IAppBackupRepository repo, IAppBackupRestoreObserver observer, int categoryCode){
        return onBackup(repo, observer);
    }

    public int onBackup(IAppBackupRepository repo, IAppBackupRestoreObserver observer, int categoryCode, List<Account> accounts){
        return onBackup(repo, observer, categoryCode);
    }

    public int onBackup(IAppBackupRepository repo, IAppBackupRestoreObserver observer){
        return -1;
    }

    public int onBackup(IAppBackupRepository repo, IAppBackupRestoreObserver observer, List<Account> accounts){
        return onBackup(repo, observer);
    }

    public int onRestore(IAppBackupRepository repo, IAppBackupRestoreObserver observer, int categoryCode){
        return onRestore(repo, observer);
    }

    public int onRestore(IAppBackupRepository repo, IAppBackupRestoreObserver observer){
        return -1;
    }

    public int onCancel(int categoryCode){
        return onCancel();
    }

    public int onCancel(){
        return -1;
    }
    abstract public String getBackupInfo(IAppBackupRepository repo);

    public boolean isEnabled(){
        return false;
    }

    public boolean isEnabled(int categoryCode){
        return isEnabled();
    }

    public Category[] getCategory(){
        return null;
    }
    public List<Account> getAccounts(int code){
        return getAccounts();
    }
    public List<Account> getAccounts(){
        return null;
    }
    public int onDeduplicate(IAppBackupRestoreObserver observer){
        try {
            observer.onResult(0);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return 0;
    }
    public int onDeduplicate(IAppBackupRestoreObserver observer, int categoryCode){
        return onDeduplicate(observer);
    }

}
