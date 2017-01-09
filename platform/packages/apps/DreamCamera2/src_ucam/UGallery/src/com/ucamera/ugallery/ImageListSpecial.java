/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import java.util.ArrayList;
import java.util.List;
import java.util.TreeMap;
import android.net.Uri;

import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.util.Util;

public class ImageListSpecial {

    public static final String BURST_PREFIX = "Burst_";
    public static final String PANORAMA_PREFIX = "PANORAMA_";
    public static final String GIF_PREFIX = "GIF_";
    public static final String GIF_TYPE = "image/gif";

    private IImageList mIImageList;
    private ArrayList<IImage> mOrigImageList = new ArrayList<IImage>();
    private ArrayList<IImage> mFilterImageList = new ArrayList<IImage>();
    private TreeMap<String, ArrayList<IImage>> mBurstTreeMap = new TreeMap<String, ArrayList<IImage>>();
    private boolean mFilterBurst = true;

    public ImageListSpecial(IImageList imageList, boolean filter) {
        mIImageList = imageList;
        initImageList(filter);
    }

    public void setFilter(boolean filter) {
        mFilterBurst = filter;
    }

    private void initImageList(boolean filter) {
        if(mFilterImageList != null && mFilterImageList.size() > 0) {
            mFilterImageList.clear();
        }
        if(mOrigImageList != null && mOrigImageList.size() > 0) {
            mOrigImageList.clear();
        }
        for(int i=0;i < mIImageList.getCount();i++) {
            IImage image = mIImageList.getImageAt(i);
            if(filter && image.getTitle().startsWith(BURST_PREFIX)) {
                String dir = Util.getBurstPrefix(image.getTitle());
                if(dir != null && !mBurstTreeMap.containsKey(dir)) {
                    mFilterImageList.add(image);
                    ArrayList<IImage> list = new ArrayList<IImage>();
                    list.add(image);
                    mBurstTreeMap.put(dir, list);
                }else {
                    if(dir != null && mBurstTreeMap.get(dir) != null) {
                        mBurstTreeMap.get(dir).add(image);
                    }
                }
            }else {
                mFilterImageList.add(image);
            }
            mOrigImageList.add(image);
        }
    }

    public IImageList getAllImageList() {
        return mIImageList;
    }

    public ArrayList<IImage> getFilterImageList() {
        if(mFilterBurst) {
            return mFilterImageList;
        }
        return mOrigImageList;
    }

    public IImage getImageAtIndex(int index) {
        if(mFilterBurst) {
            if(index >= 0 && index < mFilterImageList.size()) {
                return mFilterImageList.get(index);
            }
        }else {
            if(index >= 0 && index < mOrigImageList.size()) {
                return mOrigImageList.get(index);
            }
        }
        return null;
    }

    public ArrayList<IImage> getAllImageAtIndex(int index) {
        ArrayList<IImage> imageList = new ArrayList<IImage>();
        if(mFilterBurst) {
            if(index < 0 || index >= mFilterImageList.size()) {
                return null;
            }
            IImage image = mFilterImageList.get(index);
            if(image.getTitle().startsWith(BURST_PREFIX)) {
                String key = Util.getBurstPrefix(image.getTitle());
                if(key != null && mBurstTreeMap.containsKey(key)) {
                    return mBurstTreeMap.get(key);
                }
            }
            imageList.add(image);
        }else {
            if(index < 0 || index >= mOrigImageList.size()) {
                return null;
            }
            IImage image = mOrigImageList.get(index);
            imageList.add(image);
        }
        return imageList;
    }

    public ArrayList<IImage> getAllImageByTitle(String title) {
        String key = Util.getBurstPrefix(title);
        if(key != null && key.startsWith(BURST_PREFIX) && mBurstTreeMap.containsKey(key)) {
            return mBurstTreeMap.get(key);
        }
        return null;
    }

    public int getCount() {
        if(mFilterBurst) {
            return mFilterImageList.size();
        }
        return mOrigImageList.size();
    }

    public int getCountAtList(List<IImage> list) {
        int count = 0;
        if(mFilterBurst) {
            for(IImage image : list) {
                String key = Util.getBurstPrefix(image.getTitle());
                if(key != null && mBurstTreeMap.containsKey(key)) {
                    count += mBurstTreeMap.get(key).size();
                }else{
                    count++;
                }
            }
        }else {
            count = list.size();
        }
        return count;
    }

    public int getCountAtPosition(int position) {
        if(!mFilterBurst) {
            if(position < 0 || position  > mOrigImageList.size() || mOrigImageList.get(position) == null){
                return 0;
            }
            return 1;
        }
        if(position < 0 || position  > mFilterImageList.size() || mFilterImageList.get(position) == null) {
            return 0;
        }
        String key = Util.getBurstPrefix(mFilterImageList.get(position).getTitle());
        if(key != null && mBurstTreeMap.containsKey(key)) {
            return mBurstTreeMap.get(key).size();
        }
        return 1;
    }

    public IImage getImageAtSelectedList(List<IImage> list,int index) {
        if(!mFilterBurst) {
            if(index >= 0 && index < list.size()) {
                return list.get(index);
            }
            return null;
        }
        List<IImage> allList = new ArrayList<IImage>();
        for(IImage image : list) {
            String key = Util.getBurstPrefix(image.getTitle());
            if(key != null && mBurstTreeMap.containsKey(key)) {
                allList.addAll(mBurstTreeMap.get(key));
            }else{
                allList.add(image);
            }
        }
        if(index >= 0 && index < allList.size()) {
            return allList.get(index);
        }
        return null;
    }

    public IImage getImageForUri(Uri uri) {
        return mIImageList.getImageForUri(uri);
    }

    public int getImageIndex(IImage image) {
        if(mFilterBurst) {
            return mFilterImageList.indexOf(image);
        }
        return mOrigImageList.indexOf(image);
    }

    public ArrayList<IImage> getBurstImage(String key) {
        if(key != null && mBurstTreeMap.containsKey(key)) {
            return mBurstTreeMap.get(key);
        }
        return null;
    }

    public void close() {
        mIImageList.close();
        mFilterImageList.clear();
        mFilterImageList = null;
        mOrigImageList.clear();
        mOrigImageList = null;
        mBurstTreeMap.clear();
    }
}
