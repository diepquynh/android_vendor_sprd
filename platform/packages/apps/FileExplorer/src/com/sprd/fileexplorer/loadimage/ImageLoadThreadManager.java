package com.sprd.fileexplorer.loadimage;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import android.util.Log;


public class ImageLoadThreadManager {

    private static final String TAG = "ImageLoadThreadManager";
    private static final int CORE_POOL_SIZE = 2;
    private static final int MAXI_POOL_SIZE = 128;
    private static final int KEEP_ALIVE = 10;

    private static final ThreadPoolExecutor mExecutor;
    private static final ThreadFactory mThreadFactory;
    private static final PriorityBlockingQueue<Runnable> mWorkQueue;
    private static ConcurrentHashMap<String,ImageLoadTask> mHashMap;
    private static ConcurrentHashMap<String,MediaImageLoadTask> mMediaHashMap;
    private static final Comparator<Runnable> mComparator;

    static {
        mComparator = new TaskComparator();
        mHashMap = new ConcurrentHashMap<String,ImageLoadTask>();
        mMediaHashMap = new ConcurrentHashMap<String, MediaImageLoadTask>();
        mWorkQueue = new PriorityBlockingQueue<Runnable>(15, mComparator);
        mThreadFactory = new DefaultThreadFactory();
        mExecutor = new ThreadPoolExecutor(CORE_POOL_SIZE, MAXI_POOL_SIZE, KEEP_ALIVE,
                TimeUnit.SECONDS, mWorkQueue, mThreadFactory);
    }

    public ImageLoadThreadManager() {
        // TODO Auto-generated constructor stub
    }
    
    public static void removeTask(String fileUrl) {
        mHashMap.remove(fileUrl);
        mMediaHashMap.remove(fileUrl);
    }
    
    public static void updateWorkQueue(ArrayList<String> newTaskUrl) {
        for (ImageLoadTask task: mHashMap.values()) {
            if (!newTaskUrl.contains(task.mImageUrl)) {
                mWorkQueue.remove(task);
                mHashMap.remove(task.mImageUrl);
            }
        }
    }
    
    public static void submitTask(String fileUrl, ImageLoadTask task) {
        Log.e(TAG, "execute a ImageLoadTask ! sWorkQueue.size() = " + mWorkQueue.size());
        if (mHashMap.get(fileUrl) == null) {
            mHashMap.put(fileUrl, task);
            mExecutor.execute(task);
        } else {
            Log.e(TAG, "there is already a task running !");
        }
    }

    public static void submitTask(String fileUrl, MediaImageLoadTask task) {
        Log.d(TAG, "execute a ImageLoadTask ! sWorkQueue.size() = " + mWorkQueue.size());
        if (mMediaHashMap.get(fileUrl) == null) {
            mMediaHashMap.put(fileUrl, task);
            mExecutor.execute(task);
        } else {
            Log.e(TAG, "there is already a task running !");
        }
    }

    static class DefaultThreadFactory implements ThreadFactory {

        private final AtomicInteger mCount;

        DefaultThreadFactory() {
            mCount = new AtomicInteger(1);
        }

        public Thread newThread(Runnable runnable) {

            Log.e(TAG, "New a Thread for  ImageLoadTask:" + mCount.toString());
            return new Thread(runnable, "ImageLoadTask #" + mCount.getAndIncrement());
        }
    }
    
    static class TaskComparator implements Comparator<Runnable> {

        @Override
        public int compare(Runnable runnable1, Runnable runnable2) {
            // TODO Auto-generated method stub
            if (runnable1 instanceof ImageLoadTask
                    && runnable2 instanceof ImageLoadTask) {
                long x = ((ImageLoadTask)runnable1).mPriority;
                long y = ((ImageLoadTask)runnable2).mPriority;
                if (x < y) {
                    return -1;
                } else if (x > y){
                    return 1;
                } else {
                    return 0;
                }
            }
            return 0;
        }
        
    }
}
