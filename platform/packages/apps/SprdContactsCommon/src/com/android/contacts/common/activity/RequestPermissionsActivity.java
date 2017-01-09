/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.contacts.common.activity;

import com.sprd.contacts.common.util.SystemProperties;
import com.android.contacts.common.R;

import android.Manifest.permission;
import android.app.Activity;
import android.content.Intent;
import android.widget.Toast;
import android.util.Log;
import android.app.ActivityManagerNative;

/**
 * Activity that requests permissions needed for activities exported from Contacts.
 */
public class RequestPermissionsActivity extends RequestPermissionsActivityBase {
	private static final String TAG = "RequestPermissionsActivity";

    private static final String EMAIL_PERMISSION_READ_ATTACHMENT = "com.android.email.permission.READ_ATTACHMENT";
    private static final String[] REQUIRED_PERMISSIONS = new String[]{
            // "Contacts" group. Without this permission, the Contacts app is useless.
            permission.READ_CONTACTS,
            permission.WRITE_CONTACTS,
            // "Phone" group. This is only used in a few places such as QuickContactActivity and
            // ImportExportDialogFragment. We could work around missing this permission with a bit
            // of work.
            permission.READ_CALL_LOG,
            /**
             *  SPRD:Bug 502652 adds the required permission READ_SMS.
             *  SPRD:Bug 505672 adds the required permission READ_PHONE_STATE.
             *  SPRD:Bug 507213 adds the required permission READ_ATTACHMENT and READ_EXTERNAL_STORAGE.
             *@{
             */
            permission.READ_SMS,
            permission.READ_PHONE_STATE,
            // EMAIL_PERMISSION_READ_ATTACHMENT,
            permission.READ_EXTERNAL_STORAGE,
            /*
             *@}
             **/
    };
    /**
    * SPRD: 511631 add the required permissions for GMS version.
     *
     * @{
     **/
    private static final String[] REQUIRED_PERMISSIONS_GMS = new String[]{
            // "Contacts" group. Without this permission, the Contacts app is useless.
            permission.READ_CONTACTS,
            // "Phone" group. This is only used in a few places such as QuickContactActivity and
            // ImportExportDialogFragment. We could work around missing this permission with a bit
            // of work.
            permission.READ_CALL_LOG,
            permission.READ_SMS,
            permission.READ_PHONE_STATE,
            permission.READ_EXTERNAL_STORAGE,
    };
    /*
     * @}
     */

    @Override
    protected String[] getRequiredPermissions() {
        /**
       * SPRD: 511631 add the required permissions for GMS version.
       * Original Android code:
         * @{
       return REQUIRED_PERMISSIONS;
         */
        /**
         * @{
         */
       if (!SystemProperties.get("ro.com.google.gmsversion").isEmpty()) {
          return REQUIRED_PERMISSIONS_GMS;
       } else {
          return REQUIRED_PERMISSIONS;
        }
       /**
        * @}
        */
    }

    @Override
    protected String[] getDesiredPermissions() {
        /**
       * SPRD: 511631 add the required permissions for GMS version.
       * Original Android code:
         * @{
       return new String[]{
                permission.ACCESS_FINE_LOCATION, // Location Group
                permission.READ_CONTACTS, // Contacts group
                permission.READ_CALL_LOG, // Permission group phone
                permission.READ_CALENDAR, // Calendar group
                permission.READ_SMS, // SMS group
        };
         */
        /**
         * @{
         */
       if (!SystemProperties.get("ro.com.google.gmsversion").isEmpty()) {
               return new String[]{
                      permission.ACCESS_FINE_LOCATION, // Location Group
                      permission.READ_CONTACTS, // Contacts group
                      permission.READ_CALL_LOG, // Permission group phone
                      permission.READ_PHONE_STATE, // Permission group phone
                      permission.READ_EXTERNAL_STORAGE,
                      permission.READ_CALENDAR, // Calendar group
                      permission.READ_SMS, // SMS group
                  };
       } else {
               return new String[]{
                      permission.ACCESS_FINE_LOCATION, // Location Group
                      permission.READ_CONTACTS, // Contacts group
                      permission.WRITE_CONTACTS,  // Contacts group
                      permission.READ_CALL_LOG, // Permission group phone
                          /**
                      *  SPRD:Bug 505672 adds the desired permission READ_PHONE_STATE.
                      *  SPRD:Bug 507213 adds the desired permission READ_ATTACHMENT and READ_EXTERNAL_STORAGE.
                           *@{
                           */
                      permission.READ_PHONE_STATE, // Permission group phone
                      // EMAIL_PERMISSION_READ_ATTACHMENT,
                      permission.READ_EXTERNAL_STORAGE,
                          /*
                           *@}
                           **/
                      permission.READ_CALENDAR, // Calendar group
                      permission.READ_SMS, // SMS group
                 };
      }
      /**
       * @}
       */
   }

    /**
     * SPRD: 610770 contacts occur crash when it is in LockTaskMode
     * @{
     */
    @Override
    public void onBackPressed() {
        if (isInLockTaskMode()) {
            startPermissionActivity(this);
        } else {
            super.onBackPressed();
        }
    }

    /** To determine whether the fixed screen mode**/
    public boolean isInLockTaskMode() {
        try {
            return ActivityManagerNative.getDefault().isInLockTaskMode();
        } catch (Exception e) {
            return false;
        }
    }
    /**
      * @}
      */

    public static boolean startPermissionActivity(Activity activity) {
         /**
        * SPRD: 511631 add the required permissions for GMS version.
        * Original Android code:
          * @{
        return startPermissionActivity(activity, REQUIRED_PERMISSIONS,
             RequestPermissionsActivity.class);
         */
        /**
         * @{
         */
       String[] requiredPermissions;
       Log.d(TAG,"ro.com.google.gmsversion is "
                   + SystemProperties.get("ro.com.google.gmsversion"));
       if (!SystemProperties.get("ro.com.google.gmsversion").isEmpty()) {
           requiredPermissions = REQUIRED_PERMISSIONS_GMS;
       } else {
           requiredPermissions = REQUIRED_PERMISSIONS;
         }
       return startPermissionActivity(activity, requiredPermissions,
             RequestPermissionsActivity.class);
       /**
        * @}
        */
    }

    /**
     * SPRD: Bug594617 The Contacts app stops running after closing permissons
     * of Contacts from editing screen and fixing the screen
     * @{
     */
    public static String[] requiredPermission() {
        return REQUIRED_PERMISSIONS;
    }
    /**
     * @}
     */
}
