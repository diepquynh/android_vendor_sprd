/*
 * Copyright (C) 2011 The Android Open Source Project
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


package com.android.exchange;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.content.pm.PackageManager;
import android.Manifest.permission;

import android.content.Intent;
import android.os.Trace;

import com.android.mail.utils.LogTag;
import com.android.mail.utils.LogUtils;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * the Exchange process can request access to a permission
 *
 */
public class EasPermissionRequestor extends Activity {
    private static final int PERMISSIONS_REQUEST_ALL_PERMISSIONS = 1;
    public static final String[] REQUIRED_PERMISSIONS = new String[]{
            permission.READ_EXTERNAL_STORAGE,
            permission.READ_CONTACTS,
            permission.READ_CALENDAR,
            permission.READ_PHONE_STATE,
            "com.android.email.permission.READ_ATTACHMENT",
    };

    protected static boolean hasPermissions(Context context, String[] permissions) {
        try {
            for (String permission : permissions) {
                if (context.checkSelfPermission(permission)
                        != PackageManager.PERMISSION_GRANTED) {
                    return false;
                }
            }
            return true;
        } finally {
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(hasPermissions(this,REQUIRED_PERMISSIONS)){
            setResult(RESULT_OK);
            finish();
        }else{
           if (savedInstanceState == null) {
               requestPermissions(); 
           }
        }
    }

    private void requestPermissions() {
        Trace.beginSection("requestPermissions");
        try {
            // Construct a list of missing permissions
            final ArrayList<String> unsatisfiedPermissions = new ArrayList<>();
            for (String permission : REQUIRED_PERMISSIONS) {
                if (checkSelfPermission(permission)
                        != PackageManager.PERMISSION_GRANTED) {
                    unsatisfiedPermissions.add(permission);
                }
            }
            /* SPRD: Add for bug552436 @{ */
            if (unsatisfiedPermissions.size() == 0) {
                LogUtils.d(LogTag.getLogTag(),"Request permission activity was called even"
                        + " though all permissions are satisfied.");
                onRequestPermissionsResult(PERMISSIONS_REQUEST_ALL_PERMISSIONS, new String[]{REQUIRED_PERMISSIONS[0]},
                        new int[]{PackageManager.PERMISSION_GRANTED});
                return;
            }
            /* }@ */
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
            setResult(RESULT_OK);
        } else {
            setResult(RESULT_CANCELED);
        }
        finish();
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
        return Arrays.asList(REQUIRED_PERMISSIONS).contains(p);
    }
}
