package com.sprd.gallery3d.appbackup;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import android.app.Service;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.provider.MediaStore.Images;
import android.util.Log;

import com.sprd.appbackup.service.AbstractAppBackupAgent;
import com.sprd.appbackup.service.IAppBackupRepository;
import com.sprd.appbackup.service.IAppBackupRestoreObserver;

public class GalleryBackupService extends Service {
    private static final String TAG = "GalleryBackupService";
    private static final boolean DEB = false;

    private static final int BACKUP_MODE_BACKUP = 1;
    private static final int BACKUP_MODE_RESTORE = 2;
    private static final int FLAG_DUPLICATION_UNSUPPORT = 4;

    private static final String GALLERY_BACKUP_ZIP = "Gallery.zip";
    private static final String[] DATA_PARTITION_IMAGES_PROJECTION = new String[] { Images.ImageColumns.DATA, };

    private int mCurrentCount = 0;
    private int mTotalCount = 0;
    private HandlerThread mWorkThread;
    private static Handler sMainThreadHandler;
    private static Handler sWorkHandler;
    //private volatile boolean mInterrupted;
    private static ArrayList<BackupServiceExecuter> sExecuters;

    private class BackupServiceExecuter implements Runnable {
        private int mWorkType;
        private IAppBackupRepository mRepo;
        private IAppBackupRestoreObserver mObserver;
        private volatile boolean mInterrupted = false;
        private int mCurrentCount;
        private int mTotalCount;

        BackupServiceExecuter(int type, IAppBackupRepository repo,
                IAppBackupRestoreObserver observer) {
            mWorkType = type;
            mRepo = repo;
            mObserver = observer;
            int mTotalCount;
        }

        private void notifyResult(boolean success) {
            notifyResult(success, false);
        }

        private final void notifyResult(boolean success, boolean withUnit) {
            if (mObserver == null) {
                throw new NullPointerException("Check why the observer is null.");
            }
            try {
                if (success) {
                    Log.d(TAG, "Action success!");
                    if (withUnit) {
                        mObserver.onUpdateWithUnit(mTotalCount, mTotalCount, "KB");
                    } else {
                        mObserver.onUpdate(mTotalCount, mTotalCount);
                    }
                    mObserver.onResult(0);
                } else {
                    Log.d(TAG, "Action failed!");
                    mObserver.onUpdate(-1, -1);
                    mObserver.onResult(1);
                }
            } catch (RemoteException rex) {
                Log.e(TAG, "Can't notify result because RemoteException");
            }
        }

        @Override
        public void run() {
            Log.d(TAG, "BackupServiceExecuter start running");
            switch (mWorkType) {
            case BACKUP_MODE_BACKUP:
                doBackup();
            break;
            case BACKUP_MODE_RESTORE:
                doRestore();
            break;
            default:
                Log.w(TAG, "Unknown type " + mWorkType);
            }
            if (sMainThreadHandler != null) {
                sMainThreadHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (sExecuters != null) {
                            sExecuters.remove(BackupServiceExecuter.this);
                        }
                    }
                });
            } else {
                Log.e(TAG, "sMainThreadHandler is unexpectedly null.");
            }
        }

        private void doBackup() {
            if (mInterrupted) {
                Log.d(TAG, "mInterrupted is true, canceling this operation.");
                return;
            } else {
                Log.d(TAG, "start backup now");
            }
            /* SPRD:Add for bug599975 Catch the exception and handle with it @{ */
            Cursor c = null;
            try {
                c = getContentResolver().query(
                        Images.Media.EXTERNAL_CONTENT_URI,
                        DATA_PARTITION_IMAGES_PROJECTION, null, null, null);
            } catch (SecurityException se) {
                se.printStackTrace();
            }
            /* Bug599975 end @} */
            if (c != null && c.getCount() > 0) {
                mTotalCount = c.getCount();
            } else {
                Log.e(TAG, "Query failed, stop backup");
                notifyResult(false);
                if (c != null) {
                    c.close();
                    c = null;
                }
                return;
            }
            String filePath = "";
            OutputStream out = null;
            ZipOutputStream zos = null;

            ParcelFileDescriptor fd = null;

            try {
                fd = mRepo.write(GALLERY_BACKUP_ZIP);
                if (fd == null) {
                    Log.e(TAG, "write gallery backup zipfile failed, notifyresult");
                    notifyResult(false);
                    return;
                }
            } catch (RemoteException e) {
                Log.e(TAG, "repo write failed! " + e.getMessage());
                e.printStackTrace();
                notifyResult(false);
                return;
            }

            try {
                out = new ParcelFileDescriptor.AutoCloseOutputStream(fd);
             } catch (Exception e) {
                Log.e(TAG, "Maybe the freespace is too low, created failed", e.getCause());
                notifyResult(false);
             }
            zos = new ZipOutputStream(out);
            mCurrentCount = 0;
            while (c.moveToNext() && !mInterrupted) {
                filePath = c.getString(0);
                FileUtil.writeToZip(new File(filePath), zos);
                try {
                    mObserver.onUpdate(++mCurrentCount, mTotalCount);
                } catch (RemoteException e) {
                    Log.e(TAG, "Can't update progress");
                    e.printStackTrace();
                }
            }
            // Mark the backup service has done.
            notifyResult(true);
            c.close();
            c = null;
            try {
                zos.close();
            } catch (IOException e1) {
                Log.e(TAG,
                        "Close zip input stream failed! " + e1.getMessage());

                e1.printStackTrace();
            }
        }

        private void doRestore() {
            if (mInterrupted) {
                Log.d(TAG, "mInterrupted is true, canceling this operation.");
                return ;
            } else {
                Log.d(TAG, "start backup now");
            }

            ParcelFileDescriptor fd = null;
            try {
                fd = mRepo.read(GALLERY_BACKUP_ZIP);
                if (fd == null) {
                    Log.e(TAG, "restore failed because fd is null, notifyresult");
                    notifyResult(false);
                    return;
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Can't read the zip file, " + e.getMessage());
                e.printStackTrace();
                notifyResult(false);
                return;
            }

            InputStream input = null;
            try {
                input = new ParcelFileDescriptor.AutoCloseInputStream(
                        fd);
            } catch (Exception e) {
                Log.e(TAG, "Maybe the freespace is too low, created failed", e.getCause());
                notifyResult(false);
                return;
            }
            final long totalSize = fd.getStatSize();
            long currentSize = 0;
            ZipInputStream zis = new ZipInputStream(input);
            ZipEntry entry = null;
            boolean restoreSuccess = true;
            try {
                ArrayList<String> fileList = new ArrayList<String>();
                while (((entry = zis.getNextEntry()) != null) && !mInterrupted) {
                    String fileName = FileUtil.unpackZipEntry(entry, zis);
                    if (fileName.equals(FileUtil.INCOMPLETE_EXCEPTION)) {
                        restoreSuccess = false;
                        break;
                    }
                    if (!fileName.equals(FileUtil.UNPACK_RETURN_EXCEPTION)) {
                        fileList.add(fileName);
                    // SPRD: If return exception when unpack , it need do nofityResult(false) here. @{
                    } else {
                        restoreSuccess = false;
                        break;
                    }
                    // @}
                    Log.d(TAG, "File " + fileName + " restore success!");
                    currentSize += new File(fileName).length();
                    mObserver.onUpdateWithUnit((int) (currentSize / 1024),
                            (int) (totalSize / 1024), "KB");
                    // Log.d(TAG, "currentSize = " + currentSize + "totalSize = " + totalSize);
                    // Log.d(TAG, "mObserver this = " + mObserver);
                }
                if (!fileList.isEmpty()) {
                    MediaScannerConnection.scanFile(
                            GalleryBackupService.this,
                            (String[]) fileList.toArray(new String[] {}),
                            null, null);
                }
                if (!restoreSuccess) {
                    notifyResult(false);
                    return;
                }
            } catch (Exception e) {
                Log.e(TAG, "Catch exceptions, " + e.getMessage());
                e.printStackTrace();
                notifyResult(false);
                return;
            } finally {
                try {
                    zis.closeEntry();
                    zis.close();
                } catch (IOException e) {
                    Log.e(TAG,
                            "ZipInputStream close error!" + e.getMessage());
                    e.printStackTrace();
                }

            }
            notifyResult(true, true);
        }
    }

    private IBinder mBinder = new AbstractAppBackupAgent() {
        @Override
        public int onBackup(IAppBackupRepository repo,
                IAppBackupRestoreObserver observer) {
            Log.d(TAG, "onBackup now, posting backup runnable");
            BackupServiceExecuter executer = new BackupServiceExecuter(
                    BACKUP_MODE_BACKUP, repo, observer);
            if (sWorkHandler != null && sExecuters != null) {
                sWorkHandler.post(executer);
                sExecuters.add(executer);
            }
            return 0;
        }

        @Override
        public int onDeduplicate(IAppBackupRestoreObserver observer,
                int categoryCode) {
            // The gallery backup won't support removing duplicate
            try{
                observer.onResult(FLAG_DUPLICATION_UNSUPPORT);
            } catch (RemoteException rex) {
                Log.e(TAG, "Can't notify result because RemoteException");
            }
            return FLAG_DUPLICATION_UNSUPPORT;
        }

        @Override
        public int onRestore(IAppBackupRepository repo,
                IAppBackupRestoreObserver observer) {
            Log.d(TAG, "onRestore now, posting restore runnable");
            BackupServiceExecuter executer = new BackupServiceExecuter(
                    BACKUP_MODE_RESTORE, repo, observer);
            if (sWorkHandler != null && sExecuters != null) {
                sWorkHandler.post(executer);
                sExecuters.add(executer);
            }
            return 0;
        }

        @Override
        public int onCancel() {
            Log.d(TAG, "onCancel");
            if (sExecuters != null && !sExecuters.isEmpty()) {
                Log.d(TAG, "Caceling executers");
                for (BackupServiceExecuter executer : sExecuters) {
                    executer.mInterrupted = true;
                }
            } else {
                Log.d(TAG, "No executers to cancel");
            }
            return 0;
        }

        @Override
        public String getBackupInfo(IAppBackupRepository repo) {
            return "Greetings! I'm the service of GalleryBackup, nice to meet you.";
        }

        @Override
        public boolean isEnabled() {
            Log.d(TAG, "isEnable is called, ");
            Cursor cursor = getContentResolver().query(
                    Images.Media.EXTERNAL_CONTENT_URI,
                    DATA_PARTITION_IMAGES_PROJECTION, null, null, null);
            Log.d(TAG, "query complete");
            try {
                if (cursor == null || cursor.getCount() <= 0) {
                    return false;
                } else {
                    return true;
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "Bind Gallery backup service.");
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mWorkThread = new HandlerThread("GalleryBackupWorker");
        mWorkThread.start();
        sWorkHandler = new Handler(mWorkThread.getLooper());
        sMainThreadHandler = new Handler();
        sExecuters = new ArrayList<BackupServiceExecuter>();
    }

    @Override
    public void onDestroy() {
        sExecuters.clear();
        sExecuters = null;
        mWorkThread.quit();
        mWorkThread = null;
        sWorkHandler = null;
        sMainThreadHandler = null;
        super.onDestroy();
    }

}

