/*
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

package com.android.stk;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.android.internal.telephony.cat.CatLog;

/**
 * ListActivity used for displaying STK menus. These can be SET UP MENU and
 * SELECT ITEM menus. This activity is started multiple times with different
 * menu content.
 *
 */
public class StkMessageActivity extends Activity implements OnClickListener {
    private Context mContext;

    private TextView mMessageView;
    private String mMessageText = "";
    private Bitmap mIcon = null;
    private boolean mIconSelfExplanatory = false;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        // Remove the default title, customized one is used.
        requestWindowFeature(Window.FEATURE_LEFT_ICON);

        // Set the layout for this activity.
        setContentView(R.layout.stk_msg_dialog);
        mMessageView = (TextView) findViewById(R.id.dialog_message);

        mMessageText = StkApp.idleModeText;
        mIcon = StkApp.idleModeIcon;
        mIconSelfExplanatory = StkApp.idleModeIconSelfExplanatory;

        if (!(mIconSelfExplanatory && mIcon != null)) {
            mMessageView.setText(mMessageText);
        }

        Window window = getWindow();
        if (mIcon == null) {
            window.setFeatureDrawableResource(Window.FEATURE_LEFT_ICON,
                    com.android.internal.R.drawable.stat_notify_sim_toolkit);
        } else {
            window.setFeatureDrawable(Window.FEATURE_LEFT_ICON,
                    new BitmapDrawable(mIcon));
        }

        Button okButton = (Button)findViewById(R.id.button_ok);
        Button cancelButton = (Button) findViewById(R.id.button_cancel);

        okButton.setOnClickListener(this);
        cancelButton.setOnClickListener(this);
        mContext = getBaseContext();
    }
 
    @Override
    public void onClick(View v) {
        this.finish();
    }

    public void onStop() {
        super.onStop();
        this.finish();
    }
}


