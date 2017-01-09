/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ucamera.ugallery.gallery;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.OperationApplicationException;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Process;
import android.os.RemoteException;
import android.provider.MediaStore;
import android.util.Log;

import com.ucamera.ugallery.provider.UCamData;
import com.ucamera.ugallery.util.BitmapManager;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * A dedicated decoding thread used by ImageGallery.
 */
public class ImageLoader {
    @SuppressWarnings("unused")
    private static final String TAG = "ImageLoader";

    // Queue of work to do in the worker thread. The work is done in order.
    private final ArrayList<WorkItem> mQueue = new ArrayList<WorkItem>();

    private ArrayList<ContentProviderOperation> mSyncList = new ArrayList<ContentProviderOperation>();
    private Map<String, ContentProviderOperation> mSyncMap = new HashMap<String, ContentProviderOperation>();
    private ArrayList<String> mPathList = new ArrayList<String>();
    private ArrayList<ContentProviderOperation> mChunkList = null;
    private static final int CHUNK_PER_NUM = 100;
    private int mChunkIndex = 0;
    private int mImageCount;

    // the worker thread and a done flag so we know when to exit
    private boolean mDone;

    private Thread mDecodeThread;

    private ContentResolver mCr;

    public interface LoadedCallback {
        /**
         *
         * @param result result
         */
        void run(Bitmap result);
    }

    /**
     * getBitmap
     * @param image image
     * @param imageLoadedRunnable imageLoadedRunnable
     * @param tag tag
     */
    public void getBitmap(IImage image, LoadedCallback imageLoadedRunnable, int tag) {
        if (mDecodeThread == null) {
            start();
        }
        synchronized (mQueue) {
            WorkItem w = new WorkItem(image, imageLoadedRunnable, tag);
            mQueue.add(w);
            mQueue.notifyAll();
        }
    }

    /**
     * cancel
     * @param image image
     * @return boolean
     */
    public boolean cancel(final IImage image) {
        synchronized (mQueue) {
            int index = findItem(image);
            if (index >= 0) {
                mQueue.remove(index);
                return true;
            } else {
                return false;
            }
        }
    }

    // The caller should hold mQueue lock.
    private int findItem(IImage image) {
        for (int i = 0; i < mQueue.size(); i++) {
            if (mQueue.get(i).mImage == image) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Clear the queue. Returns an array of tags that were in the queue.
     * @return int[]
     */
    public int[] clearQueue() {
        synchronized (mQueue) {
            int n = mQueue.size();
            int[] tags = new int[n];
            for (int i = 0; i < n; i++) {
                tags[i] = mQueue.get(i).mTag;
            }
            mQueue.clear();
            return tags;
        }
    }

    private static class WorkItem {
        IImage mImage;

        LoadedCallback mOnLoadedRunnable;

        int mTag;

        WorkItem(IImage image, LoadedCallback onLoadedRunnable, int tag) {
            mImage = image;
            mOnLoadedRunnable = onLoadedRunnable;
            mTag = tag;
        }
    }

    /**
     * ImageLoader
     * @param cr cr
     * @param handler handler
     */
    public ImageLoader(ContentResolver cr, Handler handler) {
        mCr = cr;
        start();
        startDB();
    }

    private class WorkerThread implements Runnable {

        // Pick off items on the queue, one by one, and compute their bitmap.
        // Place the resulting bitmap in the cache, then call back by executing
        // the given runnable so things can get updated appropriately.
        public void run() {
            Process.setThreadPriority(0);
            while (true) {
                WorkItem workItem = null;
                synchronized (mQueue) {
                    if (mDone) {
                        break;
                    }
                    if (!mQueue.isEmpty()) {
                        workItem = mQueue.remove(0);
                    } else {
                        try {
                            mQueue.wait();
                        } catch (InterruptedException ex) {
                            // ignore the exception
                        }
                        continue;
                    }
                }
                final Object[] obj = workItem.mImage.miniThumbBitmap();
                if (workItem.mOnLoadedRunnable != null) {
                    boolean isLoaded = (Boolean) obj[0];
                    Bitmap b = (Bitmap) obj[1];
                    byte[] data = null;
                    if(!isLoaded) {
                        data = transform(b);
                    }
                    workItem.mOnLoadedRunnable.run(b);
                    if(isLoaded == false) {
                        String imagePath = workItem.mImage.getDataPath();
                        if(mPathList.contains(imagePath)) {
//                            continue;
                        }
                        mPathList.add(imagePath);
                        synchronized (mSyncList) {
                            /* FIX BUG: 5937 5933
                             * BUG CAUSE: update thumbnail database only once
                             * FIX COMMENT: make sure update thumbnail database with the new data
                             * DATE: 2014-02-14
                             */
                            if(mSyncMap.containsKey(imagePath)) {
                                mSyncList.remove(mSyncMap.get(imagePath));
                            }
                            ContentProviderOperation operation = ContentProviderOperation.newInsert(UCamData.Thumbnails.CONTENT_URI)
                                    .withValue(UCamData.Thumbnails.THUMB_ID, workItem.mImage.getImageId())
                                    .withValue(UCamData.Thumbnails.THUMB_PATH, imagePath)
                                    .withValue(UCamData.Thumbnails.THUMB, data)
                                    .withValue(UCamData.Thumbnails.THUMB_DATE, workItem.mImage.getDateTaken())
                                    .build();
                            mSyncMap.put(imagePath, operation);
                            mSyncList.add(operation);
//                            mSyncList.add(ContentProviderOperation.newInsert(UCamData.Thumbnails.CONTENT_URI)
//                                    .withValue(UCamData.Thumbnails.THUMB_ID, workItem.mImage.getImageId())
//                                    .withValue(UCamData.Thumbnails.THUMB_PATH, imagePath)
//                                    .withValue(UCamData.Thumbnails.THUMB, data)
//                                    .withValue(UCamData.Thumbnails.THUMB_DATE, workItem.mImage.getDateTaken())
//                                    .build());
                            int size = mSyncList.size();
                            if(size > 0 && (size % CHUNK_PER_NUM == 0)) {
//                                operationData(true);
                                mChunkList = new ArrayList<ContentProviderOperation>(mSyncList.subList(mChunkIndex * CHUNK_PER_NUM, ++mChunkIndex * CHUNK_PER_NUM));
                                mSyncList.notifyAll();
                            } else if(size == mImageCount) {
                                insertDataToDB();
                                mSyncList.notifyAll();
                            }
                        }
                    }
                }
            }
        }
    }

    public void setImageCount(int imageCount) {
        mImageCount = imageCount;
    }

    private void insertDataToDB() {
        try {
            mChunkList = new ArrayList<ContentProviderOperation>(mSyncList.subList(mChunkIndex
                    * CHUNK_PER_NUM, mSyncList.size()));
            if (mChunkList != null && mChunkList.size() > 0) {
                try {
                    mCr.applyBatch(UCamData.AUTHORITY, mChunkList);
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (OperationApplicationException e) {
                    e.printStackTrace();
                }
            }
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "to get subList from mSyncList, the  start index is more than end index;");
        } finally {
            mChunkList.clear();
            mSyncList.clear();
            mSyncMap.clear();
            mPathList.clear();
            mChunkIndex = 0;
        }
    }

    private class InsertThread implements Runnable {
        @Override
        public void run() {
            Process.setThreadPriority(19);
            while (true) {
                synchronized (mSyncList) {
                    if (mDone) {
                        return;
                    }
                    try {
                        while(mChunkList == null || mChunkList.isEmpty()) {
                            mSyncList.wait();
                        }
                        if(mChunkList != null) {
                            mCr.applyBatch(UCamData.AUTHORITY, mChunkList);
                            mChunkList.clear();
                            mSyncMap.clear();
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    } catch (OperationApplicationException e) {
                        e.printStackTrace();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }

    }

    private byte[] transform(Bitmap bitmap) {
        byte [] data = null;
        if(bitmap != null) {
            ByteArrayOutputStream miniOutStream = new ByteArrayOutputStream();
            bitmap.compress(Bitmap.CompressFormat.JPEG, 80, miniOutStream);

            try {
                miniOutStream.close();
                data = miniOutStream.toByteArray();
            } catch (java.io.IOException ex) {
                Log.e(TAG, "got exception ex " + ex);
            }
        }
        return data;
    }

    private void start() {
        if (mDecodeThread != null) {
            return;
        }

        mDone = false;
        Thread t = new Thread(new WorkerThread());
        t.setName("image-loader");
        mDecodeThread = t;
        t.start();
    }

    private Thread mInsertThread;
    private void startDB() {
        if(mInsertThread != null) {
            return;
        }
        Thread t = new Thread(new InsertThread());
        mInsertThread = t;
        t.start();
    }

    /**
     * stop
     */
    public void stop() {
        synchronized (mQueue) {
            mDone = true;
            mQueue.notifyAll();
        }
        if (mDecodeThread != null) {
            try {
                Thread t = mDecodeThread;
                BitmapManager.instance().cancelThreadDecoding(t, mCr);
                MediaStore.Images.Thumbnails.cancelThumbnailRequest(mCr, -1);
                t.join();
                mDecodeThread = null;
                insertDataToDB();
            } catch (InterruptedException ex) {
                // so now what?
            }
        }
    }
}
