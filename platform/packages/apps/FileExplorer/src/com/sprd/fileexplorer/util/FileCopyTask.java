package com.sprd.fileexplorer.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileExploreActivity;
import com.sprd.fileexplorer.activities.FilePasteActivity;
import com.sprd.fileexplorer.drmplugin.FileCopyTaskUtils;

public class FileCopyTask extends FileUtilTask {

    private static final String TAG = "FileCopyTask";

    private static final int COPY_BUF_SIZE = 4 * 1024;
    private static final long FOLDER_SIZE = 4 * 1024;

    private static final int CONFLICT_OP_MERGED = 0;
    private static final int CONFLICT_OP_REPLACE = 1;
    private static final int CONFLICT_OP_SAVE_BOTH = 2;

    public boolean mIsCut;
    private File mDestFolder;
    private long mHasCopyedSize;
    private int mConflictOpType;
    private List<String> mScanFiles = new ArrayList<String>();
    private volatile boolean mFileOpCancel;
    private int mFileOpCancelCount = 0;
    private int mSelectedSingleChoiceItems = 0;
    private File mConflictFile;
    private AlertDialog mCopyWarnDialog = null;
    private String numberFormatConfig;
    private boolean isCalculateDialogShow = false;

    public Map<File, List<File>> opFilesMap;
    public long srcFilesSize = 0;
    /* SPRD 457938 @{ */
    // update 3gpp,3gp,mp4 audio file mime type
    private static List<FileInfo> mFileInfoList = new ArrayList<FileInfo>();
    private static final int MAX_CHECK_COUNT = 100;
    /* @} */
    private FileCopyTask(Context context, List<File> operateFiles, File destFolder, boolean isCut) {
        mContext = context;
        mOperateFiles.addAll(operateFiles);
        mDestFolder = destFolder;
        mIsCut = isCut;
        mMainHandler = new Handler(context.getMainLooper());
        // SPRD 457938
        mFileInfoList.clear();
    }

    @Override
    public void start() {
        mWorkingProgress = new ProgressDialog(mContext);
        mWorkingProgress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mWorkingProgress.setTitle(R.string.now_calculate);
        mWorkingProgress.setCanceledOnTouchOutside(false);
        mWorkingProgress.setCancelable(false);
        mWorkingProgress.setButton(DialogInterface.BUTTON_NEGATIVE, 
                mContext.getResources().getString(android.R.string.cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog,
                            int whichButton) {
                        synchronized(mLock){
                            mCancelTask = true;
                        }
                        /* SPRD:427535,click the Button "cancel",call finish. {@*/
                        dialog.dismiss();
                        finish();
                        /* @} */
                    }
                });
        mWorkingProgress.show();
        isCalculateDialogShow = true;
        mScl = new DefaultSCL();
        mScl.affectedFiles.add(mDestFolder);
        mScl.affectedFiles.addAll(mOperateFiles);
        StorageUtil.addStorageChangeListener(mScl);
        new Thread(new Runnable() {
            @Override
            public void run() {
                doInBackground();
                finish();
            }
        }).start();
    }

    @Override
    protected void doInBackground() {
        Log.d(TAG, "start doInBackground()");
        opFilesMap = new HashMap<File, List<File>>();
        List<File> allSrcFolders = new ArrayList<File>();
        String externalStroageStr = StorageUtil.getExternalStorage().getAbsolutePath();
        /* SPRD: Add for bug466914. {@ */
        String usbStroageStr = StorageUtil.getUSBStorage().getAbsolutePath();
        boolean isUSB = mDestFolder.getAbsolutePath().startsWith(usbStroageStr);
        /* @} */
        boolean isSdcard = mDestFolder.getAbsolutePath().startsWith(externalStroageStr);
        for (File file : mOperateFiles) {
            if (mCancelTask) {
                return;
            }
            long tmplong = 0;
            List<File> tmp = FileUtil.getAllFiles(file, true);

            for (File f : tmp) {
                tmplong += f.length();
                if (mIsCut && f.isDirectory()) {
                    allSrcFolders.add(f);
                }
            }
            srcFilesSize += tmplong;
            opFilesMap.put(file, tmp);
        }
        //deal with when copy include drm file
        if(!mIsCut){
            FileCopyTaskUtils.getInstance().includeDRMFileCopy(this);
        }
        /* SPRD: Modify for bug466914. {@ */
        //long restSpace = isSdcard ? StorageUtil.getExternalStorage()
                //.getFreeSpace() : StorageUtil.getInternalStorage().getUsableSpace();
        long restSpace;
        if (isSdcard) {
            restSpace = StorageUtil.getExternalStorage().getFreeSpace();
        } else if (isUSB) {
            restSpace = StorageUtil.getUSBStorage().getFreeSpace();
        } else {
            restSpace = StorageUtil.getInternalStorage().getUsableSpace();
        }
        /* @} */
        // Modify for bug467146.
        if (srcFilesSize > restSpace || restSpace == 0) {
            mCancelTask = true;
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWorkingProgress != null && mWorkingProgress.isShowing()) {
                        mWorkingProgress.dismiss();
                    }
                    Toast.makeText(mContext, R.string.no_enough_sapce, Toast.LENGTH_LONG).show();
                }
            });
            return;
        }

        InnerEntry entry = getFitSizeEntry(srcFilesSize);
        final int maxSize = entry.value;
        final String numberFormat = "%d" + entry.unit + "/%d" + entry.unit;
        numberFormatConfig = numberFormat;
        mMainHandler.post(new Runnable() {

            @Override
            public void run() {
                isCalculateDialogShow = false;
                mWorkingProgress.setProgressNumberFormat(numberFormat);
                mWorkingProgress.setMax(maxSize);
                mWorkingProgress.setProgress(0);
                mWorkingProgress.setTitle(mIsCut ? R.string.now_moving : R.string.now_copying);
            }
        });
        for(Entry<File, List<File>> e : opFilesMap.entrySet()) {
            if(mCancelTask) {
                return;
            }
            File srcFile = e.getKey();
            List<File> allFile = e.getValue();
            final boolean isDirCopy = srcFile.isDirectory();
            File toFile = new File(mDestFolder, srcFile.getName());
            boolean exist = toFile.exists();
            mFileOpCancel = false;
            boolean backup = mDestFolder.equals(srcFile.getParentFile());
            boolean same = (mDestFolder + "/" + srcFile.getName()).toString().equals(srcFile.getAbsoluteFile().toString());
            if(exist) {
                if(isDirCopy && backup) {
                    mConflictOpType = CONFLICT_OP_SAVE_BOTH;
                }else if (mIsCut && same){
                    mConflictOpType = CONFLICT_OP_MERGED;
                    continue;
                }else {
                    //in this situation copy file have comflict, should display dialog for user choose
                    dealConflictFile(srcFile);
                }
                switch(mConflictOpType) {
                    case CONFLICT_OP_MERGED:
                        //nothing to do
                        break;
                    case CONFLICT_OP_REPLACE:
                        if(mFileOpCancel || same) {
                            continue;
                        }else{
                            erase(toFile);
                            break;
                        }
                    case CONFLICT_OP_SAVE_BOTH: {
                        if(mFileOpCancel) {
                            continue;
                        }
                        toFile = makeRenameFile(toFile);
                        if(isDirCopy) {
                            mFileOpCancel = !mkdir(toFile);
                        } else {
                            copyFileOnly(srcFile, toFile);
                            mFileOpCancel = true;
                        }
                        allFile.remove(srcFile);
                        break;
                    }
                }
            }
            if(mFileOpCancel) {
                continue;
            }
            int startIndex = srcFile.getAbsolutePath().length();
            Collections.sort(allFile);
            for(File f: allFile) {
                if(mCancelTask) {
                    return;
                }
                File newFile = new File(toFile, f.getAbsolutePath().substring(startIndex));
                if(f.isDirectory()) {
                    mkdir(newFile);
                } else {
                    copyFileOnly(f, newFile);
                }
            }
        }
        if(mIsCut && !allSrcFolders.isEmpty()) {
            Collections.sort(allSrcFolders);
            for(int i = allSrcFolders.size()-1; i >= 0; i--) {
                allSrcFolders.get(i).delete();
            }
        }
    }

    private InnerEntry mEntry = null;
    private InnerEntry mCacheEntry = new InnerEntry();

    private InnerEntry getFitSizeEntry(long length) {
        if(mEntry == null) {
            FitSizeExpression fse = new FitSizeExpression(length);
            double value = fse.getValue();
            String unit = fse.getUnit();
            int grade = fse.getGrade();
            if(value < 10 && grade > 0) {
                grade--;
                value *= FitSizeExpression.UNIT_SIZE;
                unit = FitSizeExpression.UNIT[grade].unit;
            }
            mEntry = new InnerEntry();
            mEntry.value = (int) Math.round(value);
            if(mEntry.value < 1) {
                mEntry.value = 1;
            }
            mEntry.unit = unit;
            mEntry.maxSize = grade == 0 ? 1 : (long) FitSizeExpression.UNIT[grade-1].maxValue;
            mCacheEntry.value = mEntry.value;
        } else {
            mCacheEntry.value = Math.round(length / mEntry.maxSize);
        }
        mCacheEntry.unit = mEntry.unit;
        return mCacheEntry;
    }

    class InnerEntry {

        public InnerEntry() {
            super();
        }

        long maxSize;
        int value;
        String unit;
    }

    private boolean mkdir(File dir) {
        boolean ret = true;
        if(!dir.exists()) {
            ret = dir.mkdir();
            if(ret) {
                mScanFiles.add(dir.getAbsolutePath());
            }
        }
        mHasCopyedSize += FOLDER_SIZE;
        return ret;
    }

    /**
     * create XXXX(1).XX type file
     * @param file
     * @return XXXX(1).XX type file
     */
    private File makeRenameFile(File file) {
        String name = file.getName();
        String left, right = ")";
        int index = name.lastIndexOf('.');
        if(file.isDirectory() || index < 0) {
            left = name + "(";
        } else {
            left = name.substring(0, index) + "(";
            right += name.substring(index);
        }
        int i = 1;
        File f;
        StringBuilder sb = new StringBuilder(left);
        int leftLength = left.length();
        do {
            sb.setLength(leftLength);
            sb.append(i++);
            sb.append(right);
            f = new File(mDestFolder, sb.toString());
        } while(f.exists());
        return f;
    }

    /**
     * it only copy file, if args is folder will return false
     * when the operation is cut and copy succeed, it will delete srcFile
     * @param srcFile must be fail
     * @param destFile must be fail, if destFile is not exist will create new empty file and continue copy
     * @return copy succeed return true, copy failed return false
     */
    private boolean copyFileOnly(File srcFile, File destFile) {
        if(srcFile.isDirectory() || destFile.isDirectory()) {
            return false;
        }
        FileInputStream fIn = null;
        FileOutputStream fOut = null;
        boolean isSucceed = false;
        int len = 0;
        try {
            if(!destFile.exists() && !destFile.createNewFile()) {
                return false;
            }
            long curTime, preTime = System.currentTimeMillis();
            fIn = new FileInputStream(srcFile);
            fOut = new FileOutputStream(destFile);
            byte[] buf = new byte[COPY_BUF_SIZE];
            while((len = fIn.read(buf)) != -1 && !mCancelTask) {
                fOut.write(buf, 0, len);
                mHasCopyedSize += len;
                curTime = System.currentTimeMillis();
                if((curTime - preTime) > 1000) {
                    updateProcess();
                    preTime = curTime;
                }
            }
            fOut.flush();
            updateProcess();
        } catch (IOException e) {
            Log.e(TAG, "copy file from " + srcFile + " to " + destFile + " have error", e);
            mCancelTask = true;
            isSucceed = false;
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(mContext, R.string.failed_option, Toast.LENGTH_SHORT).show();
                }

            });
            if(!(srcFile.exists() && destFile.exists())) {
                mScl.dealErrorFile();
            }
        } 
        finally {
            try {
                if(fIn != null)
                    fIn.close();
                if(fOut != null)
                    fOut.close();
            } catch (IOException e) {
                Log.w(TAG, "close file stream have exception", e);
            }
        }
        if(len == -1){
            isSucceed = true;
            mScanFiles.add(destFile.getAbsolutePath());
            // SPRD 457938
            addFileToList(srcFile,destFile);
        }
        if(mIsCut && isSucceed) {
            if(srcFile.delete()) {
                try {
                    mContext.getContentResolver().delete(MediaStore.Files.getContentUri("external"),
                            MediaStore.Files.FileColumns.DATA + "=?", new String[] { srcFile.getAbsolutePath() });
                } catch (Exception e) {
                    Log.d(TAG, "copyFileOnly(): Query database error!");
                    e.printStackTrace();
                }
            }
        }
        if(mCancelTask && !isSucceed) {
            destFile.delete();
        }
        return isSucceed;
    }

    private void updateProcess() {
        InnerEntry entry = getFitSizeEntry(mHasCopyedSize);
        final int value = entry.value;
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mWorkingProgress != null && mWorkingProgress.isShowing()) {
                    mWorkingProgress.setProgress(value);
                }
            }
        });
    }

    private void dealConflictFile(final File conflictFile) {
        mConflictFile = conflictFile;
        mMainHandler.post(new Runnable() {

            @Override
            public void run() {
                showCopyWarnDialog(conflictFile);
            }

        });
        synchronized (mContext) {
            try {
                mContext.wait();
            } catch (InterruptedException e) {
                Log.d(TAG, "thread wait have exception", e);
            }
        }
    }

    private void showCopyWarnDialog(File conflictFile) {
        final boolean isDir = conflictFile.isDirectory();
        mConflictOpType = isDir ? 0 : 1;
        String[] items = mContext.getResources().getStringArray(
                isDir ? R.array.copydirWarning : R.array.copyfileWarning);
        if(!((Activity)mContext).isFinishing()){
            mCopyWarnDialog = new AlertDialog.Builder(mContext)
            .setTitle(mContext.getResources().getString(R.string.exist, conflictFile.getName()))
            .setCancelable(false)
            .setSingleChoiceItems(items, mSelectedSingleChoiceItems, new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mSelectedSingleChoiceItems = which;
                    mConflictOpType = isDir ? which : which+1;
               }
            })
            .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mConflictOpType = isDir ? mSelectedSingleChoiceItems : mSelectedSingleChoiceItems+1;
                    synchronized (mContext) {
                        mContext.notify();
                    }
                }
            })
            .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mFileOpCancel = true;
                    mFileOpCancelCount++;
                    synchronized (mContext) {
                        mContext.notify();
                    }
                }
            })
            .show();
        }
    }

    @Override
    protected void finish() {
        updateProcess();
        scanFiles();
        if(!mCancelTask && mOperateFiles.size() == mFileOpCancelCount) {
            mCancelTask = true;
            mMainHandler.post(new Runnable() {

                @Override
                public void run() {
                    mWorkingProgress.dismiss();
                }
            });
        }


        if(mOnFinishCallBack != null) {
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    // SPRD: Modify for bug465956.
                    mOnFinishCallBack.onFinish(mCancelTask, mDestFolder.getAbsolutePath());
                    /* SPRD: Modify for bug510331, give the hint for drm files. @{ */
                    if (!mIsCut) {
                        FileCopyTaskUtils.getInstance().copyDRMFileHint(mContext,FileCopyTask.this);
                    }
                    /* @} */
                }
                
            });
        }
        /* SPRD: Delete for bug465956. @{ */
        /*if(!mCancelTask) {
            Runnable run = new Runnable() {

                @Override
                public void run() {
                  //include drm file when copy,it will give a hint
                    if(FileCopyTaskUtils.getInstance().copyDRMFileHint(mContext,FileCopyTask.this)){
                    }else{
                        Toast.makeText(mContext, R.string.delete_finish, Toast.LENGTH_SHORT).show();
                    }
                    try {
                        if( !((Activity)mContext).isFinishing() && mWorkingProgress != null && mWorkingProgress.isShowing()) {
                            mWorkingProgress.dismiss();
                        }
                    }catch(Exception e){
                        Log.w(TAG,"dialog attached activity restart");
                    }
                    mWorkingProgress = null;
                    Intent intent = new Intent(mContext, FileExploreActivity.class);
                    intent.putExtra(FilePasteActivity.PASTE_DEST_PATH, mDestFolder.getAbsolutePath());
                    mContext.startActivity(intent);
                }

            };
            mScl.addNeedRemovePost(run);
            mMainHandler.postDelayed(run, FileUtil.FILT_OP_COMPLETE_WAIT_TIME);
        }*/
        /* @} */
        StorageUtil.removeStorageChangeListener(mScl);
    }

    private void scanFiles() {
        if(mScanFiles.isEmpty()) {
            return;
        }
        Context appContext = mContext.getApplicationContext();
        int scanSize = mScanFiles.size();
        int arraySize = scanSize < 512 ? scanSize : 512;
        String[] array = new String[arraySize];
        int times = scanSize / arraySize + (scanSize % arraySize == 0 ? 0 : 1);
        if(times > 1) {
            int mod = scanSize % arraySize;
            int limit = mod == 0 ? times : times - 1;
            for(int i = 0; i < limit; i++) {
                for(int j = 0; j < arraySize; j++) {
                    array[j] = mScanFiles.get(i * arraySize + j);
                }
                android.media.MediaScannerConnection.scanFile(appContext, array, null, null);
            }
            if(mod != 0) {
                array = new String[mod];
                int index = limit * arraySize;
                for(int i = 0; i < mod; i++) {
                    array[i] = mScanFiles.get(index + i);
                }
                android.media.MediaScannerConnection.scanFile(appContext, array, null, null);
            }
        } else {
            for(int i = 0; i < arraySize; i++) {
                array[i] = mScanFiles.get(i);
            }
            android.media.MediaScannerConnection.scanFile(appContext, array, null, null);
        }
        /* SPRD 457938 @{ */
        if(mFileInfoList.size() != 0){
            for(int i=0;i<mFileInfoList.size();i++){
                mMainHandler.post(new MediaStoreUpdate(appContext,mFileInfoList.get(i)));
            }
        }
        /* @} */
    }
    /* SPRD 457938 @{ */
    private void refresh(Context context,FileInfo fileinfo){
        ContentValues value = new ContentValues();
        value.put(MediaStore.Files.FileColumns.MIME_TYPE, fileinfo.getMimetype());
        try {
            context.getContentResolver().update(
                    MediaStore.Files.getContentUri("external"),
                    value,
                    MediaStore.Audio.AudioColumns.DATA + "=?",
                    new String[] {fileinfo.getPath()});
        } catch (Exception e) {
            Log.d(TAG, "refresh(): Query database error!");
            e.printStackTrace();
        }
    }

    private class MediaStoreUpdate implements Runnable {
        private FileInfo mFileInfo;
        private int mCheckCount = 0;
        private Context mContext;
        public MediaStoreUpdate(Context context,FileInfo fileinfo){
            mFileInfo=fileinfo;
            mCheckCount = 0;
            mContext = context;
        }

        @Override
        public void run() {
            mCheckCount++;
            if(isFileExsit(mFileInfo.getPath())){
                refresh(mContext,mFileInfo);
            }else{
                if (mCheckCount < MAX_CHECK_COUNT) {
                    mMainHandler.postDelayed(this, 100);
                }
            }
        }
    };
    /* @} */
    private void erase(File file) {
        List<File> allFile = FileUtil.getAllFiles(file, true);
        if(allFile.isEmpty() || mCancelTask) {
            return;
        }
        Collections.sort(allFile);
        for(int i = allFile.size()-1; i >= 0; i--) {
            if(mCancelTask) {
                break;
            }
            File f = allFile.get(i);
            if(f.delete()) {
                try {
                    mContext.getContentResolver().delete(MediaStore.Files.getContentUri("external"),
                            MediaStore.Files.FileColumns.DATA + "=?", new String[] { f.getAbsolutePath() });
                } catch (Exception e) {
                    Log.d(TAG, "erase(): Query database error!");
                    e.printStackTrace();
                }
            }
        }
    }

    public static class Build extends InnerBuild {

        private boolean isCut;

        private File destFolder;

        public Build(Context context, boolean isCut) {
            this.context = context;
            this.isCut = isCut;
        }

        @Override
        public Build addOperateFile(File file) {
            operateFiles.add(file);
            return this;
        }

        @Override
        public Build addOperateFiles(List<File> files) {
            operateFiles.addAll(files);
            return this;
        }

        public Build setDestFolder(File folder) {
            if(folder.isDirectory()) {
                destFolder = folder;
                return this;
            } else {
                throw new IllegalArgumentException("file: " + folder + " is not a directory, " +
                        "create CoptFileTask must have a destFolder");
            }
        }

        public Build setOnFinishCallBack(OnFinishCallBack callback) {
            onFinishCallBack = callback;
            return this;
        }

        @Override
        public FileCopyTask creatTask() {
            Log.d(TAG, "create FileCopyTask, from: " + context.getClass().getName() + " operateFiles: " + operateFiles
                    + " destFolder: " + destFolder + " isCut: " + isCut);
            FileCopyTask ret = new FileCopyTask(context, operateFiles, destFolder, isCut);
            ret.mOnFinishCallBack = onFinishCallBack;
            return ret;
        }

    }
    /* sprd: fix fc issue progressdialog still exist as activity has been destroy @{ */
    public void stop(){
        Log.d(TAG, "stop FileCopyTask mCancelTask =" + mCancelTask);
        if(mWorkingProgress != null && mWorkingProgress.isShowing()){
            Log.d(TAG, "Dismiss working progress dialog");
            mWorkingProgress.dismiss();
            mWorkingProgress = null;
        }
        mCancelTask = true;
    }
    /* @} */

    public void onConfigChanged(){
        if(mWorkingProgress != null && mWorkingProgress.isShowing()){
            int maxSize = mWorkingProgress.getMax();
            int progress = mWorkingProgress.getProgress();
            mWorkingProgress.dismiss();
            mWorkingProgress = new ProgressDialog(mContext);
            mWorkingProgress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            mWorkingProgress.setCanceledOnTouchOutside(false);
            mWorkingProgress.setCancelable(false);
            mWorkingProgress.setButton(DialogInterface.BUTTON_NEGATIVE, 
                    mContext.getResources().getString(android.R.string.cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog,
                                int whichButton) {
                            synchronized(mLock){
                                mCancelTask = true;
                            }
                            dialog.dismiss();
                        }
                    });
            /* SPRD: add for bug 348893 refresh mCopyWarnDialog when ConfigurationChanged @{ */
            mWorkingProgress.setMax(maxSize);
            mWorkingProgress.setProgress(progress);
            if (isCalculateDialogShow) {
                mWorkingProgress.setTitle(R.string.now_calculate);
            } else {
                mWorkingProgress.setProgressNumberFormat(numberFormatConfig);
                mWorkingProgress.setTitle(mIsCut ? R.string.now_moving
                        : R.string.now_copying);
            }
            mWorkingProgress.show();
            /* @} */
        }
        /* SPRD: add for bug 347950 refresh mCopyWarnDialog when ConfigurationChanged @{ */
        if (mCopyWarnDialog != null && mCopyWarnDialog.isShowing()) {
            mCopyWarnDialog.dismiss();
            showCopyWarnDialog(mConflictFile);
        }
        /* @} */
    }
    /* SPRD 457938 @{ */
    private class FileInfo {
        private String mPath;
        private String mMimetype;

        public FileInfo(String path, String mimetype) {
            mPath = path;
            mMimetype = mimetype;
        }

        public String getPath(){
            return mPath;
        }

        public String getMimetype(){
            return mMimetype;
        }
    }

    private boolean isFileExsit(String path) {
        String mimetype = "";
        Cursor cursor = null;
        boolean status = false;
        try{
            cursor = mContext.getContentResolver().query(
                    MediaStore.Files.getContentUri("external"),
                    new String[] { "mime_type" },
                    MediaStore.Files.FileColumns.DATA + "=?",
                    new String[] { path }, null);
            if (cursor != null && cursor.moveToFirst() && cursor.getCount() == 1) {
                status = true;
            }
        }catch(Exception ex){

        }finally{
            if (cursor != null) {
                cursor.close();
            }
        }
        return status;
    }

    private String getMimeTypeFromDB(File file) {
        String mimetype = "";
        Cursor cursor = null;
        try{
            cursor = mContext.getContentResolver().query(
                    MediaStore.Files.getContentUri("external"),
                    new String[] { "mime_type" },
                    MediaStore.Files.FileColumns.DATA + "=?",
                    new String[] { file.getAbsolutePath() }, null);
            if (cursor != null && cursor.moveToFirst() && cursor.getCount() == 1) {
                mimetype = cursor.getString(0);
            }
        }catch(Exception ex){

        }finally{
            if (cursor != null) {
                cursor.close();
            }
        }
        return mimetype;
    }

    private void addFileToList(File srcfile,File destFile){
        String filename = destFile.getName();
        String mimetype = getMimeTypeFromDB(srcfile);
        if ((filename.endsWith(".3gpp") || filename.endsWith(".3gp") || filename
                .endsWith(".mp4")) && mimetype.startsWith("audio/") && filename.startsWith(".")) {
            mFileInfoList.add(new FileInfo(destFile.getAbsolutePath(), mimetype));
        }
    }
    /* @} */
}
