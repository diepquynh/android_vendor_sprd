/*
 * Copyright (C) 2009 The Android Open Source Project
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
 * for download_storage_save_path
 */

package com.android.browser.preferences;


import com.android.browser.Controller;
import com.android.browser.StorageUtils;
import com.android.browser.R;
import android.os.SystemProperties;
import android.app.AlertDialog;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.storage.IMountService;
import android.preference.Preference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.preference.PreferenceScreen;
import android.database.Cursor;
import android.view.LayoutInflater;
import android.widget.Button;
import android.content.DialogInterface.OnDismissListener;
import java.io.File;
import android.content.ContentValues;
import android.app.Activity;
import android.Manifest;
import android.content.pm.PackageManager;
import android.database.sqlite.SQLiteException;
import android.widget.Toast;

public class SavePathPreference extends Preference {
    final static String TAG = "SavePathPreference";
    //String intentPathEditor = "com.android.browser.PATHEDIT";
    Context mContext;

    //private String internalPathState, externalPathState;
    public SavePathPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    public SavePathPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public SavePathPreference(Context context) {
        super(context);
        init(context);
    }

    private void init(Context context) {
        mContext = context;
        setLayoutResource(R.layout.savepath_preference_layout);
        updateSavePath();
    }

    public void updateSavePath(){
        if(!StorageUtils.isExternalStorageMounted() && !StorageUtils.isInternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()){
            setEnabled(false);
            setSummary("");
        }else{
            setEnabled(true);
            setSummary(getDownloadPath());
        }
    }

    private String getCurrentDownloadPathFromDB() {
        String tmpDbSavepath = "";
        Cursor cursor = null;
        try {
            String[] SAVEPATH_PROJECTION = new String[] {
                    "_id", "savepath"};
            cursor = mContext.getContentResolver().query(
                    Uri.parse("content://browser/filesavepath"),
                    SAVEPATH_PROJECTION, "", new String[]{}, null);
            if(null != cursor){
                if (cursor.moveToNext()) {
                    tmpDbSavepath = cursor.getString(1);
                }
            }
        } catch (SQLiteException e) {
            Log.e(TAG, "getCurrentDownloadPathFromDB:", e);
        } finally {
            if(null != cursor){
                cursor.close();
            }
        }

        return tmpDbSavepath;
    }

    private String getDownloadPath(){
        String tmpDbSavepath = getCurrentDownloadPathFromDB();
        if (!StorageUtils.checkStoragePathAvailable(tmpDbSavepath)){
              tmpDbSavepath = StorageUtils.getDefaultStoragePath();
        }
        return tmpDbSavepath;
    }

    private AlertDialog mSelectStorageDialog;

    protected void selectDownloadStorage(){
        showStorage(Controller.INTERNAL,Controller.SD,Controller.OTG);
    }

    private void showStorage(final String internal,final String sdCard,final String otg){
        if(!StorageUtils.isExternalStorageMounted() && !StorageUtils.isInternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()){
            Log.i(TAG, "showStorage, no available storage");
            Toast.makeText(mContext, R.string.no_storage, Toast.LENGTH_SHORT).show();
            return;
        }

        if (!StorageUtils.isExternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()) {
            selectPath(internal);
            return;
        }

        LayoutInflater factory = LayoutInflater.from(mContext);
        final View selectPathView = factory.inflate(R.layout.select_download_path_dialog, null);
        Button mBtnCancel = (Button)selectPathView.findViewById(R.id.select_path_cancel);
        Button mBtnInternal = (Button)selectPathView.findViewById(R.id.select_path_internal);
        Button mBtnSDCard = (Button)selectPathView.findViewById(R.id.select_path_sdcard);
        Button mBtnOTG = (Button)selectPathView.findViewById(R.id.select_path_otg);

        if (!StorageUtils.isExternalStorageMounted()) {
            mBtnSDCard.setVisibility(View.GONE);
        }
        if (!StorageUtils.isUsbdiskStorageMounted()) {
            mBtnOTG.setVisibility(View.GONE);
        }


        mBtnCancel.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                dismissSelectStorageDialog();
            }
        });
        mBtnInternal.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                selectPath(internal);
                dismissSelectStorageDialog();
            }
        });
        mBtnSDCard.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                selectPath(sdCard);
                dismissSelectStorageDialog();
            }
        });
        mBtnOTG.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                selectPath(otg);
                dismissSelectStorageDialog();
            }
        });

        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.select_storage_title);
        builder.setView(selectPathView);
        if (mSelectStorageDialog == null){
            mSelectStorageDialog = builder.create();
            mSelectStorageDialog.show();
        }

        mSelectStorageDialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mSelectStorageDialog = null;
            }
        });
    }

    private void dismissSelectStorageDialog(){
        if (mSelectStorageDialog != null){
            mSelectStorageDialog.dismiss();
            mSelectStorageDialog=null;
        }
    }

    private void selectPath(String storage){
        if(Controller.isSavePathEditorActivityStart){
            Log.e(TAG, storage +" has been ingoner");
            return;
        }else {
            Log.i(TAG, storage + " has been send");
            Controller.isSavePathEditorActivityStart = true;
            Intent editPath = new Intent();
            editPath.setClassName( "com.android.browser", "com.android.browser.SavePathEditor");
            Bundle mbundle = new Bundle();
            mbundle.putString("storage", storage);
            mbundle.putString("setdownload", "yes");
            editPath.putExtras(mbundle);
            Activity mActivity = null;
            //modify for download permission
            if (mContext instanceof Activity) {
                Log.i(TAG, "mContext instanceof Activity");
                mActivity = (Activity) mContext;
            }
            if (mActivity != null && mActivity.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG, "no storage permission, Controller.mPermissionObj = " + Controller.mPermissionObj);
                if (Controller.mPermissionObj == null) {
                    Controller.mPermissionObj = editPath;
                    Log.i(TAG, "requestPermissions storage permission");
                    mActivity.requestPermissions(
                            new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                            Controller.PERMISSIONS_REQUEST_STORAGE_READ_WRITE_SELECT_PATH);
                }
            } else {
                mContext.startActivity(editPath);
            }
        }
    }
}
