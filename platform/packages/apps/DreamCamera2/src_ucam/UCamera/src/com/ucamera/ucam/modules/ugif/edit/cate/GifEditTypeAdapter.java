/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import com.ucamera.ucam.modules.ugif.edit.GifEditActivity;
import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import com.ucamera.ucomm.downloadcenter.DownloadCenter;
import com.ucamera.ucomm.downloadcenter.DownloadCenter.OnLoadCallback;

public class GifEditTypeAdapter extends BaseAdapter implements OnItemClickListener{
    private GifBasicCate mContent;
    @Target(ElementType.METHOD)
    @Retention(RetentionPolicy.RUNTIME)
    public static @interface ListenerId {
        int Id() default 0;
    }

    public GifEditTypeAdapter(Context context, int cate, ProcessCallback callback) {
        init(cate, context, callback);
    }

    private int mCurrentType = -1;
    private void init(int cate, Context context, final ProcessCallback callback) {
        mCurrentType = cate;
        switch (cate) {
        case GifEditActivity.ACTION_BASIC_EDIT_FUNCTION:
            mContent = new BasicEditCate(context, callback);
            break;
        case GifEditActivity.ACTION_EDIT_EFFECT:
            mContent = new EffectCate(context, callback);
            break;
        case GifEditActivity.ACTION_PHOTO_FRAME:
            mContent = new PhotoFrameCate(context, callback);
            DownloadCenter.loadPhotoFrame(context, new OnLoadCallback() {
                @Override
                public void onLoadComplete(String[] result) {
                    callback.updateAapter(result);
                }
            });
            break;
        case GifEditActivity.ACTION_TEXTURE:
            mContent = new TextureCate(context, callback);
            DownloadCenter.loadTexture(context, new OnLoadCallback() {
                @Override
                public void onLoadComplete(String[] result) {
                    callback.updateAapter(result);
                }
            });
            break;
        case GifEditActivity.ACTION_PLAY_SPEED:
            mContent = new PlaySpeedCate(context, callback);
            break;
        case GifEditActivity.ACTION_PLAY_MODE:
            mContent = new PlayModeCate(context, callback);
            break;
        default:
            break;
        }
    }

    @Override
    public int getCount() {
        return mContent != null ? mContent.getCount() : 0;
    }

    @Override
    public Object getItem(int position) {
        return position;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setHighlight(int selected) {
        if (mContent != null) {
            mContent.setHighlight(selected);
//            notifyDataSetChanged();
        }
    }

    public void updateContents(String[] strings) {
        if (mContent != null) {
            mContent.updateContents(strings);
            notifyDataSetChanged();
        }
    }

    public void setItemMaxSize(int size) {
        if (mContent != null) {
            mContent.setItemMaxSize(size);
        }
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        return mContent != null ? mContent.getView(position, convertView, parent) : null;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (mContent != null) {
            mContent.onItemClick(position);
            /*
             * FIX BUG:4878
             * BUG COMMENT:don't set high light if set last item
             * DATA :2013.11.08
             */
            if(mCurrentType != GifEditActivity.ACTION_PHOTO_FRAME && mCurrentType != GifEditActivity.ACTION_TEXTURE) {
                setHighlight(position);
            }
            notifyDataSetChanged();
        }
    }
}
