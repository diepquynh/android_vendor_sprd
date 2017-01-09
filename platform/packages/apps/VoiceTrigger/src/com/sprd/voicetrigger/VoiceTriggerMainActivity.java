
package com.sprd.voicetrigger;

import android.Manifest;
import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.drawable.AnimationDrawable;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.UserManager;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.content.pm.PackageManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.KeyEvent;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.Toast;

import com.sprd.voicetrigger.global.SharedPreferencesField;
import com.sprd.voicetrigger.languagesupport.SupportLanguages;
import com.sprd.voicetrigger.provider.ContentProviderHelper;
import com.sprd.voicetrigger.utils.UserVoiceManager;
import com.sprd.voicetrigger.view.ToggleSwitch;

import java.io.File;

public class VoiceTriggerMainActivity extends PreferenceActivity implements /*View.OnClickListener,*/
        CompoundButton.OnCheckedChangeListener, Preference.OnPreferenceClickListener {
    public static final String TAG = "VoiceTrigger";
    private ToggleSwitch mSwitch;
    private SharedPreferences mSharedPreferences;
    private PreferenceScreen mWorkPreference;
    private PreferenceScreen mSensibilityPreference;
    private PreferenceScreen mSupportPreference;
    private PreferenceScreen mHeadsetPreference;
    private PreferenceScreen mLanguageChoose;
    private PreferenceScreen mHelp;
    public String extDir = null;
    private UserVoiceManager mUserVoiceManager;
    private SupportLanguages mSupportLanguages;
    private MediaPlayer mMediaPlayer;
    private Dialog wavePlayerDialog;
    private boolean mNeedRequestPermissions = false;

    @Override
    public void onCreate(Bundle icycle) {
        super.onCreate(icycle);
        mNeedRequestPermissions = PermissionUtils.checkAndBuildPermissions(this,PermissionUtils.VOICETRIGGERMAIN_PERMISSIONS_REQUEST_CODE);
        addPreferencesFromResource(R.xml.voicetrigger_settings);
        View actionbarLayout = LayoutInflater.from(this).inflate(R.layout.actionbar_layout, null);
        ActionBar actionBar = getActionBar();
        actionBar.setCustomView(actionbarLayout);
        actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_CUSTOM);
        mSwitch = (ToggleSwitch) actionbarLayout.findViewById(R.id.switch_widget);
        mSwitch.setOnCheckedChangeListener(this);

        mSupportLanguages = new SupportLanguages(this);
        mWorkPreference = (PreferenceScreen) findPreference("voicetrigger_words_settings");
        mSensibilityPreference = (PreferenceScreen) findPreference("voicetrigger_sensibility_settings");
//        mSupportPreference = (PreferenceScreen) findPreference("voicetrigger_support_fun");
        mHeadsetPreference = (PreferenceScreen) findPreference("voicetrigger_connect_headset");
        mLanguageChoose = (PreferenceScreen) findPreference("voicetrigger_language_choose");
        mHelp = (PreferenceScreen) findPreference("voicetrigger_help");
        mHelp.setOnPreferenceClickListener(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());

        mUserVoiceManager = new UserVoiceManager(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        if (mNeedRequestPermissions) {
            Log.d(TAG, "need request permissions before onResume");
            return;
        }
        Log.d(TAG, "checkIsFirstUse:" + isFirstLoadComplete());
        if (isFirstLoadComplete() && ContentProviderHelper.isOpenSwitch(this)) {
            mSwitch.setChecked(true);
            setPreferenceEnable(true);
        } else {
            mSwitch.setChecked(false);
            setPreferenceEnable(false);
        }
    }

    @Override
    protected void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
        // set the summary text dependence different languages and different mode
        String text;
        if (ContentProviderHelper.isDefaultMode(this)) {
            text = mSupportLanguages.getChoosedLanguageWakeupWordsString(
                    mSupportLanguages.getCurrentLanguageIndex(),
                    getResources().getString(R.string.voicetrigger_wakeup_item_1));
        } else {
            text = getResources().getString(R.string.voicetrigger_wakeup_item_2);
        }
        mWorkPreference.setSummary(text);
        if (mNeedRequestPermissions) {
            Log.d(TAG, "need request permissions before start RecordService!");
            return;
        }
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
    }

    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            Log.d(TAG, "onOptionsItemSelected: press home button");
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private boolean isFirstLoadComplete() {
        return mSharedPreferences.getBoolean(SharedPreferencesField.IS_FIRST_LOAD_COMPLETE, false);
    }

    private void showFirstUseGuideDialog() {
        AlertDialog.Builder builder = new Builder(this);
        builder.setMessage(R.string.dialog_firstuse_info);
        builder.setTitle(R.string.dialog_title);
        builder.setPositiveButton(android.R.string.ok, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Intent intent = new Intent();
                intent.setClass(getBaseContext(), ChooseWakeUpWordActivity.class);
                startActivity(intent);
            }
        });
        builder.setNegativeButton(android.R.string.cancel, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                mSwitch.setChecked(false);
            }
        });
        // other cancel way ,such as back button ,click other area of dialog;
        builder.setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mSwitch.setChecked(false);
            }
        });
        builder.create().show();
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        Log.d(TAG, "onCheckedChanged isChecked = " + isChecked);
        ContentProviderHelper.setIsOpenSwitchStatus(this,isChecked);
        VoiceTriggerApp app = (VoiceTriggerApp) getApplication();

        if (isChecked) {
            if (!isFirstLoadComplete()) {
                UserManager userManager = (UserManager) getSystemService(Context.USER_SERVICE);
                Log.i(TAG, "VoiceTriggerService  userManager.isGuestUser() ="+userManager.isGuestUser()+",isSystemUser="+userManager.isSystemUser()
                        +",isAdminUser="+userManager.isAdminUser()+",isLinkedUser="+userManager.isLinkedUser()
                        +",getUserName ="+userManager.getUserName()+",getUserHandle="+userManager.getUserHandle());
                if (!userManager.isSystemUser()){
                    Log.i(TAG, "VoiceTriggerService isn't isSystemUser !");
                    Toast.makeText(getApplicationContext(), R.string.is_not_systemuser, Toast.LENGTH_SHORT).show();
                    finish();
                    return;
                }
                showFirstUseGuideDialog();
            } else {
                if (!app.isServiceBinded()) {
                    if (app.bindService()) {
                        app.startRecognition();
                        setPreferenceEnable(true);
                    } else {
                        buttonView.setChecked(false);
                        Toast.makeText(this,"bind service failed",Toast.LENGTH_SHORT).show();
                    }
                } else {
                    app.startRecognition();
                    setPreferenceEnable(true);
                }

            }
        } else {
            if (app.isServiceBinded()) {
                app.stopRecognition();
                app.unbindService();
                // TODO should stop service
                // app.stopService()
            }
            setPreferenceEnable(false);
        }
    }

    private void setPreferenceEnable(boolean isEnable) {
        mWorkPreference.setEnabled(isEnable);
        mSensibilityPreference.setEnabled(isEnable);
        // we should not close the user help feature
        // mSupportPreference.setEnabled(isEnable);
        // mHeadsetPreference.setEnabled(isEnable);
         mLanguageChoose.setEnabled(isEnable);
        if (isEnable && !ContentProviderHelper.isDefaultMode(this)) {
            mHelp.setEnabled(true);
        } else {
            mHelp.setEnabled(false);
        }
    }

    public boolean preparePlayer(){
        String waveDataPath = mUserVoiceManager.getWaveDataPath();
        if (waveDataPath == null) {
            Toast.makeText(getApplicationContext(), R.string.no_wakeup_words_recoded, Toast.LENGTH_SHORT).show();
            return false;
        }
        mMediaPlayer = MediaPlayer.create(this,Uri.fromFile(new File(waveDataPath)));
        mMediaPlayer.setLooping(false);
        mMediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener(){

            @Override
            public void onCompletion(MediaPlayer player) {
                ((VoiceTriggerApp)getApplication()).startRecognition();
                wavePlayerDialog.dismiss();
                if (mMediaPlayer != null) {
                    mMediaPlayer.release();
                    mMediaPlayer = null;
                }
            }
        });
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        Log.e(TAG, "onPreferenceClick");
        try {
            if(!preparePlayer()){
                return false;
            }
            View view = LayoutInflater.from(this).inflate(R.layout.udt_voice_play_dialog_layout, null);
            ImageView animationImage = (ImageView) view.findViewById(R.id.play_voice_anim_image);
            AnimationDrawable frameAnim = new AnimationDrawable();
            frameAnim.addFrame(getResources().getDrawable(R.drawable.describe_speech1), 100);
            frameAnim.addFrame(getResources().getDrawable(R.drawable.describe_speech2), 100);
            animationImage.setBackground(frameAnim);
            wavePlayerDialog = new Dialog(this);
            wavePlayerDialog.setCancelable(true);
            wavePlayerDialog.setContentView(view);
            wavePlayerDialog.setTitle(R.string.audio_voicetrigger_subtitle_help);
            wavePlayerDialog.setCanceledOnTouchOutside(true);
            wavePlayerDialog.setOnCancelListener(new OnCancelListener() {
                @Override
                public void onCancel(DialogInterface dialog) {
                    if (!mMediaPlayer.isPlaying() && mMediaPlayer != null) {
                        mMediaPlayer.stop();
                        mMediaPlayer.release();
                        mMediaPlayer = null;
                    }
                }
            });
            wavePlayerDialog.create();
            wavePlayerDialog.show();
            frameAnim.start();
            ((VoiceTriggerApp)getApplication()).stopRecognition();

            mMediaPlayer.start();
        }catch (Exception e){
            ((VoiceTriggerApp)getApplication()).startRecognition();
            e.printStackTrace();
        }finally {
            if (mMediaPlayer != null && !mMediaPlayer.isPlaying()) {
                mMediaPlayer.release();
                mMediaPlayer = null;
            }
            return true;
        }
    }

@Override
    public void onRequestPermissionsResult(int requestCode,
            String permissions[], int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult");
        switch (requestCode) {
            case PermissionUtils.VOICETRIGGERMAIN_PERMISSIONS_REQUEST_CODE: {
                mNeedRequestPermissions = PermissionUtils.requestPermissionsResult(requestCode,permissions,grantResults,this);
                return;
            }
        }
    }
}
