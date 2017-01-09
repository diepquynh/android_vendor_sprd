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

import android.Manifest.permission;
import android.app.Activity;
import android.util.Log;

/**
 * Activity that requests permissions needed for ImportVCardActivity.
 */
public class RequestImportVCardPermissionsActivity extends RequestPermissionsActivity {
	private static final String TAG = RequestImportVCardPermissionsActivity.class.getSimpleName();

    private static final String[] REQUIRED_PERMISSIONS = new String[] {
            permission.READ_CONTACTS,
            permission.READ_EXTERNAL_STORAGE,
            /*
             * SPRD: 496423 501064 Can not import .vcf file from Email and Messaging first time.
             * @{
             */
//            "com.android.messages.permission.READ_MESSAGES_ATTACHMENT",
            // "com.android.email.permission.READ_ATTACHMENT",
            /*
             * @}
             */
    };

    /**
    * SPRD: 511631 add the required permissions for GMS version.
     *
     * @{
     **/
   private static final String[] REQUIRED_PERMISSIONS_GMS = new String[] {
          permission.READ_CONTACTS,
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
        // Since this is used as an ostensible part of Dialer, lets be less pushy about asking for
        // unnecessary permissions here.
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

    /**
     * If any permissions the Contacts app needs are missing, open an Activity
     * to prompt the user for these permissions. Moreover, finish the current activity.
     *
     * This is designed to be called inside {@link android.app.Activity#onCreate}
     */
    public static boolean startPermissionActivity(Activity activity) {
        /**
       * SPRD: 511631 add the required permissions for GMS version.
       * Original Android code:
         * @{
       return startPermissionActivity(activity, REQUIRED_PERMISSIONS,
                RequestImportVCardPermissionsActivity.class);
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
             RequestImportVCardPermissionsActivity.class);
        /**
         * @}
         */
    }

    /**
     * SPRD: Bug 517203 The devices can not import vcard after closing and granting the required
     * permissions.
     * SPRD: Bug 516097 The DUT can not import vcard from email, showing I/O error.
     *
     * @{
     */
    public static String[] requiredPermission() {
        if (!SystemProperties.get("ro.com.google.gmsversion").isEmpty()) {
            return REQUIRED_PERMISSIONS_GMS;
        } else {
            return REQUIRED_PERMISSIONS;
        }
    }
    /*
     * @}
     */
}