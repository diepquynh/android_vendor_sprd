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

package plugin.sprd.limitownerinfolength;

import android.app.AddonManager;
import android.content.Context;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.widget.EditText;
import android.widget.Toast;
import android.util.Log;
import com.android.settings.R;

import com.android.settings.SetLimitOwnerInfoLength;
public class LimitOwnerInfoLength extends SetLimitOwnerInfoLength implements
        AddonManager.InitialCallback {

    public static String TAG = "LimitOwnerInfoLength";
    public static boolean DEBUG = true;
    private static final int TEXT_MAX_LENGTH = 50;

    public LimitOwnerInfoLength() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public boolean isSupport() {
        if (DEBUG) {
            Log.i(TAG, "isSupport");
        }
        return true;
    }

    @Override
    public void checkText(EditText mOwnerInfo, final Context context) {
        if (DEBUG) {
            Log.i(TAG, "checkText");
        }
        mOwnerInfo.setFilters(new InputFilter[]{new InputFilter.LengthFilter(TEXT_MAX_LENGTH)});
        mOwnerInfo.setSelection(mOwnerInfo.getText().length());
        mOwnerInfo.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (s.length() >= TEXT_MAX_LENGTH) {
                    Toast.makeText(context, R.string.name_too_long, Toast.LENGTH_SHORT).show();
                }
            }
        });
    }
}
