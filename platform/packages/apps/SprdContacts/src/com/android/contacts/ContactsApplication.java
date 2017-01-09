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

package com.android.contacts;

import android.app.Application;
import android.app.FragmentManager;
import android.app.LoaderManager;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.StrictMode;
import android.preference.PreferenceManager;
import android.provider.ContactsContract.Contacts;
import android.util.Log;

import com.android.contacts.common.ContactPhotoManager;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.testing.InjectedServices;
import com.android.contacts.common.util.Constants;
import com.android.contacts.commonbind.analytics.AnalyticsUtil;

import com.android.contacts.common.testing.NeededForTesting;
import com.google.common.annotations.VisibleForTesting;

/**
 * SPRD
 * @{
 */
import android.os.IBinder;
import android.content.Intent;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.app.NotificationManager;
import com.sprd.contacts.BatchOperationService;
import com.android.contacts.common.vcard.VCardService;
import com.sprd.contacts.appbackup.AppBackupService;
/**
 * @}
 */
@NeededForTesting
public class ContactsApplication extends Application {
    private static final boolean ENABLE_LOADER_LOG = false; // Don't submit with true
    private static final boolean ENABLE_FRAGMENT_LOG = false; // Don't submit with true

    private static InjectedServices sInjectedServices;
    /**
     * Log tag for enabling/disabling StrictMode violation log.
     * To enable: adb shell setprop log.tag.ContactsStrictMode DEBUG
     */
    public static final String STRICT_MODE_TAG = "ContactsStrictMode";
    private ContactPhotoManager mContactPhotoManager;
    private ContactListFilterController mContactListFilterController;
    /**
     * SPRD:
     * @{
     */
    private BatchOperationService mBatchOperationService;
    private VCardService mVcardService;
    private boolean isBatchBind;
    private boolean isVcardBind;
    private AppBackupService mBackupService;
    private boolean isBackupBind;

    public static ContactsApplication sApplication;

    public ContactsApplication() {
        sApplication = this;
    }
    /**
    * @}
    */

    /**
     * Overrides the system services with mocks for testing.
     */
    @VisibleForTesting
    public static void injectServices(InjectedServices services) {
        sInjectedServices = services;
    }

    public static InjectedServices getInjectedServices() {
        return sInjectedServices;
    }

    @Override
    public ContentResolver getContentResolver() {
        if (sInjectedServices != null) {
            ContentResolver resolver = sInjectedServices.getContentResolver();
            if (resolver != null) {
                return resolver;
            }
        }
        return super.getContentResolver();
    }

    @Override
    public SharedPreferences getSharedPreferences(String name, int mode) {
        if (sInjectedServices != null) {
            SharedPreferences prefs = sInjectedServices.getSharedPreferences();
            if (prefs != null) {
                return prefs;
            }
        }

        return super.getSharedPreferences(name, mode);
    }

    @Override
    public Object getSystemService(String name) {
        if (sInjectedServices != null) {
            Object service = sInjectedServices.getSystemService(name);
            if (service != null) {
                return service;
            }
        }

        return super.getSystemService(name);
    }

    @Override
    public void onCreate() {
        super.onCreate();

        if (Log.isLoggable(Constants.PERFORMANCE_TAG, Log.DEBUG)) {
            Log.d(Constants.PERFORMANCE_TAG, "ContactsApplication.onCreate start");
        }

        if (ENABLE_FRAGMENT_LOG) FragmentManager.enableDebugLogging(true);
        if (ENABLE_LOADER_LOG) LoaderManager.enableDebugLogging(true);

        if (Log.isLoggable(STRICT_MODE_TAG, Log.DEBUG)) {
            StrictMode.setThreadPolicy(
                    new StrictMode.ThreadPolicy.Builder().detectAll().penaltyLog().build());
        }

        // Perform the initialization that doesn't have to finish immediately.
        // We use an async task here just to avoid creating a new thread.
        (new DelayedInitializer()).execute();

        if (Log.isLoggable(Constants.PERFORMANCE_TAG, Log.DEBUG)) {
            Log.d(Constants.PERFORMANCE_TAG, "ContactsApplication.onCreate finish");
        }

        AnalyticsUtil.initialize(this);
    }

    private class DelayedInitializer extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... params) {
            final Context context = ContactsApplication.this;

            // Warm up the preferences and the contacts provider.  We delay initialization
            // of the account type manager because we may not have the contacts group permission
            // (and thus not have the get accounts permission).
            PreferenceManager.getDefaultSharedPreferences(context);
            getContentResolver().getType(ContentUris.withAppendedId(Contacts.CONTENT_URI, 1));
            /*
             * SPRD:
             *   Bug 273927
             *    Contacts optimized boot first start time.
             *
             * @orig
             *
             *
             * @{
             */
             NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
             notificationManager.cancelAll();
             bindBatchService();
             /*
             * @}
             */
            return null;
        }

        public void execute() {
            executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR,
                    (Void[]) null);
        }
    }
    /**
     * SPRD:
     * @{
     */
    private ServiceConnection mBatchOperationConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            mBatchOperationService = ((BatchOperationService.MyBinder) binder).getService();
            isBatchBind = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            isBatchBind = false;
        }
    };

    private ServiceConnection mVcardConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            mVcardService = ((VCardService.MyBinder) binder).getService();
            isVcardBind = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            isVcardBind = false;
        }
    };


    private ServiceConnection mBackupConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            mBackupService = ((AppBackupService.MyBinder) binder).getService();
            isBackupBind = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            isBackupBind = false;
        }
    };

    public void bindBatchService() {
        bindService(new Intent(this, BatchOperationService.class),
                mBatchOperationConnection, Context.BIND_AUTO_CREATE);
        bindService(new Intent(this, VCardService.class), mVcardConnection,
                Context.BIND_AUTO_CREATE);
        bindService(new Intent(this, AppBackupService.class), mBackupConnection,
                Context.BIND_AUTO_CREATE);
    }

    public boolean isBatchOperation() {
        return (mBatchOperationService != null && mBatchOperationService.isRunning())
                || (mVcardService != null && mVcardService.isRunning())
                || (mBackupService != null && mBackupService.isRunning());
    }
    /**
    * @}
    */
}
