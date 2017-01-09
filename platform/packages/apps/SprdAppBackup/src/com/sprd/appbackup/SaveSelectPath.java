/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.sprd.appbackup;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.io.IOException;

import com.sprd.appbackup.RestoreFragment.scanRestoreFile;
import com.sprd.appbackup.activities.MainActivity;

import android.app.AlertDialog;
import android.app.ActionBar;
import android.app.ProgressDialog;
import android.app.ListActivity;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnCancelListener;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;
import android.provider.Settings;
import android.provider.MediaStore;
import android.widget.Toast;
import android.content.Context;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.content.ContentResolver;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.view.WindowManager;
import android.view.LayoutInflater;
import android.widget.EditText;
import android.app.Activity;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class SaveSelectPath extends ListActivity {

    public static final String GET_FILE = "android.backup.action.GET_FILE";
    String externalStoragePath = EnvironmentEx.getInternalStoragePath().getPath();//TCard
    private final static String TAG = "SaveSelectPath";

    private File mCurrentDir = new File(externalStoragePath);

    // menu item constant
    private static final int MENU_ID_PATH_SAVE = Menu.FIRST;
    private static final int MENU_ID_PATH_PROP = Menu.FIRST + 1;
    private FileListAdapter mOnlyAdapter;
    boolean fileDisplay = false;
    boolean mBackup = false;
    boolean mRestore = false;
    TextView mTitleText;
    private final static boolean DBG = true;
    private final static int MSG_NOTIFY_DATA_CHANGED = 1;
    private ExecutorService mSaveSelectPathThread = Executors.newSingleThreadExecutor();

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM);
        View customTitle = getLayoutInflater().inflate(R.layout.custom_title, null);

        actionBar.setCustomView(customTitle);
        mTitleText = (TextView) customTitle.findViewById(R.id.custom_title);

        if(DBG) {Log.d(TAG, "SaveSelectPath onCreate()././.");}

        Intent mIntent = this.getIntent();
        Bundle bundle = mIntent.getExtras();
        if (bundle != null) {
            String storage = bundle.getString("storage");

            if(DBG) {Log.d(TAG, "SaveSelectPath onCreate() storage = " + storage);}

            if(!storage.endsWith("SD")){
                externalStoragePath = EnvironmentEx.getInternalStoragePath().getPath();//TCard
            }else {
                externalStoragePath = EnvironmentEx.getExternalStoragePath().getPath();//TCard
            }
            if(DBG) {Log.d(TAG, "SaveSelectPath onCreate() externalStoragePath = " + externalStoragePath);}
        }

        mCurrentDir = new File(externalStoragePath);
        if (GET_FILE.equals(mIntent.getAction())) {
            fileDisplay = true;
        } else {
            fileDisplay = false;
        }

        if (bundle != null) {
            String selectPath = bundle.getString("selectbackupPath");
            if(selectPath.endsWith("selectrestorePath")){
                if(DBG) {Log.d(TAG, "SaveSelectPath onCreate() restore ");}
                mBackup = false;
                mRestore = true;
            }else {
                if(DBG) {Log.d(TAG, "SaveSelectPath onCreate() selectbackupPath ");}
                mBackup = true;
                mRestore = false;
            }
        }

        mOnlyAdapter = new FileListAdapter(this);
        if(DBG) {Log.d(TAG, "SaveSelectPath onCreate() fileDisplay =  " + fileDisplay);}
        if (fileDisplay) {
            mOnlyAdapter.sortImpl(new File(externalStoragePath), "*");
        } else {
            mOnlyAdapter.sortImpl(new File(externalStoragePath), "0");
        }
        /* SPRD: modify for bug 632148 @{ */
        mHandler.sendEmptyMessage(MSG_NOTIFY_DATA_CHANGED);
        /* @} */
        setListAdapter(mOnlyAdapter);

        if(DBG) {Log.d(TAG, "SaveSelectPath onCreate()././.updateTitle");}
        updateTitle();
    }
    public void updateListView() {
        if(mSaveSelectPathThread != null && !mSaveSelectPathThread.isShutdown()){
            mSaveSelectPathThread.execute(new SaveSelectPathThread());
        }
    }

    private static final int MAX_FILENAME_LEN = 60;

    public void updateTitle() {
        if (mCurrentDir == null) {
            mTitleText.setText(getString(R.string.app_name));
            return;
        }

        mTitleText.setText(mCurrentDir.getPath());
    }

    class SaveSelectPathThread implements Runnable{
        @Override
        public void run() {
            Log.d(TAG,"SaveSelectPath SaveSelectPathThread");

            if (fileDisplay) {
                mOnlyAdapter.sortImpl(mCurrentDir, "*");
            } else {
                mOnlyAdapter.sortImpl(mCurrentDir, "0");
            }
            if(DBG) {Log.d(TAG, " SaveSelectPath SaveSelectPathThread end");}
            mHandler.sendEmptyMessage(MSG_NOTIFY_DATA_CHANGED);
        }
    }

    public Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
            case MSG_NOTIFY_DATA_CHANGED:
                if(DBG) {Log.d(TAG, " SaveSelectPath position");}
                mOnlyAdapter.notifyDataSetChanged();
                updateTitle();
                getListView().setSelection(0);
                break;
            default:
                break;
            }
        }
    };

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);

        String itemStr = ((FileListAdapter.ViewHolder) v.getTag()).filename
                .getText().toString();
        final File file = new File(mCurrentDir, itemStr);

        if (file.isDirectory()) {
            // Open the file folder
            mCurrentDir = new File(mCurrentDir.getPath() + "/" + itemStr);
            updateListView();

        } else {
            if (fileDisplay) {
                Uri uri = Uri.parse("file:" + file.getPath());
                Intent intent = new Intent();
                intent.setDataAndType(uri, getFileType(file));
                setResult(RESULT_OK, intent);
                finish();
            }
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
        case KeyEvent.KEYCODE_BACK:
            if (mCurrentDir == null || (mCurrentDir != null && mCurrentDir.getParentFile() == null))
                break;
            String str = mCurrentDir.getParentFile().getPath();
            String currentFileName = mCurrentDir.getName();
            if (!str.equals("/mnt")) {
                // Fix 215456 on 20130913 start
                String internalStoragePath = EnvironmentEx.getInternalStoragePath().getPath();//TCard
                //SPRD:Bug 255362, can back a invalid path when click back key @{
                String externalStoragePath = EnvironmentEx.getExternalStoragePath().getPath();
                if (internalStoragePath.equals(mCurrentDir.getPath()) || externalStoragePath.equals(mCurrentDir.getPath())) {// the root path of internal card
                    finish();
                    return true;
                }
                //@}
                mCurrentDir = new File(str);
                updateListView();

                int index = ((FileListAdapter) getListAdapter())
                        .getItemIndex(currentFileName);
                getListView().setSelection(index);

                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case MENU_ID_PATH_SAVE: {
            String absolutepathStr = mCurrentDir.getAbsolutePath() + "/";

            if(DBG) {Log.d(TAG, "onOptionsItemSelected absolutepathStr = " + absolutepathStr + " mBackup = " + mBackup);}

            Intent intent = new Intent(
                    "com.sprd.appbackup.SelectBackupPath");
            if (mBackup) {
                intent = new Intent("com.sprd.appbackup.SelectBackupPath");
            } else if (mRestore) {
                intent = new Intent("com.sprd.appbackup.SelectRestorePath");
            }
            Bundle bundle = new Bundle();
            bundle.putString("absolutepath", mCurrentDir.getAbsolutePath()
                    + "/");
            intent.putExtras(bundle);

            sendBroadcast(intent);

            finish();
            return true;
        }
        case MENU_ID_PATH_PROP: {
            finish();
            return true;
        }
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, MENU_ID_PATH_SAVE, 0, R.string.confirm).setIcon(
                android.R.drawable.ic_menu_add);
        menu.add(0, MENU_ID_PATH_PROP, 0, R.string.cancel).setIcon(
                android.R.drawable.ic_menu_add);
        return true;
    }

    public static String getFileType(File file) {
        String str = getFileExtendName(file.getPath());
        if (str == null) {
            return "filetype/null";
        }
        // ------------------------------------------
        if (str.equalsIgnoreCase("ppt") || str.equalsIgnoreCase("pps"))
            return "application/powerpoint";
        else if (str.equalsIgnoreCase("doc"))
            return "application/msword";
        else if (str.equalsIgnoreCase("pdf"))
            return "application/pdf";
        else if (str.equalsIgnoreCase("xls"))
            return "application/excel";
        // ------------------------------------------
        else if (str.equalsIgnoreCase("jpg") || str.equalsIgnoreCase("jpeg"))
            return "image/jpeg";
        else if (str.equalsIgnoreCase("bmp"))
            return "image/x-ms-bmp";
        else if (str.equalsIgnoreCase("wbmp"))
            return "image/vnd.wap.wbmp";
        else if (str.equalsIgnoreCase("png"))
            return "image/png";
        else if (str.equalsIgnoreCase("gif"))
            return "image/gif";
        // ------------------------------------------
        else if (str.equalsIgnoreCase("avi"))
            return "video/x-msvideo";
        else if (str.equalsIgnoreCase("wmv"))
            return "video/x-ms-wmv";
        else if (str.equalsIgnoreCase("mp4"))
            return "video/mp4";
        else if (str.equalsIgnoreCase("3gp"))
            return "video/3gpp";
        else if (str.equalsIgnoreCase("3g2"))
            return "video/3gpp2";
        // ------------------------------------------
        else if (str.equalsIgnoreCase("mp3"))
            return "audio/mpeg";
        else if (str.equalsIgnoreCase("amr"))
            return "audio/amr";
        else if (str.equalsIgnoreCase("mid") || str.equalsIgnoreCase("midi"))
            return "audio/midi";
        else if (str.equalsIgnoreCase("aac"))
            return "audio/aac";
        else if (str.equalsIgnoreCase("wav"))
            return "audio/x-wav";
        else if (str.equalsIgnoreCase("m4a"))
            return "audio/x-m4a";
        else if (str.equalsIgnoreCase("ogg"))
            return "audio/ogg";
        else if (str.equalsIgnoreCase("wma"))
            return "audio/wma";
        else if (str.equalsIgnoreCase("wmv"))
            return "video/wmv";
        else if (str.equalsIgnoreCase("asf"))
            return "video/asf";
        else if (str.equalsIgnoreCase("flv"))
            return "video/flv";
        else if (str.equalsIgnoreCase("flac"))
            return "audio/flac";
        else if (str.equalsIgnoreCase("ape"))
            return "audio/ape";
        // ------------------------------------------
        else if (str.equalsIgnoreCase("vcf"))
            return "text/x-vCard";
        else if (str.equalsIgnoreCase("vcs"))
            return "text/x-vCalendar";
        else if (str.equalsIgnoreCase("txt"))
            return "text/plain";
        else
            return "filetype/other";
    }

    public static String getFileExtendName(String filename) {
        int index = filename.lastIndexOf('.');
        return index == -1 ? null : filename.substring(index + 1);
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        mSaveSelectPathThread.shutdown();
        super.onDestroy();                
    }

}
