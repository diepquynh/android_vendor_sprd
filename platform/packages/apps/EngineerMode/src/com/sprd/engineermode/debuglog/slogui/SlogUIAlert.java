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

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.util.SparseArray;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.sprd.engineermode.R;
import static com.sprd.engineermode.debuglog.slogui.SlogAction.DBG;

;

/**
 * The theme of the dialog could not handle all
 * 
 * @author Duke
 */
public class SlogUIAlert extends Activity {
    public interface AlertCallBack {
        /**
         * The call back when user touch the buttons
         * 
         * @param which , the id of the button
         */
        public void onClick(int which);

        /**
         * The call back when user put someting in edittext and commit them.
         * 
         * @param which
         * @param text
         */
        public void onTextAccept(int which, String text);
    }

    private static final String TAG = "SlogUIAlert";
    private static final String ALERT_EXTRA_TITLE = "title";
    private static final String ALERT_EXTRA_MESSAGE = "message";
    private static final String ALERT_EXTRA_MESSAGE_ADDTION =
            "message_addition";
    private static final String ALERT_EXTRA_ICON = "icon";
    private static final String ALERT_EXTRA_CANCELABLE = "cancel_able";
    private static final String ALERT_EXTRA_EDITTEXT = "edittext_accept";
    private static final String ALERT_EXTRA_POSITIVE = "positive_button";
    private static final String ALERT_EXTRA_NEGATIVE = "negative_button";
    private static final String ALERT_EXTRA_NEUTRAL = "neutral_button";
    private static final int MESSAGE_CLEAR_TAGS = -1;

    public static class AlertHandler extends Handler {
        private static SparseArray<Object> sTags;
        static {
            sTags = new SparseArray<Object>();
        }

        @Override
        public void handleMessage(Message msg) {
            if (DBG) {
                Log.d(TAG, "AlertHandler:handleMessage");
            }
            Object o = null;
            String accept = null;
            Bundle data = msg.getData();
            switch (msg.what) {
                case R.id.positive_button:
                    o = sTags.get(R.id.positive_button);
                    if (DBG)
                        Log.d(TAG, "positive button");
                    break;
                case R.id.negative_button:
                    o = sTags.get(R.id.negative_button);
                    if (DBG)
                        Log.d(TAG, "negative button");
                    break;
                case R.id.neutral_button:
                    o = sTags.get(R.id.neutral_button);
                    if (DBG)
                        Log.d(TAG, "neutral button");
                    break;
                case R.id.edit_text:
                    o = sTags.get(R.id.edit_text);
                    if (data != null) {
                        accept = data.getString(ALERT_EXTRA_EDITTEXT);
                    }
                    if (DBG)
                        Log.d(TAG, "neutral button");
                    break;
                case MESSAGE_CLEAR_TAGS:
                    sTags.clear();
                default:
                    break;
            }
            if (o != null && o instanceof AlertCallBack) {
                ((AlertCallBack) o).onClick(msg.what);
                if (accept != null) {
                    ((AlertCallBack) o).onTextAccept(msg.what, accept);

                }
            }
            // super.handleMessage(msg);
        }

        public static void setTag(int id, Object r) {
            sTags.put(id, r);
        }

        public static void clearTags() {
            sTags.clear();
        }
    }

    public static VirtualAlertBuilder prepareIntent() {
        return VirtualAlertBuilder.getInstance();
    }

    public static class VirtualAlertBuilder {

        private Intent mIntent = null;

        private static final VirtualAlertBuilder sBuilder = new VirtualAlertBuilder();

        public static final VirtualAlertBuilder getInstance() {
            sBuilder.mIntent = new Intent();
            return sBuilder;
        }

        public VirtualAlertBuilder setTitle(int resId) {
            mIntent.putExtra(ALERT_EXTRA_TITLE, resId);
            return this;
        }

        public VirtualAlertBuilder setIcon(int resId) {
            mIntent.putExtra(ALERT_EXTRA_ICON, resId);
            return this;
        }

        public VirtualAlertBuilder setMessage(int resId, String additionalString) {
            mIntent.putExtra(ALERT_EXTRA_MESSAGE, resId);
            mIntent.putExtra(ALERT_EXTRA_MESSAGE_ADDTION, additionalString);
            return this;
        }

        public VirtualAlertBuilder setPositiveButton(int resId,
                AlertCallBack callback) {
            mIntent.putExtra(ALERT_EXTRA_POSITIVE, resId);
            AlertHandler.setTag(R.id.positive_button, callback);
            return this;
        }

        public VirtualAlertBuilder setNegativeButton(int resId,
                AlertCallBack callback) {
            mIntent.putExtra(ALERT_EXTRA_NEGATIVE, resId);
            AlertHandler.setTag(R.id.negative_button, callback);
            return this;
        }

        public VirtualAlertBuilder setNeutralButton(int resId,
                AlertCallBack callback) {
            mIntent.putExtra(ALERT_EXTRA_NEUTRAL, resId);
            AlertHandler.setTag(R.id.neutral_button, callback);
            return this;
        }

        public VirtualAlertBuilder setEditTextAccepter(int promptResId,
                AlertCallBack callback) {
            mIntent.putExtra(ALERT_EXTRA_EDITTEXT, promptResId);
            AlertHandler.setTag(R.id.edit_text, callback);
            return this;
        }

        public VirtualAlertBuilder setCancelable(boolean enable) {
            mIntent.putExtra(ALERT_EXTRA_CANCELABLE, enable);
            return this;
        }

        public Intent generateIntent() {
            Intent intent = (Intent) mIntent.clone();
            mIntent = null;
            return intent;
        }

    }

    private static final void resolveIntent(SlogUIAlert activity) {
        Intent data = activity.getIntent();
        if (data == null) {
            Log.d(TAG, "getIntent is null, nothing to init, finish");
            activity.finish();
            return;
        }
        int res = 0;
        // inflate title
        res = data.getIntExtra(ALERT_EXTRA_TITLE, 0);
        if (res == 0) {
            activity.mTitle.setVisibility(View.GONE);
        } else {
            activity.mTitle.setText(activity.getString(res));
        }
        // inflate icon
        res = data.getIntExtra(ALERT_EXTRA_ICON, 0);
        if (res == 0) {
            activity.mIcon.setVisibility(View.GONE);
        } else {
            activity.mIcon.setImageResource(res);
        }
        // inflate message
        res = data.getIntExtra(ALERT_EXTRA_MESSAGE, 0);
        if (res == 0) {
            activity.mMessage.setVisibility(View.GONE);
        } else {
            String addition = data.getStringExtra(ALERT_EXTRA_MESSAGE_ADDTION);
            if (null != addition) {
                activity.mMessage.setText(activity.getString(res, addition));
            } else {
                activity.mMessage.setText(res);
            }
        }
        // inflate edit text
        res = data.getIntExtra(ALERT_EXTRA_EDITTEXT, 0);
        if (res == 0) {
            activity.mEditText.setVisibility(View.GONE);
        } else {
            activity.mMessage.setText(res);
            activity.mMessage.setVisibility(View.VISIBLE);
            activity.mEditText.setVisibility(View.VISIBLE);
            activity.mMessage.setText(res);
        }
        // inflate positive button
        res = data.getIntExtra(ALERT_EXTRA_POSITIVE, 0);
        if (res == 0) {
            activity.mPositiveButton.setVisibility(View.GONE);
        } else {
            activity.mPositiveButton.setVisibility(View.VISIBLE);
            activity.mPositiveButton.setText(res);
        }
        // inflate negative button
        res = data.getIntExtra(ALERT_EXTRA_NEGATIVE, 0);
        if (res == 0) {
            activity.mNegativeButton.setVisibility(View.GONE);
        } else {
            activity.mNegativeButton.setVisibility(View.VISIBLE);
            activity.mNegativeButton.setText(res);
        }
        // inflate neutral button
        res = data.getIntExtra(ALERT_EXTRA_NEUTRAL, 0);
        if (res == 0) {
            activity.mNeutralButton.setVisibility(View.GONE);
        } else {
            activity.mNeutralButton.setVisibility(View.VISIBLE);
            activity.mNeutralButton.setText(res);
        }

    }

    private AlertHandler mAlertHandler;
    private Button mPositiveButton;
    private Button mNeutralButton;
    private Button mNegativeButton;
    private EditText mEditText;
    private ImageView mIcon;
    private TextView mTitle;
    private TextView mMessage;

    // private boolean mExitEnable;

    private OnClickListener mClickListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.slogui_alertdialog_layout);

        mPositiveButton = (Button) findViewById(R.id.positive_button);
        mNeutralButton = (Button) findViewById(R.id.neutral_button);
        mNegativeButton = (Button) findViewById(R.id.negative_button);
        mEditText = (EditText) findViewById(R.id.edit_text);
        mIcon = (ImageView) findViewById(R.id.alert_icon);
        mTitle = (TextView) findViewById(R.id.alert_title);
        mMessage = (TextView) findViewById(R.id.alert_message);
        mAlertHandler = new AlertHandler();

        mClickListener = new OnClickListener() {

            @Override
            public void onClick(View v) {
                switch (v.getId()) {
                    case R.id.positive_button:
                        if (mEditText.getVisibility() == View.VISIBLE) {
                            Message msg = mAlertHandler.obtainMessage();
                            msg.what = R.id.edit_text;
                            Bundle data = new Bundle();
                            data.putString(ALERT_EXTRA_EDITTEXT, mEditText
                                    .getText().toString());
                            msg.setData(data);
                            mAlertHandler.sendMessage(msg);
                        } else {
                            mAlertHandler.sendEmptyMessage(v.getId());
                        }
                        break;
                    case R.id.negative_button:
                        mAlertHandler.sendEmptyMessage(R.id.negative_button);
                        break;
                    case R.id.neutral_button:
                        mAlertHandler.sendEmptyMessage(R.id.negative_button);
                        break;
                    default:
                        break;

                }
                finish();
            }

        };
        mPositiveButton.setOnClickListener(mClickListener);
        mNegativeButton.setOnClickListener(mClickListener);
        mNeutralButton.setOnClickListener(mClickListener);
        resolveIntent(this);

    }

    @Override
    protected void onDestroy() {
        mPositiveButton = null;
        mNeutralButton = null;
        mNegativeButton = null;
        mNegativeButton = null;
        mTitle = null;
        mMessage = null;
        mClickListener = null;
        super.onDestroy();
    }

    @Override
    public void finish() {
        mAlertHandler.sendEmptyMessage(MESSAGE_CLEAR_TAGS);
        Log.d(TAG, "finish");
        super.finish();
    }
}
