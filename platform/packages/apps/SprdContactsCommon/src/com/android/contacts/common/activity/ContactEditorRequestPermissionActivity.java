package com.android.contacts.common.activity;

import java.util.Arrays;

import android.Manifest.permission;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;
import com.android.contacts.common.R;

public class ContactEditorRequestPermissionActivity extends RequestPermissionsActivity {
    private static final String TAG = "ContactEditorRequestPermissionActivity";

    private static final String[] REQUIRED_PERMISSIONS = new String[]{
        permission.READ_CONTACTS,
        permission.WRITE_CONTACTS,
        permission.READ_EXTERNAL_STORAGE,
        permission.READ_PHONE_STATE
    };

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[],
            int[] grantResults) {
        int lockTask = Settings.Global.getInt(getContentResolver(), "lock_task", 0);
        Log.d(TAG, "ContactEditorRequestPermissionActivity onRequestPermissionsResult lockTask : " + lockTask);
        if (lockTask == 1) {
            if (permissions == null ||  permissions.length < 0
                    || !isAllGranted(permissions, grantResults)) {
                Toast.makeText(this, R.string.missing_required_permission, Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    /**
     * sprd Bug522250 during group editor interface, close Contacts permissions, back, no
     * permission dialog or toast
     * @{
     */
    public static String[] requiredPermission () {
        return REQUIRED_PERMISSIONS;
    }
    /**
     * @}
     */

    public static boolean startPermissionActivity(Activity activity) {
        return startPermissionActivity(activity, REQUIRED_PERMISSIONS,
                ContactEditorRequestPermissionActivity.class);
    }
}
