/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.fmradio.dialogs;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.nfc.Tag;
import android.os.Bundle;
import android.text.Editable;
import android.text.Selection;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.android.fmradio.FmUtils;
import com.android.fmradio.R;

/**
 * Edit favorite station name and frequency, caller should implement EditFavoriteListener
 */
public class FmFavoriteFreqEditDialog extends DialogFragment {
    private static final String STATION_NAME = "station_name";
    private static final String STATION_FREQ = "station_freq";
    private EditFavoriteFreqListener mListener = null;
    private EditText mStationFreqEditor = null;

    /**
     * Create edit favorite dialog instance, caller should implement edit favorite listener
     * 
     * @param stationName The station name
     * @param stationFreq The station frequency
     * @return edit favorite dialog
     */
    public static FmFavoriteFreqEditDialog newInstance(int stationFreq) {
        FmFavoriteFreqEditDialog fragment = new FmFavoriteFreqEditDialog();
        Bundle args = new Bundle(2);
        // args.putString(STATION_NAME, stationName);
        args.putInt(STATION_FREQ, stationFreq);
        fragment.setArguments(args);
        return fragment;
    }

    /**
     * Edit favorite listener
     */
    public interface EditFavoriteFreqListener {
        /**
         * Edit favorite station name and station frequency
         */
        void editFavoriteFreq(int oldFreq, int changeFreq);
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        try {
            mListener = (EditFavoriteFreqListener) activity;
        } catch (ClassCastException e) {
            e.printStackTrace();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String stationName = getArguments().getString(STATION_NAME);
        final int stationFreq = getArguments().getInt(STATION_FREQ);
        View v = View.inflate(getActivity(), R.layout.editstationfreq, null);
        mStationFreqEditor = (EditText) v.findViewById(
                R.id.dlg_edit_station_freq_text);

        if (null == stationName || "".equals(stationName.trim())) {
            stationName = "";
        }

        mStationFreqEditor.requestFocus();
        mStationFreqEditor.requestFocusFromTouch();
        // Edit
        mStationFreqEditor.setText(stationName);
        Editable text = mStationFreqEditor.getText();
        Selection.setSelection(text, text.length());
        return new AlertDialog.Builder(getActivity())
                // Must call setTitle here or the title will not be displayed.
                .setTitle(getString(R.string.change_frequency)).setView(v)
                .setPositiveButton(R.string.save,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                String strFreq = mStationFreqEditor.getText().toString();
                                Log.d("station", "strFreq=" + strFreq);
                                if (strFreq.length() == 0) {
                                    // showToast(getString(R.string.freq_input_info));
                                } else {
                                    if (strFreq.charAt(0) == '.') {
                                        strFreq = '0' + strFreq;
                                    }
                                    Float fFreq = Float.parseFloat(strFreq);
                                    int iFreq = (int) (fFreq * FmUtils.CONVERT_RATE);
                                    mListener.editFavoriteFreq(stationFreq, iFreq);
                                }
                            }
                        })
                .setNegativeButton(android.R.string.cancel, null)
                .create();
    }

    /**
     * Set the dialog edit text and other attribute.
     */
    @Override
    public void onResume() {
        super.onResume();
        setTextChangedCallback();
        String toName = mStationFreqEditor.getText().toString();
        // empty or blank or white space only name is not allowed
        toggleSaveButton(toName != null && TextUtils.getTrimmedLength(toName) > 0);
    }

    /**
     * This method register callback and set filter to Edit, in order to make sure that user input
     * is legal. The input can't be empty/blank/all-spaces filename
     */
    private void setTextChangedCallback() {
        mStationFreqEditor.addTextChangedListener(new TextWatcher() {
            // not use, so don't need to implement it
            @Override
            public void afterTextChanged(Editable arg0) {
            }

            // not use, so don't need to implement it
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            /**
             * check user input whether is null or all white space.
             */
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // empty or blank or white space only name is not allowed
                toggleSaveButton(TextUtils.getTrimmedLength(s) > 0);
            }
        });
    }

    /**
     * This method enables or disables save button to forbid renaming station name to null.
     * 
     * @param isEnabled true to enable save button, false to disable save button
     */
    private void toggleSaveButton(boolean isEnabled) {
        final AlertDialog dialog = (AlertDialog) getDialog();
        if (dialog == null) {
            return;
        }
        final Button button = dialog.getButton(DialogInterface.BUTTON_POSITIVE);
        button.setEnabled(isEnabled);
    }
}
