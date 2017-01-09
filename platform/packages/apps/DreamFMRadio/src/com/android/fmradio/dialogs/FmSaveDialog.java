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

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.android.fmradio.FmRecorder;
import com.android.fmradio.FmService;
import com.android.fmradio.R;
import com.android.fmradio.FmUtils;

import java.io.File;

/**
 * SPRD:
 *
 * @{
 */
import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;

/**
 * @}
 */

/**
 * The dialog fragment for save recording file
 */
public class FmSaveDialog extends DialogFragment {
    private static final String TAG = "FmSaveDialog";

    // save recording file button
    private Button mButtonSave = null;
    // discard recording file button
    private Button mButtonDiscard = null;
    // rename recording file edit text
    private EditText mRecordingNameEditText = null;
    // recording file default name
    private String mDefaultRecordingName = null;
    // recording file to save name
    private String mRecordingNameToSave = null;
    private OnRecordingDialogClickListener mListener = null;

    // The default filename need't to check whether exist
    private boolean mIsNeedCheckFilenameExist = false;
    // record sd card path when start recording
    private String mRecordingSdcard = null;

    private String mRecordingFileName = null;

    private String mTempRecordingName = null;

    /**
     * SPRD: bug509670, recording time is too short, causing music player anr.
     * 
     * @{
     */
    private long mRecordingTime;

    /**
     * @}
     */

    /**
     * FM record dialog fragment, because fragment manager need empty constructor to instantiated
     * this dialog fragment when configuration change
     */
    public FmSaveDialog() {
    }

    /**
     * FM record dialog fragment according name, should pass value recording file name
     * 
     * @param defaultName The default file name in FileSystem
     * @param recordingName The name in the dialog for show and save
     */
    public FmSaveDialog(String sdcard, String defaultName, String recordingName) {
        mRecordingSdcard = sdcard;

        /**
         * SPRD: bug474741, recording format selection. Original Android code: mTempRecordingName =
         * defaultName + FmRecorder.RECORDING_FILE_EXTENSION;
         * 
         * @{
         */
        if (FmRecorder.GLOBAL_RECORD_FORMAT_FLAG == 1) {
            mTempRecordingName = "." + defaultName + FmRecorder.RECORDING_FILE_EXTENSION + ".tmp";
        } else {
            mTempRecordingName = "." + defaultName + FmRecorder.RECORDING_FILE_EXTENSION_AMR + ".tmp";
        }
        /**
         * @}
         */

        mDefaultRecordingName = recordingName;
        mRecordingNameToSave = recordingName;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        try {
            mListener = (OnRecordingDialogClickListener) activity;
        } catch (ClassCastException e) {
            e.printStackTrace();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        /**
         * SPRD: bug509670, recording time is too short, causing music player anr.
         * 
         * @{
         */
        Bundle recordingData = getArguments();
        if (recordingData != null) {
            mRecordingTime = recordingData.getLong(FmRecorder.FM_RECORDING_TIME_KEY);
        }
        /**
         * @}
         */

        if (savedInstanceState != null) {
            mRecordingNameToSave = savedInstanceState.getString("record_file_name");
            mDefaultRecordingName = savedInstanceState.getString("record_default_name");
            mRecordingSdcard = FmService.getRecordingSdcard();
            /**
             * SPRD: bug502693, NullPointerException occurs during monkey running.
             * 
             * @{
             */
            mTempRecordingName = savedInstanceState.getString("record_temp_name");
            /**
             * @}
             */
            if (savedInstanceState.containsKey(FmRecorder.FM_RECORDING_TIME_KEY)) {
                mRecordingTime = savedInstanceState.getLong(FmRecorder.FM_RECORDING_TIME_KEY);
            }
        }
        setStyle(STYLE_NO_TITLE, 0);
        View view = getActivity().getLayoutInflater().inflate(R.layout.save_dialog, null);
        mButtonSave = (Button) view.findViewById(R.id.save_dialog_button_save);
        mButtonSave.setOnClickListener(mButtonOnClickListener);

        mButtonDiscard = (Button) view.findViewById(R.id.save_dialog_button_discard);
        mButtonDiscard.setOnClickListener(mButtonOnClickListener);

        // Set the recording edit text
        mRecordingNameEditText = (EditText) view.findViewById(R.id.save_dialog_edittext);
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity()).setView(view);
        return builder.create();
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        if (mListener != null) {
            mListener.onRecordingDialogClick(mRecordingFileName);
            mListener = null;
        }
    }

    /**
     * Set the dialog edit text and other attribute
     */
    @Override
    public void onResume() {
        super.onResume();
        // have define in fm_recorder_dialog.xml length at most
        // 250(maxFileLength - suffixLength)
        if (mDefaultRecordingName != null) {
            if (null != mRecordingNameToSave) {
                // this case just for,fragment recreate
                mRecordingNameEditText.setText(mRecordingNameToSave);
                if ("".equals(mRecordingNameToSave)) {
                    mButtonSave.setEnabled(false);
                }
            } else {
                mRecordingNameEditText.setText(mDefaultRecordingName);
            }
        }

        mRecordingNameEditText.requestFocus();
        setTextChangedCallback();
        Dialog dialog = getDialog();
        dialog.setCanceledOnTouchOutside(false);
        dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE
                | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        outState.putString("record_file_name", mRecordingNameToSave);
        outState.putString("record_default_name", mDefaultRecordingName);
        /**
         * SPRD: bug502693, NullPointerException occurs during monkey running.
         * 
         * @{
         */
        outState.putString("record_temp_name", mTempRecordingName);
        /**
         * @}
         */
        outState.putLong(FmRecorder.FM_RECORDING_TIME_KEY, mRecordingTime);
        super.onSaveInstanceState(outState);
    }

    /**
     * This method register callback and set filter to Edit, in order to make sure that user input
     * is legal. The input can't be illegal filename and can't be too long.
     */
    private void setTextChangedCallback() {
        mRecordingNameEditText.addTextChangedListener(new TextWatcher() {
            // not use, so don't need to implement it
            @Override
            public void afterTextChanged(Editable arg0) {
            }

            // not use, so don't need to implement it
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            /**
             * check user input whether include invalid character
             */
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Filename changed, so we should check the new filename is
                // whether exist.
                mIsNeedCheckFilenameExist = true;
                String recordName = s.toString().trim();
                // Characters not allowed by file system
                if (recordName.length() <= 0
                        || recordName.startsWith(".")
                        || recordName.matches(".*[/\\\\:*?\"<>|\t].*")) {
                    mButtonSave.setEnabled(false);
                } else {
                    mButtonSave.setEnabled(true);
                }

                mRecordingNameToSave = mRecordingNameEditText.getText().toString().trim();
            }
        });
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
            String permissions[], int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult");
        switch (requestCode) {
            case FM_PERMISSIONS_REQUEST_CODE: {
                boolean resultsAllGranted = true;
                if (grantResults.length > 0) {
                    for (int result : grantResults) {
                        if (PackageManager.PERMISSION_GRANTED != result) {
                            resultsAllGranted = false;
                        }
                    }
                } else {
                    resultsAllGranted = false;
                }
                if (resultsAllGranted) {
                    onClickSave();
                } else {
                    onClickDiscard();
                }
            }
            default:
                break;
        }
    }

    private OnClickListener mButtonOnClickListener = new OnClickListener() {
        /**
         * Define the button operation
         */
        @Override
        public void onClick(View v) {

            File recordingFolderPath = new File(mRecordingSdcard, FmRecorder.FM_RECORD_FOLDER);

            switch (v.getId()) {
                case R.id.save_dialog_button_save:
                    if (v.getContext().checkSelfPermission(
                            Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                        requestPermissions(new String[] {
                            Manifest.permission.WRITE_EXTERNAL_STORAGE
                        }, FM_PERMISSIONS_REQUEST_CODE);
                        return;
                    }
                    onClickSave();
                    break;

                case R.id.save_dialog_button_discard:
                    onClickDiscard();
                    break;

                default:
                    break;
            }
        }
    };

    /**
     * The listener for click Save or Discard
     */
    public interface OnRecordingDialogClickListener {
        /**
         * Record dialog click callback
         * 
         * @param recordingName The user input recording name
         */
        void onRecordingDialogClick(String recordingName);
    }

    private static final int FM_PERMISSIONS_REQUEST_CODE = 200;

    private void onClickDiscard() {
        File recordingFolderPath = new File(mRecordingSdcard, FmRecorder.FM_RECORD_FOLDER);
        dismissAllowingStateLoss();
        /**
         * SPRD: bug502693, NullPointerException occurs during monkey running.
         * 
         * @{
         */
        if (mTempRecordingName == null) {
            return;
        }
        /**
         * @}
         */
        // here need delete discarded recording file
        File needToDelete = new File(recordingFolderPath, mTempRecordingName);
        if (needToDelete.exists()) {
            needToDelete.delete();
        }
        if (FmUtils.isAirplane(getActivity())) {
            getActivity().finish();
        }
    }

    private void onClickSave() {
        File recordingFolderPath = new File(mRecordingSdcard, FmRecorder.FM_RECORD_FOLDER);
        /**
         * SPRD: bug509670, recording time is too short, causing music player anr.
         * 
         * @{
         */
        if (mRecordingTime < 1000) {
            mRecordingFileName = FmRecorder.FM_RECORDING_TIME_KEY;
            dismissAllowingStateLoss();
            if (mTempRecordingName != null) {
                File needToDelete = new File(recordingFolderPath, mTempRecordingName);
                if (needToDelete.exists()) {
                    needToDelete.delete();
                }
            }
            return;
        }
        /**
         * @}
         */

        String msg = null;
        // Check the recording name whether exist
        mRecordingNameToSave = mRecordingNameEditText.getText().toString().trim();

        /**
         * SPRD: bug474741, recording format selection. Original Android code: File
         * recordingFileToSave = new File(recordingFolderPath, mRecordingNameToSave +
         * FmRecorder.RECORDING_FILE_EXTENSION);
         * 
         * @{
         */
        File recordingFileToSave = null;
        if (FmRecorder.GLOBAL_RECORD_FORMAT_FLAG == 1) {
            recordingFileToSave = new File(recordingFolderPath, mRecordingNameToSave
                    + FmRecorder.RECORDING_FILE_EXTENSION);
        } else {
            recordingFileToSave = new File(recordingFolderPath, mRecordingNameToSave
                    + FmRecorder.RECORDING_FILE_EXTENSION_AMR);
        }
        /**
         * @}
         */

        if (mRecordingNameToSave.equals(mDefaultRecordingName)) {
            mIsNeedCheckFilenameExist = false;
        } else {
            mIsNeedCheckFilenameExist = true;
        }

        if (recordingFileToSave.exists() && mIsNeedCheckFilenameExist) {
            // show a toast notification if can't renaming a file/folder
            // to the same name
            msg = mRecordingNameEditText.getText().toString() + " "
                    + getActivity().getResources().getString(R.string.already_exists);
            Toast.makeText(getActivity(), msg, Toast.LENGTH_SHORT).show();
        } else {
            mRecordingFileName = mRecordingNameToSave;
            dismissAllowingStateLoss();
        }
        if (FmUtils.isAirplane(getActivity())) {
            getActivity().finish();
        }
    }

}
