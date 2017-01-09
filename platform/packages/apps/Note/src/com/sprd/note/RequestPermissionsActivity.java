package com.sprd.note;

import android.Manifest.permission;
import android.app.Activity;

public class RequestPermissionsActivity extends RequestPermissionsActivityBase {

    private static final String[] REQUIRED_PERMISSIONS = new String[] {
        permission.SEND_SMS,
        permission.CALL_PHONE,
        permission.READ_EXTERNAL_STORAGE };

    @Override
    protected String[] getRequiredPermissions() {
        return REQUIRED_PERMISSIONS;
    }

    @Override
    protected String[] getDesiredPermissions() {
        return new String[] {
                permission.SEND_SMS,
                permission.CALL_PHONE,
                permission.READ_EXTERNAL_STORAGE };
    }

    public static boolean startPermissionActivity(Activity activity) {
        return startPermissionActivity(activity, REQUIRED_PERMISSIONS,
                RequestPermissionsActivity.class);
    }
}

