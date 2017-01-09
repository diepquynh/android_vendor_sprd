package com.sprd.fileexplorer.util;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ContentProviderOperation;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.loadimage.ImageCache;

public class FileDeleteTask extends FileUtilTask {

    public static final String TAG = "FileDeleteTask";
    
    private boolean mIsClear;
    private boolean mFailThenFinish = false;
    private List<File> mDeleteFiles = new ArrayList<File>();
    private ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();

    private FileDeleteTask(Context context, List<File> operateFiles, boolean isClear, boolean isFinish) {
        mContext = context;
        mOperateFiles.addAll(operateFiles);
        mIsClear = isClear;
        mMainHandler = new Handler(context.getMainLooper());
        mFailThenFinish = isFinish;
    }
    
    /**
     * it must be call from ui thread
     */
    @Override
    public void start() {
        mWorkingProgress = new ProgressDialog(mContext);
        mWorkingProgress.setCancelable(false);
        mWorkingProgress.setTitle(mIsClear ? R.string.operate_clear : R.string.operate_delete);
        mWorkingProgress.setMessage(mContext.getResources().getString(mIsClear ? R.string.now_cleared : R.string.now_deleting));
        mWorkingProgress.setButton(DialogInterface.BUTTON_NEGATIVE,
                mContext.getResources().getString(android.R.string.cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        synchronized(mLock){
                            mCancelTask = true;
                        }
                        NotifyManager.getInstance().showToast(mContext.getResources().getString(R.string.cancel_copy));
                        dialog.dismiss();
                    }
                });
        mWorkingProgress.show();
        mScl = new DefaultSCL();
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
        for (File file : mOperateFiles) {
            if (mCancelTask) {
                break;
            }
            erase(file);
        }
        try{
            mContext.getContentResolver().applyBatch("media", ops);
            for (File f : mDeleteFiles){
                ImageCache.remove(f.getAbsolutePath().toString());
            }
            mDeleteFiles.clear();
        }catch(Exception e){
            Log.w(TAG,"media db applyBatch delete failed");
            for(File f : mDeleteFiles){
                mContext.getContentResolver().delete(MediaStore.Files.getContentUri("external"),
                MediaStore.Files.FileColumns.DATA + "=?", new String[] { f.getAbsolutePath() });
                ImageCache.remove(f.getAbsolutePath().toString());
            }
            mDeleteFiles.clear();
        }
    }
    
    @Override
    protected void finish() {
        if(mOnFinishCallBack != null) {
            mMainHandler.post(new Runnable() {

                @Override
                public void run() {
                    if (!((Activity) mContext).isFinishing()) {
                        // SPRD: Modify for bug465956.
                        mOnFinishCallBack.onFinish(mCancelTask, null);
                    }
                }
                
            });
        }
        if (!mCancelTask) {
            Runnable run = new Runnable() {

                @Override
                public void run() {
                    Toast.makeText(mContext, R.string.delete_finish, Toast.LENGTH_SHORT).show();
                    if( !((Activity)mContext).isFinishing() && mWorkingProgress != null && mWorkingProgress.isShowing()) {
                        mWorkingProgress.dismiss();
                    }
                    mWorkingProgress = null;
                }
            };
            mScl.addNeedRemovePost(run);
            mMainHandler.postDelayed(run, FileUtil.FILT_OP_COMPLETE_WAIT_TIME);
        }
        StorageUtil.removeStorageChangeListener(mScl);
    }

    private void erase(File file) {
        List<File> allFile = FileUtil.getAllFiles(file, true);
        if(allFile.isEmpty() || mCancelTask) {
            return;
        }
        Collections.sort(allFile);
        if(mIsClear) {
            allFile.remove(0);
        }
        for(int i = allFile.size()-1; i >= 0 && !mCancelTask; i--) {
            File f = allFile.get(i);
            if(f.delete()) {
                ops.add(ContentProviderOperation.newDelete(MediaStore.Files.getContentUri("external"))
                        .withSelection(MediaStore.Files.FileColumns.DATA + "=?", new String[]{f.getAbsolutePath()})
                        .build());
                mDeleteFiles.add(f);
 //               mContext.getContentResolver().delete(MediaStore.Files.getContentUri("external"),
 //                       MediaStore.Files.FileColumns.DATA + "=?", new String[] { f.getAbsolutePath() });
            } else {
                if(f.exists()) {
                    final String name = f.getName();
                    mCancelTask = true;
                    mMainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            NotifyManager.getInstance().showToast(mContext.getResources()
                                    .getString(R.string.delete_fail, name), Toast.LENGTH_LONG);
                            if( !((Activity)mContext).isFinishing() && mWorkingProgress != null && mWorkingProgress.isShowing()) {
                                mWorkingProgress.dismiss();
                            }
                            mWorkingProgress = null;
                            if (mFailThenFinish) {
                                ((Activity)mContext).finish();
                            }
                        }
                    });
                } else {
                    mScl.dealErrorFile();
                }
            }
        }
    }

    public static class Build extends InnerBuild {

        private boolean isClear;
        private boolean isFinish;

        public Build(Context context, boolean isClear, boolean isFinish) {
            this.context = context;
            this.isClear = isClear;
            this.isFinish = isFinish;
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
        
        public Build setOnFinishCallBack(OnFinishCallBack callback) {
            onFinishCallBack = callback;
            return this;
        }
        
        @Override
        public FileDeleteTask creatTask() {
            FileDeleteTask task = new FileDeleteTask(context, operateFiles, isClear, isFinish);
            task.mOnFinishCallBack = onFinishCallBack;
            Log.d(TAG, "create FileDeleteTask, from: " + context.getClass().getName() + " operateFiles: "
             + operateFiles + " isClear: " + isClear);
            return task;
        }

    }

    public void onConfigChanged(){
        // SPRD: add an if construct for bug 394722.
        if(mCancelTask) return;

        if(mWorkingProgress != null){
            mWorkingProgress.dismiss();
        }
        mWorkingProgress = new ProgressDialog(mContext);
        mWorkingProgress.setCancelable(false);
        mWorkingProgress.setTitle(mIsClear ? R.string.operate_clear : R.string.operate_delete);
        mWorkingProgress.setMessage(mContext.getResources().getString(mIsClear ? R.string.now_cleared : R.string.now_deleting));
        mWorkingProgress.setButton(DialogInterface.BUTTON_NEGATIVE,
                mContext.getResources().getString(android.R.string.cancel),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        synchronized(mLock){
                            mCancelTask = true;
                        }
                        NotifyManager.getInstance().showToast(mContext.getResources().getString(R.string.cancel_copy));
                        dialog.dismiss();
                    }
                });
        mWorkingProgress.show();
    }

    public void cancelTask(boolean cancelOrnot) {
        mCancelTask = cancelOrnot;
    }
}
