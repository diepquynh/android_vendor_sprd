/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif;

import java.io.File;
import java.util.ArrayList;

import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import com.ucamera.ucomm.puzzle.PuzzleActivity;
import com.android.camera2.R;

public class CollageMenuAdapter extends BaseAdapter {
    private LayoutInflater mInflater = null;
    private int MAX_BITMAP_NUM = 9;
    private Context mContext = null;
    // CID 109278 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mImagePaddingTop = 0;
    // private int mImagePaddingRight = 0;
    private int mThumbnailWidth = -1;
    private int mThumbnailHeight = -1;
    private int mGridItemWidth = 0;
    private ArrayList<String> mArrayFileNames = new ArrayList<String>();
    private ArrayList<Uri> mArrayUris = new ArrayList<Uri>();
    private ArrayList<Bitmap> mArrayBitmap = new ArrayList<Bitmap>();
    private ThumbnailListener mThumbnailListener;

    public interface ThumbnailListener{
        public void thumbnailChanged();
    }

    public CollageMenuAdapter(Context context) {
        mContext = context;
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        getImagePadding();
        countGridItemWidth();
    }

    private void getImagePadding() {
        // CID 124242 : Dead store to drawable
        //final Drawable drawable = mContext.getResources().getDrawable(R.drawable.collage_btn_delete);
        // CID 109278 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mImagePaddingTop = drawable.getIntrinsicHeight() / 2;
        // mImagePaddingRight = drawable.getIntrinsicWidth() / 2;
    }

    private void countGridItemWidth() {
        final Drawable drawable = mContext.getResources().getDrawable(R.drawable.collage_init_item);
        mGridItemWidth = drawable.getIntrinsicWidth();
    }

    public int getGridItemWidth() {
        return mGridItemWidth;
    }

    public ArrayList<Bitmap> getAllBitmap(){
        return mArrayBitmap;
    }

    public void setMaxItemCount(int count){ MAX_BITMAP_NUM = count;}
    public int getMaxItemCount() { return MAX_BITMAP_NUM; }
    public int getItemCount()    { return mArrayBitmap.size();}

    @Override
    public int getCount() {
        return getItemCount() + 1;
    }

    @Override
    public Bitmap getItem(int position) {
        if (position < getItemCount()) {
            return mArrayBitmap.get(position);
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.collage_menu_item_layout, parent, false);
            holder = new ViewHolder();
            holder.itemView = ((ImageView) convertView.findViewById(R.id.collage_grid_item_id));
            holder.delView = ((LinearLayout) convertView.findViewById(R.id.collage_grid_del));
            holder.textView = ((TextView) convertView.findViewById(R.id.collage_grid_number));

            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        if (position < mArrayBitmap.size()) {
            holder.textView.setVisibility(View.VISIBLE);
            holder.delView.setVisibility(View.VISIBLE);
            holder.delView.setOnClickListener(new MyDeleteOnClickListener(position));
            holder.itemView.setImageBitmap(createFitBitmap(mArrayBitmap.get(position)));
            holder.itemView.setBackgroundResource(R.drawable.collage_thumbnail_normal);
            holder.textView.setText("" + (position + 1));
        } else {
            holder.textView.setVisibility(View.INVISIBLE);
            holder.delView.setVisibility(View.INVISIBLE);
            holder.delView.setOnClickListener(null);
            holder.itemView.setImageBitmap(null);
            if(position == mArrayBitmap.size()) {
                holder.itemView.setBackgroundResource(R.drawable.collage_menu_item_bk);
            } else {
                holder.itemView.setBackgroundResource(R.drawable.collage_init_item_blank);
            }
        }

        return convertView;
    }

    public int getThumbnailWidth() {
        if (mThumbnailWidth == -1) {
            initThumbnailSize();
        }
        return mThumbnailWidth;
    }

    public int getThumbnailHeight() {
        if (mThumbnailHeight == -1) {
            initThumbnailSize();
        }
        return mThumbnailHeight;
    }

    public void updateThumbnail() {
        if (mArrayUris == null)
            return;
        for (int i = mArrayUris.size() - 1; i >= 0; i--) {
            File file = null;
            String path = Utils.getFilePathByUri(mArrayUris.get(i), mContext.getContentResolver());
            if (path != null) {
                file = new File(path);
            }

            if (path == null || !file.exists()) {
                mArrayBitmap.size();
                Utils.recycleBitmap(mArrayBitmap.remove(i));
                mArrayUris.remove(i);
                mArrayFileNames.remove(i);
                notifyDataSetChanged();
            }
        }
    }

    private void initThumbnailSize() {
        final Drawable drawable = mContext.getResources().getDrawable(R.drawable.collage_init_item);
        mThumbnailWidth = (int) (drawable.getIntrinsicWidth() - 14 * UiUtils.screenDensity());
        mThumbnailHeight = (int) (drawable.getIntrinsicHeight() - 14 * UiUtils.screenDensity());
    }

    /**
     * @param bitmap
     *            thumbnail
     * @param uri
     *            orginal image uri
     * @param strFileName
     *            orginal image file path used for delete, if null not delete
     *            orignal file
     */
    public void setThumbnail(Bitmap bitmap, Uri uri, String strFileName, boolean started) {
        if (bitmap == null || mArrayBitmap.size() == MAX_BITMAP_NUM) {
            return;
        }

        Bitmap bm;
        if (bitmap.getWidth() != getThumbnailWidth() || bitmap.getHeight() != getThumbnailHeight()) {
            bm = Bitmap.createScaledBitmap(bitmap, getThumbnailWidth(), getThumbnailHeight(), true);
        } else {
            bm = Bitmap.createBitmap(bitmap);
        }
        mArrayBitmap.add(bm);
        mArrayFileNames.add(strFileName);
        mArrayUris.add(uri);
        if(!started && mArrayBitmap.size() == MAX_BITMAP_NUM){
            gotoPuzzleActivity();
        }

        notifyDataSetChanged();
    }

    public void setThumbnail(Bitmap bitmap){
        if(bitmap == null){
            return;
        }
        mArrayBitmap.add(bitmap);
        notifyDataSetChanged();
    }

    private Bitmap createFitBitmap(Bitmap bitmap) {
        Bitmap bm;
        if (bitmap.getWidth() != getThumbnailWidth() || bitmap.getHeight() != getThumbnailHeight()) {
            bm = Bitmap.createScaledBitmap(bitmap, getThumbnailWidth(), getThumbnailHeight(), true);
        } else {
            bm = Bitmap.createBitmap(bitmap);
        }

        return bm;
    }

    public void setThumbnailListener(ThumbnailListener listener){
        mThumbnailListener = listener;
    }

    public void recyleBitmaps() {
        int nSize = mArrayBitmap.size();
        if (nSize == 0) {
            return;
        }
        Bitmap bitmap = null;
        for (int i = 0; i < nSize; i++) {
            bitmap = mArrayBitmap.get(i);
            Utils.recycleBitmap(bitmap);
        }
        mArrayBitmap.clear();
        mArrayFileNames.clear();
        mArrayUris.clear();
        if(mThumbnailListener != null){
            mThumbnailListener.thumbnailChanged();
        }
        notifyDataSetChanged();
    }

    public void gotoPuzzleActivity() {
         Intent intent = new Intent(mContext, PuzzleActivity.class);
         intent.putExtra(PuzzleActivity.INTENT_EXTRA_IMAGES,
         mArrayUris.toArray(new Uri[0]));
         mContext.startActivity(intent);
    }

    protected class MyDeleteOnClickListener implements OnClickListener {
        private int mPosition = -1;

        MyDeleteOnClickListener(int position) {
            mPosition = position;
        }

        @Override
        public void onClick(View v) {
            if (mPosition >= mArrayBitmap.size()) {
                return;
            }

            Utils.recycleBitmap(mArrayBitmap.remove(mPosition));
            if(mArrayUris != null && mArrayUris.size() > mPosition &&
                    mArrayFileNames != null && mArrayFileNames.size() > mPosition){
                /*
                 * BUG FIX: 3375
                 * FIX COMMENT: delete not really delete the picture.
                 */
                mArrayUris.remove(mPosition);
                mArrayFileNames.remove(mPosition);
            }
            if(mThumbnailListener != null){
                mThumbnailListener.thumbnailChanged();
            }
            notifyDataSetChanged();
        }

        private void deleteFile(Uri uri, String strFileName) {
            if (strFileName == null)
                return;
            StorageUtil storageUtil = StorageUtil.getInstance();
            String filePath = storageUtil.getFileDir();
            String strFullName = StorageUtil.getImageBucketId(filePath) + "/" + strFileName + ".jpg";

            /* CID 109313 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
            if(new File(strFullName).delete()){
                StorageUtil.deleteImage(mContext.getContentResolver(), uri);
            }
            /* @} */
        }
    }

    class ViewHolder {
        TextView textView;
        ImageView itemView;
        LinearLayout delView;
    }
}
