/*
 *   Copyright (C) 2010,2013 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucam.modules.ugif;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.WeakHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.android.camera2.R;
import com.ucamera.ucam.modules.compatible.Models;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;

import android.app.Activity;
import android.app.ActivityGroup;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.Media;
import android.provider.MediaStore.Images.Thumbnails;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

public class GifBrowser extends ActivityGroup implements View.OnClickListener, OnScrollListener{
    private GridView gridView;
    private LayoutInflater inflater;
    private static final String WHERE_CLAUSE = "(" + Media.MIME_TYPE + " in ('image/gif'))"
            + " AND " + Media.SIZE + "> 0 ";
//    private float roundPix;
    private long[] mThumbnailIDList;
    private String[] mGifPathList;
    private long[] mGifSizeList;
    private MySimpleAdapter myAdapter;
    private boolean showCheckBox = false;
    private int[] mItemSelectedList;
    private int numOfselectedPic = 0;
    private int mCellWidth;
    private int mCellHeight;
    private Context mContext;
    private TextView mMultipleSelect, mAllSelect, mCancelAllSelect,mDelete;
    private boolean mPause = false;
    private boolean mGridViewScrollStarted = false;
    private ExecutorService executorService;
    private ImageLoader imageLoader;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /*SPRD:fix bug527736 Some Activity about UCamera lacks method of checkpermission@{ */
        if (!UCamUtill.checkPermissions(this)) {
            finish();
            return;
        }
        /* }@ */
        mContext = this;
        imageLoader=new ImageLoader(mContext);
        setContentView(R.layout.gif_mode_image_browser);
        inflater = (LayoutInflater) this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        executorService = Executors.newFixedThreadPool(5);

        mMultipleSelect = (TextView) findViewById(R.id.btn_multi_select);
        mMultipleSelect.setOnClickListener(this);
        mAllSelect = (TextView) findViewById(R.id.btn_all_select);
        mAllSelect.setOnClickListener(this);
        mCancelAllSelect = (TextView) findViewById(R.id.btn_multi_cancel);
        mCancelAllSelect.setOnClickListener(this);

        /* SPRD: fix bug 526266 @{*
         * mShare = (TextView) findViewById(R.id.btn_share);
        mShare.setOnClickListener(this);
        @}*/

        mDelete = (TextView) findViewById(R.id.btn_delete);
        mDelete.setOnClickListener(this);


        gridView = (GridView) findViewById(R.id.gif_mode_image_gridview);
        gridView.setNumColumns(3);
        gridView.setLayoutParams(new RelativeLayout.LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.WRAP_CONTENT));
        gridView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View arg1, int arg2,
                    long arg3) {
                if (showCheckBox) {
                    if (mItemSelectedList[arg2] == -1) {
                        mItemSelectedList[arg2] = arg2;
                        numOfselectedPic++;
                    } else {
                        mItemSelectedList[arg2] = -1;
                        numOfselectedPic--;
                    }
                    myAdapter.notifyDataSetChanged();
                    setButtonState(numOfselectedPic > 0);
                } else {
                    Intent intent = new Intent(GifBrowser.this,
                            GifPlayActivity.class);
                    Uri shareUri = Uri.parse(Images.Media.EXTERNAL_CONTENT_URI
                            .toString()
                            + "/"
                            + String.valueOf(mThumbnailIDList[arg2]));
                    Bundle b = new Bundle();
                    // add boolean to discriminate playing gif from other applition.
                    b.putBoolean("fromOutSide", false);
                    // b.putStringArray("pathArray", gifPathStrings);
                    // b.putInt("index", arg2);
                    // b.putLongArray("size", gifSize);
                    // b.putLongArray("idList", thumbnailIDList);
                    intent.putExtras(b);
                    intent.setData(shareUri);
                    startActivity(intent);
                    mPause = true;
                    finish();
                }
            }
        });
        gridView.setOnScrollListener(this);

        mCellWidth = mCellHeight = (UiUtils.screenWidth() - UiUtils.dpToPixel(20)) / 3;

        createAdapter(GifUtils.getGifListFromDB(mContext));
        resetState();
    }

    private void setMultipleMode(boolean multiple) {
        int visible = multiple ? View.VISIBLE : View.GONE;
        if(mThumbnailIDList == null || mThumbnailIDList.length == 0) {
            findViewById(R.id.layout_no_gifs).setVisibility(View.VISIBLE);
        }else {
            findViewById(R.id.layout_no_gifs).setVisibility(View.GONE);
        }
        findViewById(R.id.layout_multi_select).setVisibility(View.GONE - visible);
        findViewById(R.id.layout_all_select).setVisibility(visible);
        findViewById(R.id.layout_multi_cancel).setVisibility(visible);
        // SPRD: fix bug 526266
        // findViewById(R.id.layout_share).setVisibility(visible);
        findViewById(R.id.layout_delete).setVisibility(visible);
    }

    private void setButtonState(boolean hasSelected) {
        if(mThumbnailIDList == null || numOfselectedPic == mThumbnailIDList.length) {
            mAllSelect.setEnabled(false);
            mMultipleSelect.setEnabled(false);
        }else {
            mAllSelect.setEnabled(true);
            mMultipleSelect.setEnabled(true);
        }
        mCancelAllSelect.setEnabled(hasSelected);
        /* SPRD: fix bug 526266 @{*
         * mShare.setEnabled(hasSelected);
        @}*/

        mDelete.setEnabled(hasSelected);
    }
    class MySimpleAdapter extends SimpleAdapter {
        private Context mContext;
        private int mResource;
        private int[] mTo;
        private long[] mBitmaps;
        private List<Bitmap> mBitmapList = new ArrayList<Bitmap>();
        private int mCurrentIndex = 0;


        public MySimpleAdapter(Context context, List<? extends Map<String, ?>> data, int resource,
                String[] from, int[] to, long[] bmp) {
            super(context, data, resource, from, to);
            mResource = resource;
            mTo = to;
            mBitmaps = bmp;
            mContext = context;
        }

        public void addItem(Bitmap bitmap, int index) {
            if (index > 0 && index < mBitmapList.size()) {
                mBitmapList.set(index, bitmap);
            } else {
                mBitmapList.add(index, bitmap);
            }
        }

        public Bitmap getItem(int position) {
            if(position < 0 || position >= mBitmapList.size()) {
                return null;
            }
            return mBitmapList.get(position);
        }

        public void updateDisplay() {
            notifyDataSetChanged();
        }

        public int getCount() {
            return mBitmaps.length;
        }

        public int getCurrentIndex() {
            return mCurrentIndex;
        }

        public long getItemId(int position) {
            return position;
        }

        public void setCheckboxOff(boolean bOff) {
            if (bOff) {
                for (int i = 0; i < getCount(); i++) {
                    mItemSelectedList[i] = -1;
                }
                showCheckBox = true;
            } else {
                showCheckBox = false;
            }

        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            mCurrentIndex = position;
            View view;
            if (convertView == null) {
                view = inflater.inflate(mResource, parent, false);
            } else {
                view = convertView;
            }
            ImageView v = (ImageView) view.findViewById(mTo[0]);
            imageLoader.DisplayImage(mBitmaps[position], v);
            ImageView checkboxOffView = (ImageView) view.findViewById(R.id.gif_image_checkbox_off);
            ImageView checkboxOnView = (ImageView) view.findViewById(R.id.gif_image_checkbox_on);
            if (showCheckBox) {
                if (mItemSelectedList[position] == -1) {
                    checkboxOffView.setVisibility(View.VISIBLE);
                    checkboxOnView.setVisibility(View.INVISIBLE);
                } else {
                    checkboxOnView.setVisibility(View.VISIBLE);
                    checkboxOffView.setVisibility(View.INVISIBLE);
                }
            } else {
                checkboxOnView.setVisibility(View.INVISIBLE);
                checkboxOffView.setVisibility(View.INVISIBLE);
            }
            return view;
        }
    }

    private void setItemView(ImageView v, Bitmap bm) {
        if (bm == null) {
            Drawable missB = mContext.getResources().getDrawable(
                    R.drawable.missing_thumbnail_picture);
            bm = drawableToBitmap(missB);
        }
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) v.getLayoutParams();
        params.height = mCellHeight;
        params.width = mCellWidth;
        v.setLayoutParams(params);
        v.setImageBitmap(bm);
        v.setBackgroundDrawable(null);
    }

    Runnable mDeleteRunnable = new Runnable() {
        private boolean deleteFailed;
        public void run() {
            for (int i = 0; i < mItemSelectedList.length; i++) {
                if (mItemSelectedList[i] != -1) {
                    File f = new File(
                            mGifPathList[mItemSelectedList[i]]);
                    /* SPRD: CID 109205 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
                    if(f.delete()){
                        Uri delUri = Uri.parse(Images.Media.EXTERNAL_CONTENT_URI
                                .toString()
                                + "/"
                                + String.valueOf(mThumbnailIDList[mItemSelectedList[i]]));
                        getContentResolver().delete(delUri, null, null);
                        numOfselectedPic--;
                        updateProgress();
                    } else{
                        deleteFailed = true;
                    }
                    /* @} */
                }
            }
            if(deleteFailed){
                runOnUiThread( new Runnable() {
                    public void run() {
                        Toast.makeText(mContext, "delete file failed", Toast.LENGTH_SHORT).show();
                        mDelProgressDialog.dismiss();
                        resetState();
                    }
                });
            }
        }
    };

    private ProgressDialog mDelProgressDialog;
    private void showDeleteProgressDialog(int deleteCount) {
        if(mDelProgressDialog == null) {
            mDelProgressDialog = new ProgressDialog(this);
        }
        mDelProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mDelProgressDialog.setMax(deleteCount);
        mDelProgressDialog.setTitle(R.string.text_waiting);
        mDelProgressDialog.setCancelable(false);
        mDelProgressDialog.show();
    }

    private void updateProgress() {
            runOnUiThread( new Runnable() {
                public void run() {
                    if(numOfselectedPic > 0) {
                        if(mDelProgressDialog != null){
                            mDelProgressDialog.incrementProgressBy(1);
                        }
                    } else {
                        if(mDelProgressDialog != null){
                            mDelProgressDialog.dismiss();
                            mDelProgressDialog = null;
                        }
                        createAdapter(GifUtils.getGifListFromDB(mContext));
                        resetState();
                    }
                }
            });
    }

    public void deletelGifs() {
        if (numOfselectedPic > 0) {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.text_edit_exit_tip_title)
                    .setMessage(R.string.pref_camera_review_del)
                    .setPositiveButton(R.string.text_multi_select_confirm,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    showDeleteProgressDialog(numOfselectedPic);
                                    new Thread(mDeleteRunnable).start();
                                    dialog.dismiss();
                                }
                            })
                    .setNegativeButton(R.string.text_multi_select_cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            }).show();
        }
    }

    private int createAdapter(Cursor cursor) {
        int numOfGIF = 0;
        if (cursor != null) {
            cursor.moveToLast();
            numOfGIF = cursor.getCount();
            if (numOfGIF == 0) {
                return numOfGIF;
            }
            mThumbnailIDList = new long[numOfGIF];
            mGifPathList = new String[numOfGIF];
            mGifSizeList = new long[numOfGIF];
            mItemSelectedList = new int[numOfGIF];
            for(int i = 0, j =0; i < numOfGIF; i++){
                /*
                 * BUG FIX: 1495 FIX COMMENT: remove redundant code for get
                 * thumbnail url Date: 2012-08-21
                 */
                if(cursor.getLong(0) != -1) {
                    mThumbnailIDList[j] = cursor.getLong(0);
                    mGifPathList[j] = cursor.getString(1);
                    mGifSizeList[j] = cursor.getLong(2);
                    mItemSelectedList[j] = -1; // fix bug 5359
                    j++;
                }
                cursor.moveToPrevious();
            };
            cursor.close();
        } else {
            return numOfGIF;
        }
        myAdapter = new MySimpleAdapter(this, null, R.layout.gif_mode_image_browser_item,
                new String[] {
                    "image_list"
                }, new int[] {
                    R.id.gif_image_list
                }, mThumbnailIDList);

        gridView.setAdapter(myAdapter);
        loadThumbnail();
        return numOfGIF;
    }

    private void loadThumbnail() {
        executorService.submit(mLoadThumbnailThread);
    }

    private Runnable mLoadThumbnailThread = new Runnable() {
        @Override
        public void run() {
            Bitmap b = null;
            int start = myAdapter.getCurrentIndex() - 36;
            if(start < 0) {
                start = 0;
            }
            int end = myAdapter.getCurrentIndex() + 36;
            if(end >= mThumbnailIDList.length) {
                end = mThumbnailIDList.length;
            }
            /* SPRD: fix bug 528267 Load from the second Thumbnail. @{*
             * The first Thumbnail should only be got by getView.
            @}*/
            for(int i = 1; i < myAdapter.getCount(); i++) {
                if(mPause) return;
                if(i >= start && i < end) {
                    if(myAdapter.getItem(i) != null && !myAdapter.getItem(i).isRecycled()) {
                        continue;
                    }
                    try {
                        b = Thumbnails.getThumbnail(mContext.getContentResolver(), mThumbnailIDList[i],
                                Images.Thumbnails.MINI_KIND, Utils.getNativeAllocOptions());
                    } catch (Throwable ex) {
                        Log.e("GIFBrowser", "microThumbBitmap got exception", ex);
                    }
                    myAdapter.addItem(b, i);
                }else {
                    Bitmap bitmap = myAdapter.getItem(i);
                    if(bitmap != null && !bitmap.isRecycled()) {
                        bitmap.recycle();
                        bitmap = null;
                    }
                }
            }
        }
    };

    /**
     * clear the current state and set state to initial
     */
    @Override
    protected void onRestart() {
        super.onRestart();
        /*
         * FIX BUG: 1675 BUG CAUSE: clear the current state and set state to
         * initial FIX COMMENT: Do not clear the current state and set state to
         * initial DATE: 2012-10-10
         */
        // initViews();
    }

    public void shareGifs() {
            if (numOfselectedPic > 0) {
                ArrayList<Uri> uriList = new ArrayList<Uri>();
                for (int i = 0; i < mItemSelectedList.length; i++) {
                    if (mItemSelectedList[i] != -1) {
                        Uri shareUri = Uri.parse(Images.Media.EXTERNAL_CONTENT_URI.toString() + "/"
                                + String.valueOf(mThumbnailIDList[mItemSelectedList[i]]));
                        uriList.add(shareUri);
                    }
                }
                if (uriList.size() == 1) {
//                    ShareUtils.shareImage(this, uriList.get(0));
                } else {
                    Intent intent = new Intent();
                    intent.setType("image/*");
                    intent.setAction(Intent.ACTION_SEND_MULTIPLE);
                    intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, uriList);
                    startActivity(intent);
                }
            }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (showCheckBox) {
                resetState();
                return true;
            }
        } else if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private void resetState() {
        showCheckBox = false;
        numOfselectedPic = 0;
        if(myAdapter != null){
            myAdapter.setCheckboxOff(false);
            myAdapter.notifyDataSetChanged();
        }
        setMultipleMode(false);
        setButtonState(false);
    }

    @Override
    protected void onPause() {
        mPause = true;
        super.onPause();
        // StatApi.onPause(this);
    }

    @Override
    protected void onResume() {
        mPause = false;
        super.onResume();
        // StatApi.onResume(this);
    }

    @Override
    protected void onDestroy() {
        myAdapter = null;
        if (imageLoader != null) {
            imageLoader.clearCache();
            imageLoader = null;
        }
        mThumbnailIDList = null;
        super.onDestroy();
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()) {
        case R.id.btn_multi_select:
            setMultipleMode(true);
            showCheckBox = true;
            myAdapter.setCheckboxOff(true);
            myAdapter.notifyDataSetChanged();
            break;
        case R.id.btn_all_select:
            for(int i = 0; i < myAdapter.getCount() && i < mItemSelectedList.length; i++) {
                mItemSelectedList[i] = i;
            }
            myAdapter.notifyDataSetChanged();
            numOfselectedPic = myAdapter.getCount();
            setButtonState(true);
            break;
        case R.id.btn_multi_cancel:
            for(int i = 0; i < myAdapter.getCount() && i < mItemSelectedList.length; i++) {
                mItemSelectedList[i] = -1;
            }
            myAdapter.notifyDataSetChanged();
            numOfselectedPic = 0;
            setButtonState(false);
            break;
            /* SPRD: fix bug 526266 @{*
             *case R.id.btn_share:
            shareGifs();
            break;
            @}*/
        case R.id.btn_delete:
            deletelGifs();
            break;
        }
    }

    @Override
    public void onScrollStateChanged(AbsListView view, int scrollState) {
        if(scrollState == OnScrollListener.SCROLL_STATE_FLING) {
            mGridViewScrollStarted = true;
        } else if(scrollState == OnScrollListener.SCROLL_STATE_IDLE && mGridViewScrollStarted){
            loadThumbnail();
            mGridViewScrollStarted = false;
        }
    }

    @Override
    public void onScroll(AbsListView view, int firstVisibleItem,
            int visibleItemCount, int totalItemCount) {
    }

    /* SPRD:Add for bug 534534 @{ */
    public Bitmap drawableToBitmap(Drawable drawable) {
        int width = drawable.getIntrinsicWidth();
        int height = drawable.getIntrinsicHeight();
        Bitmap.Config config = drawable.getOpacity() != PixelFormat.OPAQUE ? Bitmap.Config.ARGB_8888
                : Bitmap.Config.RGB_565;
        Bitmap bitmap = Bitmap.createBitmap(width, height, config);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, width, height);
        drawable.draw(canvas);
        return bitmap;
    }
    /* @} */

    /* SPRD:add for bug 535328 @{ */
    public class MemoryCache {

        private static final String TAG = "MemoryCache";
        private Map<Long, Bitmap> cache = Collections
                .synchronizedMap(new LinkedHashMap<Long, Bitmap>(10, 1.5f, true));
        private long size = 0;
        private long limit = 1000000;// max memory in bytes

        public MemoryCache() {
            // use 25% of available heap size
            setLimit(Runtime.getRuntime().maxMemory() / 4);
        }

        public void setLimit(long new_limit) {
            limit = new_limit;
            Log.i(TAG, "MemoryCache will use up to " + limit / 1024. / 1024. + "MB");
        }

        public Bitmap get(long id) {
            try {
                if (!cache.containsKey(id))
                    return null;
                return cache.get(id);
            } catch (NullPointerException ex) {
                return null;
            }
        }

        public void put(long id, Bitmap bitmap) {
            try {
                if (cache.containsKey(id))
                    size -= getSizeInBytes(cache.get(id));
                cache.put(id, bitmap);
                size += getSizeInBytes(bitmap);
                checkSize();
            } catch (Throwable th) {
                th.printStackTrace();
            }
        }

        private void checkSize() {
            Log.i(TAG, "cache size=" + size + " length=" + cache.size());
            if (size > limit) {
                Iterator<Entry<Long, Bitmap>> iter = cache.entrySet().iterator();
                while (iter.hasNext()) {
                    Entry<Long, Bitmap> entry = iter.next();
                    size -= getSizeInBytes(entry.getValue());
                    iter.remove();
                    if (size <= limit)
                        break;
                }
                Log.i(TAG, "Clean cache. New size " + cache.size());
            }
        }

        public void clear() {
            cache.clear();
        }

        long getSizeInBytes(Bitmap bitmap) {
            if (bitmap == null)
                return 0;
            return bitmap.getRowBytes() * bitmap.getHeight();
        }
    }

    class ImageLoader {

        MemoryCache memoryCache = new MemoryCache();

        private Map<ImageView, Long> imageViews = Collections
                .synchronizedMap(new WeakHashMap<ImageView, Long>());

        ExecutorService executorService;

        public ImageLoader(Context context) {
            executorService = Executors.newFixedThreadPool(5);
        }

        //CID:124514 unread field
        //final int stub_id = R.drawable.missing_thumbnail_picture;

        public void DisplayImage(long thumbId, ImageView imageView) {
            imageViews.put(imageView, thumbId);

            Bitmap bitmap = memoryCache.get(thumbId);
            if (bitmap != null)
                setItemView(imageView, bitmap);
            else {
                queuePhoto(thumbId, imageView);
                setItemView(imageView, bitmap);
                // imageView.setImageResource(stub_id);
            }
        }

        private void queuePhoto(long thumbId, ImageView imageView) {
            PhotoToLoad p = new PhotoToLoad(thumbId, imageView);
            executorService.submit(new PhotosLoader(p));
        }

        private Bitmap getBitmap(long thumbId) {

            try {
                Bitmap bm = Thumbnails.getThumbnail(mContext.getContentResolver(), thumbId,
                        Images.Thumbnails.MINI_KIND, Utils.getNativeAllocOptions());
                return bm;
            } catch (Exception ex) {
                ex.printStackTrace();
                return null;
            }
        }

        private class PhotoToLoad {
            public long thumbId;
            public ImageView imageView;

            public PhotoToLoad(long id, ImageView i) {
                thumbId = id;
                imageView = i;
            }
        }

        class PhotosLoader implements Runnable {
            PhotoToLoad photoToLoad;

            PhotosLoader(PhotoToLoad photoToLoad) {
                this.photoToLoad = photoToLoad;
            }

            @Override
            public void run() {
                if (imageViewReused(photoToLoad))
                    return;
                Bitmap bmp = getBitmap(photoToLoad.thumbId);
                memoryCache.put(photoToLoad.thumbId, bmp);
                if (imageViewReused(photoToLoad))
                    return;
                BitmapDisplayer bd = new BitmapDisplayer(bmp, photoToLoad);
                Activity a = (Activity) photoToLoad.imageView.getContext();
                a.runOnUiThread(bd);
            }
        }

        boolean imageViewReused(PhotoToLoad photoToLoad) {
            long tag = imageViews.get(photoToLoad.imageView);
            if (tag == -1 || tag != photoToLoad.thumbId)
                return true;
            return false;
        }

        class BitmapDisplayer implements Runnable {
            Bitmap bitmap;
            PhotoToLoad photoToLoad;

            public BitmapDisplayer(Bitmap b, PhotoToLoad p) {
                bitmap = b;
                photoToLoad = p;
            }

            public void run() {
                if (imageViewReused(photoToLoad))
                    return;
                setItemView(photoToLoad.imageView, bitmap);
            }
        }

        public void clearCache() {
            memoryCache.clear();
        }

    }
    /* @} */
}
