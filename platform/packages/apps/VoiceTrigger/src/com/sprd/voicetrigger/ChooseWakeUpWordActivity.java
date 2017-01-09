
package com.sprd.voicetrigger;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.voicetrigger.languagesupport.SupportLanguages;
import com.sprd.voicetrigger.provider.ContentProviderHelper;
import com.sprd.voicetrigger.utils.DataCreaterUtil;
import com.sprd.voicetrigger.utils.UserVoiceManager;

import java.io.File;

public class ChooseWakeUpWordActivity extends Activity {
    private static final String TAG = "ChooseWakeUpWordAct";
    private SupportLanguages mSupportLanguages;
    private UserVoiceManager mUserVoiceManager;
    private Context mContext;

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        mContext = getBaseContext();
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        mSupportLanguages = new SupportLanguages(this);
        mUserVoiceManager = new UserVoiceManager(this);
        dialog();
    }

    protected void dialog() {
        View view = LayoutInflater.from(ChooseWakeUpWordActivity.this).inflate(
                R.layout.wakeup_word_dialog_layout, null);
        TextView defaultWakeupWordsText = (TextView) view
                .findViewById(R.id.wakeup_word_dialog_text1);
        String text = mSupportLanguages.getChoosedLanguageWakeupWordsString(
                mSupportLanguages.getCurrentLanguageIndex(),
                getResources().getString(R.string.voicetrigger_wakeup_item_1));
        defaultWakeupWordsText.setText(text);
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        final AlertDialog dialog = builder.setTitle(R.string.voicetrigger_wakeup_title)
                .setPositiveButton(android.R.string.cancel, new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        finish();
                    }
                }).setView(view).create();
        // other cancel way ,such as back button ,click other area of dialog;
        builder.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        });
        DialogItemOnclickListener dialogItemOnclickListener = new DialogItemOnclickListener(dialog);
        view.findViewById(R.id.wakeup_word_dialog_text1).setOnClickListener(
                dialogItemOnclickListener);
        view.findViewById(R.id.wakeup_word_dialog_text2).setOnClickListener(
                dialogItemOnclickListener);

        RadioGroup mRadioGroup = (RadioGroup) view.findViewById(R.id.wakeup_word_dialog_radioGroup);
        boolean isDefaultWakeUp = ContentProviderHelper.isDefaultMode(mContext);
        // set default
        if (isDefaultWakeUp) {
            mRadioGroup.check(R.id.wakeup_word_dialog_radioButton1);
        } else {
            mRadioGroup.check(R.id.wakeup_word_dialog_radioButton2);
        }
        mRadioGroup.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
//                Editor editor = mSharedPreferences.edit();
                String key = mSupportLanguages.getCurrentLanguageKey();
                switch (checkedId) {
                    // default set use eft
                    case R.id.wakeup_word_dialog_radioButton1:
                        if (mUserVoiceManager.hasEFTVoiceDataFile("user0", key)) {
                            String dataDir = mUserVoiceManager.getEFTDataPath("user0", key);
                            ContentProviderHelper.setMode(ChooseWakeUpWordActivity.this, true);
                            if (!setDataToDevice(dataDir)){
                                ContentProviderHelper.setMode(ChooseWakeUpWordActivity.this, false);
                            }
                        } else {
                            gotoRecording(0);
                        }
                        dialog.dismiss();
                        finish();
                        break;
                    // udt
                    case R.id.wakeup_word_dialog_radioButton2:
                        if (mUserVoiceManager.hasUDTVoiceDataFile("user0")) {
                            String dataDir = mUserVoiceManager.getUDTDataPath("user0");
                            ContentProviderHelper.setMode(ChooseWakeUpWordActivity.this, false);
                            if( !setDataToDevice(dataDir)){
                                ContentProviderHelper.setMode(ChooseWakeUpWordActivity.this, true);
                            }
                        } else {
                            gotoRecording(1);
                        }
                        dialog.dismiss();
                        finish();
                        break;
                    default:
                        break;
                }
            }
        });
        builder.show();
    }

    private boolean setDataToDevice(String dataDir) {

        if (((VoiceTriggerApp)getApplication()).isServiceBinded()) {
            ((VoiceTriggerApp)getApplication()).loadSoundModel(false,dataDir);
            // replace voiceFile to default file dir ;
            DataCreaterUtil.copyFile(dataDir, getFilesDir().getPath() + File.separator + "SMicTD1.dat");
            return true;
        } else {
            Toast.makeText(mContext, R.string.save_file_to_device_error, Toast.LENGTH_SHORT).show();
            return false;
        }
    }

    private void gotoRecording(int witch) {
        Intent intent = new Intent();
        intent.setClass(this,WakeupDescribeActivity.class);
        intent.setAction("android.intent.action.MAIN");
        intent.putExtra("choosemode", witch);
        intent.putExtra("languageKey", mSupportLanguages.getCurrentLanguageKey());
        startActivity(intent);
    }

    public class DialogItemOnclickListener implements android.view.View.OnClickListener {
        private Dialog mDialog;

        public DialogItemOnclickListener(Dialog dialog) {
            mDialog = dialog;
        }

        @Override
        public void onClick(View v) {

            switch (v.getId()) {
                case R.id.wakeup_word_dialog_text1:
                    gotoRecording(0);
                    mDialog.dismiss();
                    finish();
                    break;
                case R.id.wakeup_word_dialog_text2:
                    gotoRecording(1);
                    mDialog.dismiss();
                    finish();
                    break;
                default:
                    break;
            }
        }

    }
}
