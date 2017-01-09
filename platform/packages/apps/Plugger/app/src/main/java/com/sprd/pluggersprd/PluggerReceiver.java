/*
 * Copyright (C) 2007 Esmertec AG.
 * Copyright (C) 2007 The Android Open Source Project
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

package com.sprd.pluggersprd;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class PluggerReceiver extends BroadcastReceiver {
    private static final String TAG = "PluggerReceiver";
    public static final String PLUGGER_SEND_MMS_NOSAVE = "android.intent.action.plugger.send_mms_nosave";
    
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.v(TAG,"PluggerReceiver: Action="+intent.getAction());
        if (PLUGGER_SEND_MMS_NOSAVE.equals(intent.getAction())) {
            Bundle extras = intent.getExtras();
            if (extras == null) {
                Log.e(TAG,"from PluggerActivity extra is null");
                return;
            }

            intent.putExtras(extras);
            Log.v(TAG,"receive pluggerActiciy broadcast action:"+intent.getAction()+" type:"+intent.getType()+" ready to start PluggerService");
            intent.setClass(context, PluggerIntentService.class);
            context.startService(intent);
       }else if("android.intent.action.SEND".equals(intent.getAction())){
            Bundle extras = intent.getExtras();
            if (extras == null) {
                Log.e(TAG,"from PluggerActivity extra is null");
                return;
            }
            intent.putExtras(extras);
            Log.v(TAG, "receive pluggerActiciy broadcast action:" + intent.getAction() + " type:" + intent.getType() + " ready to start PluggerService");
            intent.setClass(context, PluggerIntentService.class);
            context.startService(intent);
        }
    }
}
