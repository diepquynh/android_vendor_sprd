package com.sprd.providers.contacts;

import com.android.providers.contacts.R;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.Manifest.permission;
import android.os.Trace;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import android.os.Bundle;
import android.util.Log;

public class RequestProviderPermissionsActivity extends Activity {
    private static final int PERMISSIONS_REQUEST_ALL_PERMISSIONS = 1;
    private static final String[] REQUIRED_PERMISSIONS = new String[] {
            permission.READ_CONTACTS,
            permission.READ_PHONE_STATE,
    };
    private String origAction = "";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /**
         * SPRD:514856 The acore stops running after checking and unchecking screen pinning on the
         * Contacts Storage permission screen.
         * @{
         */
        boolean isPermissionSatisfied = false;
        for (String permission : REQUIRED_PERMISSIONS) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                isPermissionSatisfied = true;
                break;
            }
        }
        if (savedInstanceState == null && isPermissionSatisfied) {
            isPermissionSatisfied = false;
            requestPermissions();
        }
        origAction = getIntent().getStringExtra("origIntent");
    }

    private void requestPermissions() {
        Trace.beginSection("requestPermissions");
        try {
            // Construct a list of missing permissions
            final ArrayList<String> unsatisfiedPermissions = new ArrayList<>();
            for (String permission : getRequiredPermissions()) {
                if (checkSelfPermission(permission)
                        != PackageManager.PERMISSION_GRANTED) {
                    unsatisfiedPermissions.add(permission);
                }
            }
            if (unsatisfiedPermissions.size() == 0) {
                throw new RuntimeException("Request permission activity was called even"
                        + " though all permissions are satisfied.");
            }
            requestPermissions(
                    unsatisfiedPermissions.toArray(new String[unsatisfiedPermissions.size()]),
                    PERMISSIONS_REQUEST_ALL_PERMISSIONS);
        } finally {
            Trace.endSection();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[],
            int[] grantResults) {
        if (permissions != null && permissions.length > 0
                && isAllGranted(permissions, grantResults)) {
            Intent intent = new Intent();
            intent.setAction("com.android.contacts.provider.action.permission");
            intent.putExtra("origIntent",origAction);
            if(!isFinishing()){
                sendBroadcast(intent);
            }
        } else {
            Toast.makeText(this, R.string.missing_required_permission, Toast.LENGTH_SHORT).show();
        }
        finishAndRemoveTask();
    }

    private boolean isAllGranted(String permissions[], int[] grantResult) {
        for (int i = 0; i < permissions.length; i++) {
            if (grantResult[i] != PackageManager.PERMISSION_GRANTED
                    && isPermissionRequired(permissions[i])) {
                return false;
            }
        }
        return true;
    }

    private boolean isPermissionRequired(String p) {
        return Arrays.asList(getRequiredPermissions()).contains(p);
    }

    protected String[] getRequiredPermissions() {
        return REQUIRED_PERMISSIONS;
    }
}
