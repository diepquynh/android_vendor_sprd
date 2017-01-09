package com.sprd.fileexplorer.util;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.app.ProgressDialog;
import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;

public abstract class FileUtilTask {

    protected volatile boolean mCancelTask = false;
    protected byte[] mLock = new byte[0];
    protected ProgressDialog mWorkingProgress;
    public List<File> mOperateFiles = new ArrayList<File>();;
    protected Context mContext;
    protected DefaultSCL mScl;
    protected OnFinishCallBack mOnFinishCallBack;
    public Handler mMainHandler;
    /**
     * it must be call from ui thread
     */
    public abstract void start();
    /**
     * it called by {@link #start()}
     */
    protected abstract void doInBackground();
    /**
     * it called by {@link #start()} after {@link #doInBackground()}
     */
    protected abstract void finish();
    
    /**
     * DefaultSCL implement default handle with StorageChangedListener, if you want to
     * change the way, you can extends DefaultSCL and Override onStorageChanged()
     * @author xing
     */
    protected class DefaultSCL implements StorageChangedListener {

        protected List<File> affectedFiles = new ArrayList<File>();
        protected List<Runnable> needRemovePost;
        
        public void addNeedRemovePost(Runnable run) {
            if(needRemovePost == null) {
                needRemovePost = new ArrayList<Runnable>();
            }
            needRemovePost.add(run);
        }
        
        public void dealErrorFile() {
            if(mMainHandler == null) {
                return;
            }
            mMainHandler.post(new Runnable() {

                @Override
                public void run() {
                    synchronized(mLock){
                        mCancelTask = true;
                    }
                    try{
                        if( !((Activity)mContext).isFinishing() && mWorkingProgress != null && mWorkingProgress.isShowing()) {
                            mWorkingProgress.dismiss();
                        }
                    }catch(Exception e){
                        Log.w("FileUtilTask","dialog attached activity restart");
                    }
                    mWorkingProgress = null;
                    Toast.makeText(mContext, R.string.sdcard_had_unmounted, Toast.LENGTH_LONG).show();
                }

            });
        }

        // SPRD: Modify for bug509242.
        @Override
        public void onStorageChanged(String path, boolean available, boolean sdcard) {
            if(available || mMainHandler == null)
                return;
            for(File f: affectedFiles) {
                if(f.getAbsolutePath().startsWith(path)) {
                    mMainHandler.post(new Runnable() {

                        @Override
                        public void run() {
                            synchronized(mLock){
                                mCancelTask = true;
                            }
                            if( !((Activity)mContext).isFinishing() &&
                                    mWorkingProgress != null && mWorkingProgress.isShowing()) {
                                mWorkingProgress.dismiss();
                            }
                            if(needRemovePost != null) {
                                for(Runnable r: needRemovePost) {
                                    mMainHandler.removeCallbacks(r);
                                }
                            }
                            Toast.makeText(mContext, R.string.sdcard_had_unmounted, Toast.LENGTH_LONG).show();
                        }

                    });
                    break;
                }
            }
        }
        
    }
    
    public interface OnFinishCallBack {
        // SPRD: Modify for bug465956.
        void onFinish(boolean cancelTask, String path);
    }
    
    protected static abstract class InnerBuild {
        
        List<File> operateFiles = new ArrayList<File>();
        Context context;
        OnFinishCallBack onFinishCallBack;
        
        public abstract InnerBuild addOperateFile(File file);
        public abstract InnerBuild addOperateFiles(List<File> files);
        public abstract FileUtilTask creatTask();

    }
}
