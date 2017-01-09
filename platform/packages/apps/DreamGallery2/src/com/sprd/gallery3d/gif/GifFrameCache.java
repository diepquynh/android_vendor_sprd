
package com.sprd.gallery3d.gif;

import java.util.LinkedHashMap;
import android.graphics.Bitmap;

public class GifFrameCache {
    public final static int MAX_CACHE_SIZE = 20;
    private LinkedHashMap<Integer, Bitmap> mHashMap;
    private int mRecoder = 0;

    public GifFrameCache() {
        mHashMap = new LinkedHashMap<Integer, Bitmap>();
    }

    public void addToCache(Integer key, Bitmap value) {
        synchronized (mHashMap) {
            if (mHashMap.size() < MAX_CACHE_SIZE) {
                mHashMap.put(key, value);
            } else {
                mHashMap.remove(mRecoder);
                mHashMap.put(key, value);
                mRecoder++;
            }
        }
    }

    public Bitmap getFormCache(Integer key) {
        synchronized (mHashMap) {
            return mHashMap.get(key);
        }
    }

    public void reset() {
        synchronized (mHashMap) {
            mHashMap.clear();
            mRecoder = 0;
        }
    }

}
