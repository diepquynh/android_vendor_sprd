package addon.sprd.downloadprovider;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.content.Context;
import android.app.AddonManager;
import android.content.Intent;
import android.app.Activity;
import android.util.Log;
import android.content.ContentUris;
import android.content.DialogInterface;
import android.content.Intent;
import java.lang.Long;
import android.os.AsyncTask;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.drm.DrmStore.DrmObjectType;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import java.io.File;
import android.net.Uri;

//import addon.sprd.downloadprovider.R;
import addon.sprd.downloadprovider.DownloadProviderUtil;
import com.android.providers.downloads.DownloadInfo;
import android.app.ProgressDialog;
import addon.sprd.downloadprovider.DownloadDrmHelper.DrmClientOnErrorListener;
import android.widget.Toast;
import com.android.providers.downloads.ui.uiplugin.DownloaduiDRMUtils;

public class AddonDownloaduiDRMUtils extends DownloaduiDRMUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "AddonDownloaduiDRMUtils";
    private boolean mErrorDrm = false;
    private boolean mSuccess = false;

    public AddonDownloaduiDRMUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        Log.d(TAG, "-> onCreateAddon clazz: " + clazz);
        return clazz;
    }

    @Override
    public boolean isDRMDownloadSuccess(long id, Context uiContext){
        final Context context = uiContext;
        final Intent intent = DownloadDRMUltil.startPluginViewIntent(context, id, 0);
        Log.d(TAG, "AddonDownloaduiDRMUtils isDRMDownloadSuccess --- " + id);

        if (intent == null){
            Log.d(TAG, "AddonDownloaduiDRMUtils isDRMDownloadSuccess intent == null");
            Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_SHORT)
            .show();
            ((Activity)context).finish();
        }else{
           String filename = intent.getStringExtra("drmpath");
           String mimeType = intent.getStringExtra("drmtype");
           if (DownloadDRMUltil.isDrmType(context, filename, mimeType)){
               if (DownloadDRMUltil.isSupportDRMType(context, filename, mimeType)){
                   if (DownloadDRMUltil.isShowDrmInstallDilog(context, filename, mimeType)){
                       int type =  DownloadDRMUltil.getInstallDrmType(context, filename, mimeType);
                       installDrmSaveRights(type, context, filename, mimeType);
                   }
                   else if (DownloadDRMUltil.isDrmCDFile(context, filename, mimeType)){
                       String orignalType = DownloadDRMUltil.getOriginalMimeType(context, filename, mimeType);
                       Toast.makeText(context, mAddonContext.getString(R.string.drm_file_expired), Toast.LENGTH_SHORT)
                            .show();
                       ((Activity)context).finish();
                   }
                   else if (DownloadDRMUltil.isShowDrmConsumeDilog(context, filename, mimeType)){
                       AlertDialog.Builder builder = new AlertDialog.Builder(context, AlertDialog.THEME_DEVICE_DEFAULT_LIGHT);
                       builder.setTitle(mAddonContext.getString(R.string.title_drm_rights_consume))
                     .setMessage(mAddonContext.getString(R.string.message_drm_rights_consume))
                     .setPositiveButton(mAddonContext.getString(R.string.ok_drm_rights_consume), new DialogInterface.OnClickListener() {

                         @Override
                         public void onClick(DialogInterface dialog, int which) {
                             // TODO Auto-generated method stub
                             try {
                                 ((Activity)context).startActivity(intent);
                                 ((Activity)context).finish();
                             } catch (ActivityNotFoundException e) {
                                 Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_SHORT)
                                 .show();
                                 ((Activity)context).finish();
                             }
                         }
                     })
                     .setNegativeButton(mAddonContext.getString(R.string.cancel_drm_rights_consume), new DialogInterface.OnClickListener() {
                         @Override
                         public void onClick(DialogInterface dialog, int which) {
                             // TODO Auto-generated method stub
                             ((Activity)context).finish();
                         }
                     })
                    .setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            // TODO Auto-generated method stub
                            if (!((Activity)context).isFinishing()){
                                ((Activity)context).finish();
                            }
                        }
                    }).show();
                   }

                   else if (DownloadDRMUltil.isShowDrmRenewDilog(context, filename, mimeType)){
                     final String name = filename;
                     final String type = mimeType;
                     String drmName = null;
                     if (filename != null){
                         drmName = filename.substring(filename.lastIndexOf("/") + 1);
                     }
                     AlertDialog aDialog =new AlertDialog.Builder(context, AlertDialog.THEME_DEVICE_DEFAULT_LIGHT)
                     .setTitle(mAddonContext.getString(R.string.drm_file_renew_rights_title))
                     .setMessage(mAddonContext.getResources().getString(R.string.drm_file_renew_rights_message, drmName))
                     .setPositiveButton(mAddonContext.getString(R.string.drm_file_get_rights_renew),
                             new DialogInterface.OnClickListener() {

                         public void onClick(DialogInterface dialog, int which) {
                             try{
                                 String url =  DownloadDrmHelper.renewDrmRightsDownload(context, name, type);
                                 Log.i("DrmDownloadManager", "url -- "+url);
                                 boolean isCDfile = DownloadDrmHelper.isDrmCDFile(context, name, type);
                                 if (isCDfile && url == null){
                                     if (new File(name).exists()){
                                         boolean delete = new File(name).delete();
                                     }
                                     Intent intent = new Intent("com.android.downloads.DELETE_DOWNLOAD_INFORMATION");
                                     intent.putExtra("path", name);
                                     intent.putExtra("renew_drm", true);
                                     context.sendBroadcast(intent);
                                     return;
                                 }
                                 if (url == null && !isCDfile){
                                     Log.i("DrmDownloadManager", "url == null ");
                                     Toast.makeText(context, mAddonContext.getString(R.string.get_drm_rights_download_error), Toast.LENGTH_SHORT).show();
                                     ((Activity)context).finish();
                                     return;
                                 }
                                 Intent intent = new Intent(Intent.ACTION_VIEW);
                                 intent.setDataAndType(Uri.parse(url), "text/html");
                                 ((Activity)context).startActivity(intent);
                                 ((Activity)context).finish();
                             } catch (ActivityNotFoundException e) {
                                 Toast.makeText(context, mAddonContext.getString(R.string.rights_invalid), Toast.LENGTH_SHORT)
                                     .show();
                                 ((Activity)context).finish();
                             }
                         }
                     })
                     .setNegativeButton(mAddonContext.getString(R.string.get_drm_rights_download_cancel), new DialogInterface.OnClickListener() {
                         @Override
                         public void onClick(DialogInterface dialog, int which) {
                             // TODO Auto-generated method stub
                             ((Activity)context).finish();
                         }
                     })
                     .setOnDismissListener(new DialogInterface.OnDismissListener() {
                         @Override
                         public void onDismiss(DialogInterface dialog) {
                             // TODO Auto-generated method stub
                             if (!((Activity)context).isFinishing()){
                                 ((Activity)context).finish();
                             }
                         }
                     }).show();
                   }else{
                       try {
                           ((Activity)context).startActivity(intent);
                           ((Activity)context).finish();
                       } catch (ActivityNotFoundException e) {
                           Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_SHORT)
                           .show();
                           ((Activity)context).finish();
                       }
                   }
               }else{
                   Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_SHORT)
                   .show();
                   ((Activity)context).finish();
               }
           } else {
               try {
                   ((Activity)context).startActivity(intent);
                   ((Activity)context).finish();
               } catch (ActivityNotFoundException e) {
                   Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_SHORT)
                   .show();
                   ((Activity)context).finish();
               }
           }
        }
        return true;
    }

    private ProgressDialog mProgressDialog;

    private void installDrmSaveRights(final int drmType, final Context context, final String path, final String mimeType){
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
                    Toast.makeText(context, mAddonContext.getText(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
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
                            Toast.makeText(context, mAddonContext.getText(R.string.drm_rights_install_success), 1).show();
                        }else if (drmType == DrmStore.DrmObjectType.TRIGGER_OBJECT){
                            Toast.makeText(context, mAddonContext.getText(R.string.drm_rights_install_success_save), 1).show();
                        }
                    }else{
                        Toast.makeText(context, mAddonContext.getText(R.string.drm_rights_install_failed), Toast.LENGTH_SHORT).show();
                    }
                }
                ((Activity)context).finish();
            }

            @Override
            protected void onPreExecute() {
                // TODO Auto-generated method stub
                super.onPreExecute();
                CharSequence message = mAddonContext.getText(R.string.drm_file_processing);
                mProgressDialog = new ProgressDialog(context, ProgressDialog.THEME_DEVICE_DEFAULT_LIGHT);
                mProgressDialog.setMessage(message);
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
                if (result == DrmManagerClient.ERROR_NONE){
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
//    @Override
//    public void setTextColor
}
