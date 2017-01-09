/*
 * Copyright (C) 2010 The Android Open Source Project
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

import java.io.File;

import android.R.bool;
import android.R.integer;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.DownloadManager;
import android.app.ProgressDialog;
import android.app.DownloadManager.Query;
import android.app.FragmentManager;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.res.Resources.Theme;
import android.database.Cursor;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.drm.DrmStore.DrmObjectType;
import android.net.Uri;
import android.nfc.Tag;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.provider.Downloads;
import com.android.providers.downloads.Constants;
import addon.sprd.downloadprovider.DownloadDrmHelper;
import addon.sprd.downloadprovider.DownloadDrmHelper.DrmClientOnErrorListener;
import addon.sprd.downloadprovider.DownloadDRMUltil;
import java.io.File;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import android.Manifest;
import android.widget.Toast;
import android.content.pm.PackageManager;
/**
 *  View showing a list of all downloads the Download Manager knows about.
 */
public class DownloadDRMList extends Activity {
    private Context  mContext;
    private String mPath = null;
    private String mFilename = null;
    private String mMimeType = null;
    private boolean mRenew = false;
    private Button mButtonOk;
    private Button mButtonCancel;
    private TextView mMessage;
    private TextView mTitle;
    private static final String TAG = "DownloadDRMList";
    static final int PERMISSIONS_REQUEST_STORAGE_READ_WRITE = 1;
    private boolean mErrorDrm = false;
    private boolean mSuccess = false;
    private boolean mProcessingRights = false;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        //        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
        //                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        Intent exintent = getIntent();
        mRenew = exintent.getBooleanExtra("isrenew", false);
        mPath = exintent.getStringExtra("filename");
        if (mPath != null && mPath.startsWith("file://")){
            mPath = mPath.substring(7);
        }
        if (mPath != null){
            int index = mPath.lastIndexOf("/");
            mFilename = mPath.substring(index+1);
        }
        mMimeType = exintent.getStringExtra("mimetype");
        Log.d(TAG, "externalFilename --- "+mPath);
        Log.d(TAG, "externalMimeType --- "+mMimeType);
        Log.d(TAG, "renew --- "+mRenew);
        /*
         * for documentui_DRM
         *@{
         */
        if (DownloadDRMUltil.isDrmEnabled()){
            Log.i(TAG, "Drm enable, check the storage permission");
            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG, "no storage permission, request it");
                    requestPermissions(
                            new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                            PERMISSIONS_REQUEST_STORAGE_READ_WRITE);
            } else {
               initprocess();
            }
        }
        /*@}*/
    }

    private void initprocess(){
      if (mRenew){
          if (DownloadDRMUltil.isDrmFLFileInValid(this, mPath, mMimeType)) {
              Log.d(TAG, "DownloadDRMUltil isDrmFLFileInValid --- "+mRenew);
              Toast.makeText(DownloadDRMList.this, getString(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
              finish();
              return;
          }

          if (DownloadDRMUltil.isDrmCDFile(this, mPath, mMimeType)){
              Log.d(TAG, "DownloadDRMUltil.isDrmCDFile --- "+mRenew);
              Toast.makeText(DownloadDRMList.this, getString(R.string.drm_file_expired), Toast.LENGTH_SHORT).show();
              finish();
              return;
          }
      }
      AlertDialog.Builder builder = new AlertDialog.Builder(this, AlertDialog.THEME_DEVICE_DEFAULT_LIGHT);
      AlertDialog dialog = builder.setTitle(showDialogTitle())
      .setMessage(showDialogMessage())
      .setPositiveButton(showDialogButtonRenew(), new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
              // TODO Auto-generated method stub
              processDrmFile();
          }
      })
      .setNegativeButton(R.string.cancel_drm_rights_consume, new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
              // TODO Auto-generated method stub
              DownloadDRMList.this.finish();
          }
      })
      .create();
      dialog.setOnDismissListener(new OnDismissListener() {
        @Override
        public void onDismiss(DialogInterface dialog) {
            // TODO Auto-generated method stub
            if (!DownloadDRMList.this.isFinishing() && !mProcessingRights){
                DownloadDRMList.this.finish();
                }
        }
    });

      dialog.show();
    }

    /*
     * for documentui_DRM
     *@{
     */
    @Override
    public void onRequestPermissionsResult(int requestCode,
            String permissions[], int[] grantResults) {
        if(requestCode == PERMISSIONS_REQUEST_STORAGE_READ_WRITE) {
            if (grantResults.length > 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG, "storage permission granted");
                initprocess();
            } else {
                Log.i(TAG, "storage permission denied");
                Toast.makeText(this, R.string.error_permissions, Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }
    /*@}*/

    private String showDialogTitle() {
        if (mRenew){
            return getResources().getString(R.string.drm_file_renew_rights_title);
        }else{
            if (mPath != null && mPath.endsWith(".dcf")){
                return  getResources().getString(R.string.title_drm_rights_consume);
            }else{
                return getResources().getString(R.string.drm_file_install_rights_title);
            }

        }
    }
    private String showDialogMessage() {
        if (mRenew){
            return getResources().getString(R.string.drm_file_renew_rights_message, mFilename);
        }else{
            if (mPath != null && mPath.endsWith(".dcf")){
                return getResources().getString(R.string.message_drm_rights_consume);
            }else{
                return getResources().getString(R.string.drm_file_install_rights_message, mFilename);
            }
        }
    }
    private String showDialogButtonRenew() {
        if (mRenew){
            return getResources().getString(R.string.drm_file_get_rights_renew);
        }else{
            return getResources().getString(R.string.drm_file_get_rights_install);
        }
    }

    private void processDrmFile(){
        mContext = this.getApplicationContext();
        if ((mPath == null) || (mMimeType == null)){
            Toast.makeText(DownloadDRMList.this, "path and mime type is null ", Toast.LENGTH_SHORT).show();
            DownloadDRMList.this.finish();
            return;

        }
        if (mRenew){
            if (DownloadDRMUltil.isShowDrmRenewDilog(mContext, mPath, mMimeType)){
                String url =  DownloadDrmHelper.renewDrmRightsDownload(mContext, mPath, mMimeType);
                boolean isCDfile = DownloadDrmHelper.isDrmCDFile(mContext, mPath, mMimeType);
                if (url == null && !isCDfile){
                    Log.i("DrmDownloadManager", "url == null ");
                    Toast.makeText(DownloadDRMList.this, getString(R.string.get_drm_rights_download_error), Toast.LENGTH_SHORT).show();
                    DownloadDRMList.this.finish();
                    return;
                }
                try {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.setDataAndType(Uri.parse(url), "text/html");
                    startActivity(intent);
                    DownloadDRMList.this.finish();
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(DownloadDRMList.this, getString(R.string.rights_invalid), Toast.LENGTH_SHORT).show();
                    DownloadDRMList.this.finish();
                    Log.e(TAG, "DownloadDRMList ActivityNotFoundException");
                }

            }else{
                Toast.makeText(DownloadDRMList.this, "Renew is true failed !", Toast.LENGTH_SHORT).show();
                DownloadDRMList.this.finish();
            }
        }else{
            if (DownloadDRMUltil.isShowDrmInstallDilog(mContext, mPath, mMimeType)){
                int type =  DownloadDRMUltil.getInstallDrmType(mContext, mPath, mMimeType);
                installDrmSaveRights(type, mContext, mPath, mMimeType);
            }else if (mPath != null && mPath.endsWith(".dcf")){
                String originnalMimetype = DownloadDrmHelper.getOriginalMimeType(mContext, mPath, mMimeType);
                Intent activityIntent = new Intent(Intent.ACTION_VIEW);
                activityIntent.setDataAndType(Uri.fromFile(new File(mPath)), originnalMimetype);
                activityIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                try {
                    mContext.startActivity(activityIntent);
                    finish();
                } catch (ActivityNotFoundException ex) {
                    Log.d(TAG,"processDrmFile::ActivityNotFoundException");
                    Toast.makeText(DownloadDRMList.this, R.string.download_no_application_title, Toast.LENGTH_SHORT)
                    .show();
                    finish();
                }
            }else {
                 Toast.makeText(DownloadDRMList.this, "Renew is false failed !", Toast.LENGTH_SHORT).show();
                 DownloadDRMList.this.finish();
            }
        }
    }

    private ProgressDialog mProgressDialog;

    private void installDrmSaveRights(final int drmType, final Context context, final String path, final String mimeType){
        mProcessingRights = true;
        String desFile = DownloadDrmHelper.getDRMTempDestinationPath(path);
        if (desFile == null) {
            Toast.makeText(DownloadDRMList.this, getApplicationContext().getText(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
            mProcessingRights = false;
            DownloadDRMList.this.finish();
            return;
        }
        File file = new File(desFile);
        if (file != null && file.exists()){
            Toast.makeText(DownloadDRMList.this, getApplicationContext().getText(R.string.drm_file_processing), Toast.LENGTH_SHORT).show();
            mProcessingRights = false;
            DownloadDRMList.this.finish();
            return;
        }
        DownloadDrmHelper.DrmClientOnErrorListener listener = new DrmClientOnErrorListener() {

            @Override
            public void onError() {
                // TODO Auto-generated method stub
                Log.i(TAG,"DownloadDrmList DrmClientOnErrorListener -> onError");
                mErrorDrm = true;
            }

            @Override
            public void onSuccess() {
                // TODO Auto-generated method stub
                Log.i(TAG,"DownloadDrmList DrmClientOnErrorListener -> onSuccess");
                mSuccess = true;
            }
        };
        DownloadDrmHelper.setDrmClientOnErrorListener(listener);

        new AsyncTask<Void, Void, Integer>() {
            int result = -1;
            @Override
            protected void onPostExecute(Integer result) {
                // TODO Auto-generated method stub
                super.onPostExecute(result);
                if (mProgressDialog != null){
                    if (mProgressDialog.isShowing()){
                        try {
                            mProgressDialog.dismiss();
                            mProgressDialog = null;
                        } catch (Exception e) {
                            Log.e("DownloadDRMList", "Exception   "+e);
                        }

                    }
                }
                if (mErrorDrm){
                    Toast.makeText(context, getApplicationContext().getText(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
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
                            Toast.makeText(context, getApplicationContext().getText(R.string.drm_rights_install_success), Toast.LENGTH_SHORT).show();
                        }else if (drmType == DrmStore.DrmObjectType.TRIGGER_OBJECT){
                            Toast.makeText(context, getApplicationContext().getText(R.string.drm_rights_install_success_save), Toast.LENGTH_SHORT).show();
                        }
                    }else{
                        Toast.makeText(context, getApplicationContext().getText(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
                    }
                }
                mProcessingRights = false;
                DownloadDRMList.this.finish();
            }

            @Override
            protected void onPreExecute() {
                // TODO Auto-generated method stub
                super.onPreExecute();
                CharSequence message = getApplicationContext().getText(R.string.drm_file_processing);
                mProgressDialog = new ProgressDialog(DownloadDRMList.this, ProgressDialog.THEME_DEVICE_DEFAULT_LIGHT);
                mProgressDialog.setMessage(message);
                mProgressDialog.setCanceledOnTouchOutside(false);
                mProgressDialog.show();
            }

            @Override
            protected Integer doInBackground(Void... params) {
                // TODO Auto-generated method stub
                long startTime = System.currentTimeMillis();
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
                long endTime = System.currentTimeMillis();
                if (endTime - startTime < 2000){
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
                return result;
            }

            @Override
            protected void onProgressUpdate(Void... values) {
                // TODO Auto-generated method stub
                super.onProgressUpdate(values);
            }
        }.execute();
    }

    private void setWaitProcessDrmInfo(){
        int count = 0;
        while(true){
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
                    if (count > 100){
                        break;
                    }
                    Thread.sleep(3000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private boolean queryDownloadStatusSuccess(Context context, String path){

        Cursor cursor = context.getContentResolver().query(Downloads.Impl.ALL_DOWNLOADS_CONTENT_URI,
                new String[]{Downloads.Impl.COLUMN_STATUS},
                Downloads.Impl._DATA + "=?",
                new String[] { path },
                null);

        if(cursor != null){
            if (cursor.getCount() > 0){
                if (cursor.moveToFirst()){
                    int statusColumn = cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_STATUS);
                    int status = cursor.getInt(statusColumn);
                    Log.i("processConvertDrmInfo", "queryDownloadStatusSuccess status  -  "+status);
                    if (status == Downloads.Impl.STATUS_SUCCESS){
                        if (cursor != null){
                            cursor.close();
                        }
                        return true;
                    }else{
                        if (cursor != null){
                            cursor.close();
                        }
                        return false;
                    }
                }
            }
        }else{
            return true;
        }

       if (cursor != null){
           cursor.close();
       }
       return true;
    }

    private String getDRMDestinationPath(String original){
        String destination = null;
        if (original != null){
            int dotIndex = original.lastIndexOf(".");
            if (dotIndex > 0){
                destination = original.substring(0, dotIndex) + ".dcf";
            }
        }
        return destination;
    }
}
