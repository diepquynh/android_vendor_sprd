/**
 *   Copyright (C) 2010,2013 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.RelativeLayout;

@SuppressWarnings("deprecation")
public class DownloadTabActivity extends Activity implements OnClickListener{
    private static boolean isNeedManga = true;
    private static boolean isNeedNetworkPermissoin = false;
    public static void setNeedManga(boolean isNeed) {
        isNeedManga = isNeed;
    }

    public static void setNetworkPermisson(boolean network) {
        isNeedNetworkPermissoin = network;
    }

    public static boolean isNeedNetworkPermission() {
        return isNeedNetworkPermissoin;
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

       String downloadType = getIntent().getStringExtra(Constants.ACTION_DOWNLOAD_TYPE);
       if(!TextUtils.isEmpty(downloadType)) {
           Intent intent = new Intent(this,DownloadCenterActivity.class);
           intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, downloadType);
           startActivity(intent);
           finish();
           return;
       }

       setContentView(R.layout.download_tab_layout);
       ((RelativeLayout)findViewById(R.id.scenery_background_layout)).setOnClickListener(this);
       ((RelativeLayout)findViewById(R.id.collage_background_layout)).setOnClickListener(this);
       ((RelativeLayout)findViewById(R.id.frame_background_layout)).setOnClickListener(this);
       ((RelativeLayout)findViewById(R.id.texture_background_layout)).setOnClickListener(this);
       ((RelativeLayout)findViewById(R.id.decoration_background_layout)).setOnClickListener(this);
       ((RelativeLayout)findViewById(R.id.font_background_layout)).setOnClickListener(this);
       if(isNeedManga) {
           ((RelativeLayout)findViewById(R.id.manga_background_layout)).setVisibility(View.VISIBLE);
           ((RelativeLayout)findViewById(R.id.manga_background_layout)).setOnClickListener(this);
       }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onClick(View v) {
        Intent intent = new Intent(this,DownloadCenterActivity.class);
        switch(v.getId()) {
        case R.id.scenery_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_FRAME_VALUE);
            break;
        }
        case R.id.collage_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_PUZZLE_VALUE);
            break;
        }
        case R.id.frame_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_PHOTO_FRAME_VALUE);
            break;
        }
        case R.id.texture_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_TEXTURE_VALUE);
            break;
        }
        case R.id.decoration_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_DECOR_VALUE);
            break;
        }
        case R.id.font_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_FONT_VALUE);
            break;
        }
        case R.id.manga_background_layout: {
            intent.putExtra(Constants.ACTION_DOWNLOAD_TYPE, Constants.EXTRA_MANGA_FRAME_VALUE);
            break;
        }
        default: {
            break;
        }
        }
        startActivity(intent);
    }
}
