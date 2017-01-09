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

package com.sprd.contacts.activities;

import android.text.TextUtils;
import com.android.contacts.R;
import com.sprd.contacts.BatchOperationService;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

/**
 * The Activity for canceling vCard import/export.
 */
public class CancelBatchOperationActivity extends Activity {

    private class CancelListener
            implements DialogInterface.OnClickListener, DialogInterface.OnCancelListener {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            finish();
        }

        @Override
        public void onCancel(DialogInterface dialog) {
            finish();
        }
    }

    private final CancelListener mCancelListener = new CancelListener();
    private String mTag;
    private int mMode;
    private static Dialog mDialog;
    private static final String TAG = "CancelBatchOperationActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTag = getIntent().getStringExtra(BatchOperationService.KEY_NOTIFICATION_TAG);
        mMode = getIntent().getIntExtra(BatchOperationService.KEY_MODE, -1);
        if (TextUtils.isEmpty(mTag) || mMode == -1) {
            return;
        }
        if (mDialog != null) {
            mDialog.cancel();
        }
        showDialog(R.id.dialog_cancel_confirmation);
    }

    @Override
    protected Dialog onCreateDialog(int id, Bundle bundle) {

        switch (id) {
            case R.id.dialog_cancel_confirmation: {
                String message = null;
                if (mMode == BatchOperationService.MODE_START_BATCH_IMPORT_EXPORT) {
                    message = getString(R.string.cancel_batch_import_confirmation);
                } else if (mMode == BatchOperationService.MODE_START_BATCH_DELETE) {
                    message = getString(R.string.cancel_batch_delete_confirmation);
                } else if (mMode == BatchOperationService.MODE_START_BATCH_STARRED) {
                    message = getString(R.string.cancel_batch_star_confirmation);
                } else {
                    Log.e(TAG, "CancelBatchOperationActivity: unknown mode:" + mMode);
                    break;
                }

                final AlertDialog.Builder builder = new AlertDialog.Builder(this)
                        .setTitle(android.R.string.dialog_alert_title)
                        .setMessage(message)
                        .setPositiveButton(android.R.string.ok,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        final Intent intent = new Intent(
                                                CancelBatchOperationActivity.this,
                                                BatchOperationService.class);
                                        intent.putExtra(BatchOperationService.KEY_NOTIFICATION_TAG,
                                                mTag);
                                        intent.putExtra(BatchOperationService.KEY_MODE,
                                                BatchOperationService.MODE_CANCEL);
                                        startService(intent);
                                        finish();
                                    }
                                })
                        .setNegativeButton(android.R.string.cancel, mCancelListener)
                        .setOnCancelListener(mCancelListener);
                mDialog = builder.create();
                return mDialog;
            }
        }
        mDialog = super.onCreateDialog(id, bundle);
        return mDialog;
    }

}
