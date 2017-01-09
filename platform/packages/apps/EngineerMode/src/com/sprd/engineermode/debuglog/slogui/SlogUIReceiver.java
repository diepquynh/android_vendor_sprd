/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
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

package com.sprd.engineermode.debuglog.slogui;

import static com.sprd.engineermode.debuglog.slogui.SlogAction.ACTION_CLEARLOG_COMPLETED;
import static com.sprd.engineermode.debuglog.slogui.SlogAction.ACTION_DUMPLOG_COMPLETED;
import static com.sprd.engineermode.debuglog.slogui.SlogAction.EXTRA_CLEAR_RESULT;
import static com.sprd.engineermode.debuglog.slogui.SlogAction.EXTRA_DUMP_RESULT;
import static com.sprd.engineermode.debuglog.slogui.SlogAction.SLOG_COMMAND_RETURN_OK;
import static com.sprd.engineermode.debuglog.slogui.SlogService.ACTION_SCREEN_SHOT;
import com.sprd.engineermode.debuglog.slogui.StorageUtil.StorageChangedListener;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.widget.Toast;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.view.WindowManager;

import com.sprd.engineermode.R;

public class SlogUIReceiver extends BroadcastReceiver {
    /**LOGPATHCHANGE mark sdcard state has changed*/
    public static final String LOGPATHCHANGE = "log_path_chage_to_internel";
    private SharedPreferences mSettings;
    private Context mContext;
    
    private StorageChangedListener mStorageChangedListener = new StorageChangedListener() {
        @Override
        public void onStorageChanged(String path, boolean available) {
            // when mCache is null, SlogAction.getState(SlogAction.STORAGEKEY)
            // return internel
            if (!mSettings.getBoolean(LOGPATHCHANGE, false) && !available) {
                mSettings.edit().putBoolean(LOGPATHCHANGE, true).commit();
                AlertDialog.Builder alert = new AlertDialog.Builder(mContext);
                alert.setMessage(R.string.log_path_chage_to_internel);
                alert.setCancelable(false);
                alert.setNeutralButton(R.string.alertdialog_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                            }
                        });
                AlertDialog changeInternel = alert.create();
                changeInternel.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                changeInternel.show();
                new Thread(new Runnable(){
                    @Override
                    public void run() {
                        SlogAction.reload();
                    }
                }).start();
            }
            /**mount proc*/
            if (mSettings.getBoolean(LOGPATHCHANGE, false) && available) {
                mSettings.edit().putBoolean(LOGPATHCHANGE, false).commit();
                AlertDialog.Builder alert = new AlertDialog.Builder(mContext);
                alert.setMessage(R.string.log_path_chage_to_externel);
                alert.setCancelable(false);
                alert.setNeutralButton(R.string.alertdialog_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                            }
                        });
                AlertDialog changeExternel = alert.create();
                changeExternel.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                changeExternel.show();
                new Thread(new Runnable(){
                    @Override
                    public void run() {
                        SlogAction.reload();
                    }
                }).start();
            }
        }
    };

    @Override
    public void onReceive(Context context, Intent intent) {
        if (context == null || intent == null) {
            return;
        }
        mContext = context.getApplicationContext();
        mSettings = mContext.getSharedPreferences("settings",
                Context.MODE_PRIVATE);
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            StorageUtil.setStorageChangeListener(mStorageChangedListener);
            context.getApplicationContext().startService(
                    new Intent(Intent.ACTION_BOOT_COMPLETED)
                            .setClass(context.getApplicationContext(),
                            SlogService.class));
            return;
        }
        if (ACTION_SCREEN_SHOT.equals(intent.getAction())) {
            SlogAction.snap(context.getApplicationContext());
            return;
        }
        if (ACTION_CLEARLOG_COMPLETED.equals(intent.getAction())) {
            boolean success = SLOG_COMMAND_RETURN_OK == intent.getIntExtra(
                    EXTRA_CLEAR_RESULT, -1);
            /* Toast.makeText(
                    context,
                    context.getString(success ? R.string.clear_action_successed
                            : R.string.clear_action_failed), Toast.LENGTH_LONG)
                    .show(); */
            return;
        }
        if (ACTION_DUMPLOG_COMPLETED.equals(intent.getAction())) {
            boolean success = SLOG_COMMAND_RETURN_OK == intent.getIntExtra(
                    EXTRA_DUMP_RESULT, -1);
            Toast.makeText(
                    context,
                    context.getString(success ? R.string.dump_action_successed
                            : R.string.dump_action_failed), Toast.LENGTH_LONG)
                    .show();
            return;
        }
    }

}
