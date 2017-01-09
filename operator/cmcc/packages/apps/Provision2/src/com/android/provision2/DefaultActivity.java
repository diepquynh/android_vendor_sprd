/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.provision2;

import android.app.Activity;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.content.Intent;
import android.util.Log;

/**
 * Application that sets the provisioned bit, like SetupWizard does.
 */
public class DefaultActivity extends Activity implements OnClickListener {
    protected Button mStatementButton;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.activity_main);

        mStatementButton = (Button) findViewById(R.id.confirm);
        mStatementButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if (v == mStatementButton) {
            // Add a persistent setting to allow other apps to know the device has been provisioned.
            Settings.Global.putInt(getContentResolver(), Settings.Global.DEVICE_PROVISIONED, 1);
            Settings.Secure.putInt(getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE, 1);

            // remove this activity from the package manager.
            PackageManager pm = getPackageManager();
            ComponentName name = new ComponentName(this, DefaultActivity.class);
            pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);

            choose_secure();

            // terminate the activity.
            finish();
        }
    }

    private void choose_secure() {
        String secureCheck = SystemProperties.get("persist.support.securetest", "0");
        Log.d("DefaultActivity", "need check secure flag is:" + secureCheck);
        if (secureCheck.equals("1")) {
            Intent intent = new Intent("ACTION_SPR_SECURE_CHOOSE");
            sendBroadcast(intent);
        } else {
            Log.d("DefaultActivity", "After start up needen't slect security configuration");
        }
    }

    @Override
    public void onBackPressed() {
        return;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        switch (event.getKeyCode()) {
        case KeyEvent.KEYCODE_CALL:
            return true;
        default:
            break;
        }
        return super.dispatchKeyEvent(event);
    }
}