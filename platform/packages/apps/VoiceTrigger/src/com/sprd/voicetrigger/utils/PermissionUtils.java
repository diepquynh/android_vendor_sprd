package com.sprd.voicetrigger;

import android.Manifest;
import android.app.Activity;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.util.Log;
import android.content.pm.PackageManager;
import android.view.KeyEvent;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;



public class PermissionUtils{
    private static final String TAG = "PermissionUtils";
    public static final int VOICETRIGGERMAIN_PERMISSIONS_REQUEST_CODE = 200;
    public static final int HELLOTRIGGER_PERMISSIONS_REQUEST_CODE = 300;

    private AlertDialog mErrorPermissionsDialog;

    public static boolean checkAndBuildPermissions(Context context, int requestCode ) {
        Log.d(TAG, "checkAndBuildPermissions");
        if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) {
            return false;
        }
       String[] permissionsToRequest = new String[1];
       permissionsToRequest[0] = Manifest.permission.RECORD_AUDIO;

      if (requestCode == VOICETRIGGERMAIN_PERMISSIONS_REQUEST_CODE){
          ((VoiceTriggerMainActivity)context).requestPermissions(permissionsToRequest, requestCode);
      } else if  (requestCode == HELLOTRIGGER_PERMISSIONS_REQUEST_CODE) {
          ((HelloTriggerActivity)context).requestPermissions(permissionsToRequest, requestCode);
      }
       return true;
   }


    public static  boolean requestPermissionsResult(final int requestCode,
            String permissions[], int[] grantResults ,final Context context ) {
            AlertDialog dialog = null;
            Log.d(TAG, "onRequestPermissionsResult");
            boolean mNeedRequestPermissions = true;
            boolean resultsAllGranted = true;
            if (grantResults.length > 0) {
                for (int result : grantResults) {
                    if (PackageManager.PERMISSION_GRANTED != result) {
                        resultsAllGranted = false;
                    }
                }
            } else {
                resultsAllGranted = false;
            }
            if (resultsAllGranted) {
                mNeedRequestPermissions = false;
            } else {
                if (dialog != null){
                    dialog.dismiss();
                    dialog = null;
                }
                dialog  = new AlertDialog.Builder(context)
                        .setTitle(context.getResources().getString(R.string.error_app_internal))
                        .setMessage(context.getResources().getString(R.string.error_permissions))
                        .setCancelable(false)
                        .setOnKeyListener(new Dialog.OnKeyListener() {
                            @Override
                            public boolean onKey(DialogInterface dialog, int keyCode,
                                    KeyEvent event) {
                                if (keyCode == KeyEvent.KEYCODE_BACK) {
                                    if (requestCode == VOICETRIGGERMAIN_PERMISSIONS_REQUEST_CODE){
                                        ((VoiceTriggerMainActivity)context).finish();
                                    } else if  (requestCode == HELLOTRIGGER_PERMISSIONS_REQUEST_CODE) {
                                        ((HelloTriggerActivity)context).finish();
                                    }
                                }
                                return true;
                            }
                        })
                        .setPositiveButton(context.getResources().getString(R.string.dialog_dismiss),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        if (requestCode == VOICETRIGGERMAIN_PERMISSIONS_REQUEST_CODE){
                                            ((VoiceTriggerMainActivity)context).finish();
                                        } else if  (requestCode == HELLOTRIGGER_PERMISSIONS_REQUEST_CODE) {
                                            ((HelloTriggerActivity)context).finish();
                                        }
                                    }
                                })
                        .show();
            }
            return mNeedRequestPermissions;
        }

}