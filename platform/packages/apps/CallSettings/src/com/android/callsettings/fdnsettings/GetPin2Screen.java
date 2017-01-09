/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.callsettings.fdnsettings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextUtils;
import android.text.method.DigitsKeyListener;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

/* SPRD: function FDN support. @{ */
import android.telephony.TelephonyManager;
import android.widget.Toast;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import com.android.callsettings.CallSettingsActivityContainer;
import com.android.callsettings.R;
/* }@ */

/**
 * Pin2 entry screen.
 */
public class GetPin2Screen extends Activity implements TextView.OnEditorActionListener {
    private static final String LOG_TAG = "GetPin2Screen";

    private EditText mPin2Field;
    private Button mOkButton;
    /* SPRD: function FDN support. @{ */
    private TextView mTextView;
    /* }@ */
    /* SPRD: add for bug645817 @{ */
    private CallSettingsActivityContainer mActivityContainer;
    private static final String PHONE_ID = "phone_id";
    private static final int NO_VALID_PHONE_ID = -1;
    /* @} */

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.xml.get_pin2_screen);
        /* SPRD: function FDN support. @{ */
        Intent intent = getIntent();
        int times = intent.getIntExtra("times", 0);
        log("remain times:" + times);
        if (times > 0) {
            mTextView = (TextView) findViewById(R.id.prompt);
            CharSequence nstr = getString(R.string.promptWordOne) + times + getString(R.string.promptWordTwo);
            mTextView.setText(nstr);
        }
        /* }@ */
        mPin2Field = (EditText) findViewById(R.id.pin);
        mPin2Field.setKeyListener(DigitsKeyListener.getInstance());
        // SPRD: add for bug 602973
        mPin2Field.setFilters(new InputFilter[]{new InputFilter.LengthFilter(8)});

        /* SPRD: delete for function FDN support. @{
        ** ORIGIN
        mPin2Field.setMovementMethod(null);
        **/

        mPin2Field.setOnEditorActionListener(this);
        mPin2Field.setInputType(
                InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_VARIATION_PASSWORD);

        mOkButton = (Button) findViewById(R.id.ok);
        mOkButton.setOnClickListener(mClicked);
        /* SPRD: add for bug645817 @{ */
        int phoneId = intent.getIntExtra(PHONE_ID, NO_VALID_PHONE_ID);
        mActivityContainer = CallSettingsActivityContainer.getInstance();
        mActivityContainer.setApplication(getApplication());
        mActivityContainer.addActivity(this, phoneId);
        /* @} */
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private String getPin2() {
        return mPin2Field.getText().toString();
    }

    private void returnResult() {
        Bundle map = new Bundle();
        map.putString("pin2", getPin2());

        Intent intent = getIntent();
        Uri uri = intent.getData();

        Intent action = new Intent();
        if (uri != null) action.setAction(uri.toString());
        setResult(RESULT_OK, action.putExtras(map));
        finish();
    }

    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
            mOkButton.performClick();
            return true;
        }
        return false;
    }

    private final View.OnClickListener mClicked = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            /* SPRD: function FDN support. @{ */
            if (isAirplaneModeOn(getBaseContext())) {
                Toast.makeText(GetPin2Screen.this, R.string.airplane_changed_on, Toast.LENGTH_LONG).show();
                return;
            }
            /* }@ */
            if (TextUtils.isEmpty(mPin2Field.getText())) {
                return;
            }

            returnResult();
        }
    };

    private boolean isAirplaneModeOn(Context context) {
        if (context == null) {
            return true;
        }
        return Settings.System.getInt(context.getContentResolver(),
                Settings.System.AIRPLANE_MODE_ON, 0) != 0;
    }

    private void log(String msg) {
        Log.d(LOG_TAG, "[GetPin2] " + msg);
    }

    /* SPRD: add for bug645817 @{ */
    protected void onDestroy() {
        super.onDestroy();
        if (mActivityContainer != null) {
            mActivityContainer.removeActivity(this);
        }
    }
    /* @} */
}
