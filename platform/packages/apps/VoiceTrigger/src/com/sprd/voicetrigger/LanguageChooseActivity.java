
package com.sprd.voicetrigger;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Window;
import android.widget.Toast;

import com.sprd.voicetrigger.global.SharedPreferencesField;
import com.sprd.voicetrigger.languagesupport.SupportLanguages;
import com.sprd.voicetrigger.provider.ContentProviderHelper;
import com.sprd.voicetrigger.utils.DataCreaterUtil;
import com.sprd.voicetrigger.utils.UserVoiceManager;

import java.io.File;

public class LanguageChooseActivity extends Activity {
    private static final String TAG = "LanguageChooseActivity";
    public static final int REQUEST_CHANGELANGUAGES_CODE = 0x0001;
    private String[] items;
    private SharedPreferences mSharedPreferences;
    private Editor mEditor;
    private String choosedLanguage = SupportLanguages.getCurrentLauguage();
    private int mCheckedItem = -1;
    private SupportLanguages mSupportLanguages;
    private Context mContext;

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        mSupportLanguages = new SupportLanguages(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        mEditor = mSharedPreferences.edit();
        choosedLanguage = mSupportLanguages.getCurrentLanguageKey();
        mCheckedItem = mSupportLanguages.getCurrentLanguageIndex();
        mContext = this;
        items = getResources().getStringArray(R.array.support_languages_value);
        showDialog();
    }

    protected void showDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.audio_voicetrigger_language_choose)
                .setSingleChoiceItems(items, mCheckedItem, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (mCheckedItem == which) {
                            dialog.cancel();
                        } else {
                            new DoChangeLanguageTask().execute(which);
                            dialog.dismiss();
                        }
                    }
                }).setNegativeButton(android.R.string.cancel, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                finish();
            }
        });
        // other cancel way ,such as back button ,click other area of dialog;
        builder.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        });
        builder.create().show();
    }

    private class DoChangeLanguageTask extends AsyncTask<Integer, Integer, Integer> {
        private final int STATUS_UNKNOWN = -1;
        private final int STATUS_FAILED = 0;
        private final int STATUS_SUCCESS = 1;
        private final int STATUS_NOT_HAS_DATA_FILE = 2;
        private final int STATUS_USER_CANCEL = 3;
        ProgressDialog mypDialog = new ProgressDialog(mContext);
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            mypDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
            mypDialog.setMessage(getResources().getString(
                    R.string.change_language_progress_dialog_message));
            mypDialog.setIndeterminate(false);
            mypDialog.setCancelable(false);
            mypDialog.show();
        }

        @Override
        protected void onPostExecute(Integer result) {

            switch (result) {
                case STATUS_UNKNOWN:
                    break;
                case STATUS_FAILED:
                    Toast.makeText(getApplicationContext(), R.string.language_changed_failed, Toast.LENGTH_SHORT)
                            .show();
                    break;
                case STATUS_SUCCESS:
                    Toast.makeText(getApplicationContext(), R.string.language_changed_success, Toast.LENGTH_SHORT).show();
                    mEditor.putString(SharedPreferencesField.CHOOSED_LANGUAGE, choosedLanguage);
                    mEditor.commit();
                    finish();
                    break;
                case STATUS_USER_CANCEL:
                    Toast.makeText(getApplicationContext(), R.string.language_change_has_canceled, Toast.LENGTH_SHORT)
                            .show();
                    finish();
                    break;
                case STATUS_NOT_HAS_DATA_FILE:
                    mypDialog.cancel();
                    builder.setTitle(R.string.dialog_title_notice)
                            .setMessage(R.string.dialog_change_language_first)
                            .setPositiveButton(android.R.string.ok, new OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    // TODO start Activity for result
                                    Intent intent = new Intent();
                                    intent.setClass(mContext, WakeupDescribeActivity.class);
                                    intent.putExtra("languageKey", choosedLanguage);
                                    startActivityForResult(intent, REQUEST_CHANGELANGUAGES_CODE);
                                }
                            }).setNegativeButton(android.R.string.cancel, new OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            // user canceled
                            onPostExecute(STATUS_USER_CANCEL);
                            dialog.dismiss();
                        }
                    });
                    // other cancel way 
                    builder.setOnCancelListener(new OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialog) {
                            onPostExecute(STATUS_USER_CANCEL);
                            dialog.dismiss();
                        }
                    });
                    builder.create().show();
                    break;

                default:
                    break;
            }
            super.onPostExecute(result);
            mypDialog.cancel();
        }

        @Override
        protected Integer doInBackground(Integer... params) {
            choosedLanguage = mSupportLanguages.getLanguageKeyString(params[0]);
            boolean isDefaultWakeUp = ContentProviderHelper.isDefaultMode(mContext);
            UserVoiceManager userVoiceManager = new UserVoiceManager(mContext);
            if (isDefaultWakeUp) {
                if (userVoiceManager.hasEFTVoiceDataFile("user0", choosedLanguage)) {
                    if (setDataFileToDevice(userVoiceManager.getEFTVoiceDataFile("user0",
                            choosedLanguage))) {
                        Log.e(TAG, "set file to device successfull");
                    } else {
                        Log.e(TAG, "set file to device failed");
                        return STATUS_FAILED;// failed
                    }
                } else { // there is not have dat files in files directory
                    return STATUS_NOT_HAS_DATA_FILE;
                }
            }
            if (ChangeVoiceRecogniserLanguage(params[0])) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_FAILED;
            }
        }

    }

    private boolean ChangeVoiceRecogniserLanguage(int langugaeId) {
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        // TODO ChangeVoiceRecogniserLanguage
        Log.d(TAG, "ChangeVoiceRecogniserLanguage and return true");
        return true;
    }

    private boolean setDataFileToDevice(File dataFile) {
        String dataDir = dataFile.getPath();
        VoiceTriggerApp app = (VoiceTriggerApp) getApplication();
        // replace voiceFile to default file dir ;
        DataCreaterUtil.copyFile(dataDir, getFilesDir().getPath() + File.separator + "SMicTD1.dat");
        if (app.isServiceBinded()) {
            app.loadSoundModel(false, dataDir);
            return true;
        } else {
            Log.d(TAG, "setDataFileToDevice and return false");
            return false;
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.d(TAG, "onActivityResult");
        if (resultCode == Activity.RESULT_OK) {
            mEditor.putString(SharedPreferencesField.CHOOSED_LANGUAGE, choosedLanguage);
            mEditor.commit();
            Toast.makeText(mContext, "change Language success", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(mContext, "change Language fail", Toast.LENGTH_SHORT).show();
        }
        finish();
    }

}
