/*
 * Copyright (c) 2012 Thunder Software Technology Co.,Ltd.
 * All Rights Reserved.
 * Thundersoft Confidential and Proprietary.
 * Developed by Thundersoft Engineering team.
 */

package com.sprd.soundrecorder;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import com.android.soundrecorder.R;
import com.android.soundrecorder.SoundRecorder;
import com.android.soundrecorder.R.id;
import com.android.soundrecorder.R.layout;
import com.android.soundrecorder.R.string;
import com.android.soundrecorder.RecordService;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.content.SharedPreferences;


public class PathSelect extends ListActivity implements Button.OnClickListener{
    private List<String> items = null;
    private List<String> paths = null;
    private String sdcardPath = "/storage";
    private String curPath = "/storage";
    private String sdCardPathMnt = "/mnt";
    private String internalSdcard = "";
    private String externalSdcard = "";
    private String internalUID = "";
    private TextView mPath;
    private static final int HANDLER_SET_LISTADAPTER = 1;
    private PathSelectAdapter mSelectAdapter = null;
    private Object mHoldLock = new Object();
    //modify by linying
    //SoundRecorder need get stored path.
    public static final String DATA_BASE = "data_base";
    public static final String SAVE_PATH = "save_path";
    private static final int PATH_MAX_LENGTH = 31;
    //modify by linying
    private SharedPreferences mSavePath = null;
    public static boolean isStorageMounted = false;

    private File[] initFiles = null;

    final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case HANDLER_SET_LISTADAPTER:
                    synchronized (mHoldLock) {
                        if (null != items && null != paths && items.size() != 0
                                && paths.size() != 0) {
                            mSelectAdapter = new PathSelectAdapter(PathSelect.this, items, paths);
                            setListAdapter(mSelectAdapter);
                            // mSelectAdapter.notifyDataSetChanged();
                        }
                    }
                    break;
            }
        }
    };

    private void configSDCardPath() {
        File sdcardDirectory = null;
        sdcardDirectory = StorageInfos.getInternalStorageDirectory();
        if (sdcardDirectory != null) {
            internalUID = sdcardDirectory.getAbsolutePath();
            internalSdcard = internalUID.substring(0 , internalUID.lastIndexOf("/"));
        }

        sdcardDirectory = StorageInfos.getExternalStorageDirectory();
        if (sdcardDirectory != null) {
            externalSdcard = sdcardDirectory.getAbsolutePath();
        }
    }

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
//      requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.pathselect);
        configSDCardPath();
        mPath = (TextView) findViewById(R.id.mPath);
        Button buttonConfirm = (Button) findViewById(R.id.buttonConfirm);
        buttonConfirm.setOnClickListener(this);
        Button buttonCancle = (Button) findViewById(R.id.buttonCancle);
        buttonCancle.setOnClickListener(this);
        initializeList();
        mSavePath = getSharedPreferences(DATA_BASE, MODE_PRIVATE);
        if (mSavePath != null) {
            curPath = mSavePath.getString(SAVE_PATH, curPath);
        }
        getFileDir(curPath);
    }

    private void initializeList(){
        String sdState = EnvironmentEx.getExternalStoragePathState();
        String internalState = EnvironmentEx.getInternalStoragePathState();
        File internalPath = EnvironmentEx.getInternalStoragePath();
        File externalPath = EnvironmentEx.getExternalStoragePath();
        if(Environment.MEDIA_MOUNTED.equals(internalState) && Environment.MEDIA_MOUNTED.equals(sdState)) {
            initFiles = new File[]{internalPath, externalPath};
        } else if(Environment.MEDIA_MOUNTED.equals(internalState)){
            initFiles = new File[]{internalPath};
        } else {
            return;
        }
    }

    private void getFileDir(String filePath) {
        //filePath = subString(filePath, PATH_MAX_LENGTH);
        final String tmpFilePath = filePath;
        mPath.setText(tmpFilePath);
//        mPath.setTextSize(TypedValue.COMPLEX_UNIT_PX,27);
        // retrieve all of files under path, put it in worker thread
        new Thread() {
            public void run() {
                synchronized (mHoldLock) {
                    items = new ArrayList<String>();
                    paths = new ArrayList<String>();
                    File f = new File(tmpFilePath);
                    File[] files = null;
                    if(tmpFilePath.equals("/storage")) {
                        files = initFiles;
                    } else {
                        files = f.listFiles();
                    }
                    if (!tmpFilePath.equals(sdcardPath) && !tmpFilePath.equals(sdCardPathMnt)) {
                        items.add("back");
                        paths.add(f.getParent());
                    }
                    if (files != null) {
                        for (int i = 0; i < files.length; i++) {
                            File file = files[i];
                            if (file.isDirectory()) {
                                String path = file.getAbsolutePath();
                                if (((!"".equals(externalSdcard) && path.startsWith(externalSdcard) && StorageInfos
                                        .isExternalStorageMounted())
                                        || (!"".equals(internalSdcard) && path
                                                .equals(internalSdcard))
                                        || (!"".equals(internalUID) && path.startsWith(internalUID))) && !file.getName().startsWith(".")) {
                                    items.add(file.getName());
                                    paths.add(file.getPath());
                                }
                            }
                        }
                    }
                    if(paths != null){
                        Collections.sort(paths, new Comparator<String>() {

                            @Override
                            public int compare(String lhs, String rhs) {
                                if(null != lhs && rhs != null){
                                    return lhs.compareTo(rhs);
                                }
                                return 0;
                            }

                        });
                    }

                    // notify main thread to update UI
                    Message msg = new Message();
                    msg.what = HANDLER_SET_LISTADAPTER;
                    mHandler.sendMessage(msg);
                }
            }
        }.start();
    }

    /*private String subString(String str, int maxCount) {
        int strLength = 0;
        String reString = "";
        char[] tempChar = str.toCharArray();
        for (int i = 0; (i < tempChar.length && maxCount > strLength); i++) {
            String s1 = str.valueOf(tempChar[i]);
            byte[] charByte = s1.getBytes();
            if (charByte.length == 3) {
                strLength += charByte.length -1;
            }else {
                strLength += charByte.length;
            }
            reString += tempChar[i];
       }

       if (strLength >= maxCount) {
           reString =reString+"...";
       }
       return reString;
  }*/

    public void onClick(View button){
        if (!button.isEnabled())
            return;
        switch (button.getId()) {
            case R.id.buttonConfirm:
                if (!(curPath.startsWith(externalSdcard) || curPath.startsWith(internalUID))) {
                    curPath = internalUID;
                    Toast.makeText(getApplicationContext(), R.string.path_default, Toast.LENGTH_SHORT).show();
                }
                if(curPath.startsWith(externalSdcard) && !StorageInfos.isExternalStorageMounted()){
                    curPath = internalUID;
                    Toast.makeText(getApplicationContext(), R.string.error_sdcard_path, Toast.LENGTH_SHORT).show();
                }
                if(!StorageInfos.isExternalStorageMounted() && !StorageInfos.isInternalStorageMounted()){
                    Toast.makeText(this, R.string.stroage_not_mounted, Toast.LENGTH_LONG).show();
                    isStorageMounted = true;
                }else{
                //Intent  data = new Intent(PathSelect.this, SoundRecorder.class);
                //Bundle bundle = new Bundle();
                //bundle.putString("file", curPath);
                //data.putExtras(bundle);
                //setResult(2, data);
                    Intent intent = new Intent();
                    intent.setAction(RecordService.PATHSELECT_BROADCAST);
                    intent.putExtra("newPath", curPath);
                    sendBroadcast(intent);
                    Toast.makeText(getApplicationContext(), R.string.path_save, Toast.LENGTH_SHORT).show();
                }
                if (mSavePath != null) {
                    SharedPreferences.Editor edit = mSavePath.edit();
                    edit.putString(SAVE_PATH, curPath);
                    edit.commit();
                }
                /* SPRD: add new feature @{ */
                Intent  data = new Intent(PathSelect.this, RecordSetting.class);
                Bundle bundle = new Bundle();
                bundle.putString("path", curPath);
                data.putExtras(bundle);
                setResult(1, data);
                /* @} */
                finish();
                break;
            case R.id.buttonCancle:
                Toast.makeText(getApplicationContext(), R.string.path_nosave, Toast.LENGTH_SHORT).show();
                finish();
                break;
        }
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        if (position >= paths.size()) {
            return;
        }
        curPath = paths.get(position);
        if(curPath.equals("/storage/emulated")) {
            curPath = "/storage";
        }
        getFileDir(curPath);
    }

}
