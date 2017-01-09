/*
 * Copyright (C) 2013 The Android Open Source Project
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

package addon.sprd.downloadprovider;

import static android.app.DownloadManager.COLUMN_URI;

import java.io.File;

import android.R.bool;
import android.R.integer;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.DownloadManager;
import android.app.ProgressDialog;
import android.app.DownloadManager.Query;
import android.app.FragmentManager;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.drm.DrmStore.DrmObjectType;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.android.providers.downloads.Constants;
import addon.sprd.downloadprovider.DownloadDrmHelper;
import addon.sprd.downloadprovider.DownloadDrmHelper.DrmClientOnErrorListener;
import libcore.io.IoUtils;
import addon.sprd.downloadprovider.DownloadDRMUltil;
import addon.sprd.downloadprovider.R;

/**
 * Intercept all download clicks to provide special behavior. For example,
 * PackageInstaller really wants raw file paths.
 */
public class DownloadProviderUtil {
    private static final String LOGTAG = "DownloadProviderUIPlugin";
    private boolean mErrorDrm = false;
    private boolean mSuccess = false;
    private ProgressDialog mProgressDialog;

    public void installDrmSaveRights(final int drmType, final Context context, final String path, final String mimeType){

        Log.d(LOGTAG, "installDrmSaveRights: drmType" + drmType + "path = " + path + "mimeType = " + mimeType);

        DownloadDrmHelper.DrmClientOnErrorListener lis = new DrmClientOnErrorListener() {
            @Override
            public void onError() {
                // TODO Auto-generated method stub
                mErrorDrm = true;
            }
            @Override
            public void onSuccess() {
                // TODO Auto-generated method stub
                mSuccess = true;
            }
        };
        DownloadDrmHelper.setDrmClientOnErrorListener(lis);
        new AsyncTask<Void, Void, Integer>() {
            int result = -1;
            @Override
            protected void onPostExecute(Integer result) {
                // TODO Auto-generated method stub
                super.onPostExecute(result);
                if (mProgressDialog != null){
                    if (mProgressDialog.isShowing()){
                        mProgressDialog.dismiss();
                        mProgressDialog = null;
                    }
                }
                if (mErrorDrm){
                    Toast.makeText(context, context.getApplicationContext().getText(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
                    mErrorDrm = false;
                }else{
                    if (result == DrmManagerClient.ERROR_NONE){
                        if (drmType == DrmStore.DrmObjectType.RIGHTS_OBJECT){
                            if (new File(path).exists()){
                                boolean delete = new File(path).delete();
                            }
                            Intent intent = new Intent("com.android.downloads.DELETE_DOWNLOAD_INFORMATION");
                            intent.putExtra("path", path);
                            context.sendBroadcast(intent);
                            Toast.makeText(context, context.getApplicationContext().getText(R.string.drm_rights_install_success), Toast.LENGTH_LONG).show();
                        }else if (drmType == DrmStore.DrmObjectType.TRIGGER_OBJECT){
                            Toast.makeText(context, context.getApplicationContext().getText(R.string.drm_rights_install_success_save), Toast.LENGTH_LONG).show();
                        }
                    }
                    else if (result == -1){
                        Toast.makeText(context, "Development, please wait", Toast.LENGTH_SHORT).show();
                    }
                    else{
                        Toast.makeText(context, "An error has occurred", Toast.LENGTH_SHORT).show();
                    }
                }
                ((Activity)context).finish();
            }

            @Override
            protected void onPreExecute() {
                // TODO Auto-generated method stub
                super.onPreExecute();
                CharSequence message = context.getApplicationContext().getText(R.string.drm_file_processing);
                mProgressDialog = new ProgressDialog(context/*TrampolineActivity.this*/, ProgressDialog.THEME_DEVICE_DEFAULT_LIGHT);
                mProgressDialog.setMessage(message);
                mProgressDialog.show();
            }

            @Override
            protected Integer doInBackground(Void... params) {
                // TODO Auto-generated method stub
                if (drmType == DrmStore.DrmObjectType.CONTENT){
                    result = -1;
                }else if (drmType == DrmStore.DrmObjectType.RIGHTS_OBJECT){
                    result = DownloadDrmHelper.saveDrmObjectRights(context, path, mimeType);
                }else if (drmType == DrmStore.DrmObjectType.TRIGGER_OBJECT){
                    result = DownloadDrmHelper.processConvertDrmInfo(context, path, mimeType, null);
                    setWaitProcessDrmInfo();
                }else{
                    result = -2;
                }
                if (result == DrmManagerClient.ERROR_NONE){
                }
                int count = 0;
                while(true){
                    long endTime = System.currentTimeMillis();
                    if (mSuccess){
                        mSuccess = false;
                        break;
                    }else{
                        try {
                            count ++;
                            if (count > 40){
                                break;
                            }
                            Thread.sleep(3000);
                        } catch (InterruptedException e) {
                            // TODO Auto-generated catch block
                            e.printStackTrace();
                        }
                    }
                }
                return result;
            }
        }.execute();
    }

    private void setWaitProcessDrmInfo(){
        int count = 0;
        while(true){
            long endTime = System.currentTimeMillis();
            if (mErrorDrm){
                mErrorDrm = false;
                break;
            }
            if (mSuccess){
                mSuccess = false;
                break;
            }else{
                try {
                    count ++;
                    if (count > 40){
                        break;
                    }
                    Thread.sleep(3000);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }
    }
}
