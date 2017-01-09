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

package com.android.contacts.common.util;

import android.content.Context;
import android.os.AsyncTask;
import android.telephony.PhoneNumberFormattingTextWatcher;
import android.util.Log;
import android.widget.TextView;

import com.android.contacts.common.GeoUtil;
import com.android.contacts.common.compat.PhoneNumberFormattingTextWatcherCompat;

public final class PhoneNumberFormatter {
    private PhoneNumberFormatter() {}

    /**
     * Load {@link TextWatcherLoadAsyncTask} in a worker thread and set it to a {@link TextView}.
     */
    private static class TextWatcherLoadAsyncTask extends
            AsyncTask<Void, Void, PhoneNumberFormattingTextWatcher> {
        private final String mCountryCode;
        private final TextView mTextView;
        private final boolean mFormatAfterWatcherSet;

        public TextWatcherLoadAsyncTask(
                String countryCode, TextView textView, boolean formatAfterWatcherSet) {
            mCountryCode = countryCode;
            mTextView = textView;
            mFormatAfterWatcherSet = formatAfterWatcherSet;
            /**
            * SPRD:
            *
            * @{
            */
            mKeeppAndw = false;
            /**
            * @}
            */
        }

        @Override
        protected PhoneNumberFormattingTextWatcher doInBackground(Void... params) {
            // SPRD: BUG606484 The DUT occurs anr while editoring contacts.
            try {
                return PhoneNumberFormattingTextWatcherCompat.newInstance(mCountryCode);
            } catch (OutOfMemoryError e) {
                Log.e("PhoneNumberFormatter", "OutOfMemoryError: " + e.getMessage());
                return null;
            }

        }

        @Override
        protected void onPostExecute(PhoneNumberFormattingTextWatcher watcher) {
            if (watcher == null || isCancelled()) {
                return; // May happen if we cancel the task.
            }

            // Setting a text changed listener is safe even after the view is detached.
            mTextView.addTextChangedListener(watcher);

            // Forcing formatting the existing phone number
            if (mFormatAfterWatcherSet) {
                watcher.afterTextChanged(mTextView.getEditableText());
            }
        }
        /**
        * SPRD:
        *
        * @{
        */
        private final boolean mKeeppAndw;
        /**
        * @}
        */
    }

    /**
     * Delay-set {@link PhoneNumberFormattingTextWatcher} to a {@link TextView}.
     */
    public static final void setPhoneNumberFormattingTextWatcher(Context context,
            TextView textView) {
        setPhoneNumberFormattingTextWatcher(context, textView,
                /* formatAfterWatcherSet =*/ false);
    }

    /**
    * SPRD:
    *
    * @{
    */
    public static final void setPhoneNumberFormattingTextWatcher(Context context,
            TextView textView, boolean isKeeppAndw) {
        new TextWatcherLoadAsyncTask(GeoUtil.getCurrentCountryIso(context), textView,
                isKeeppAndw)
                .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, (Void[]) null);
    }
    /**
    * @}
    */
}
