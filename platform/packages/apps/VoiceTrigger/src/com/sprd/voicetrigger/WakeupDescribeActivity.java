
package com.sprd.voicetrigger;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.AssetManager;
import android.graphics.drawable.AnimationDrawable;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.AudioSystem;
import android.media.MediaRecorder;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.sprd.voicetrigger.global.RuntimeStates;
import com.sprd.voicetrigger.global.SharedPreferencesField;
import com.sprd.voicetrigger.languagesupport.SupportLanguages;
import com.sprd.voicetrigger.nativeinterface.Audio;
import com.sprd.voicetrigger.nativeinterface.Recog;
import com.sprd.voicetrigger.provider.ContentProviderHelper;
import com.sprd.voicetrigger.utils.DataCreaterUtil;
import com.sprd.voicetrigger.utils.LangagePackageManager;
import com.sprd.voicetrigger.utils.UserVoiceManager;
import com.sprd.voicetrigger.utils.VocabularyPackageManager;
import com.sprd.voicetrigger.view.RippleOnTouchListener;
import com.sprd.voicetrigger.view.RippleView;

import java.io.File;
import java.util.ArrayList;

public class WakeupDescribeActivity extends Activity {
    private static final String TAG = "WakeupDescribeActivity";
    private final String COMMANDONLY = "<No Trigger>";
    private final int ANIMATION_SHOW_TIME = 5000;
    // screen status
    private final int ENROLL_SCREEN_RESET = 0;
    private final int ENROLL_SCREEN_START = 1;
    private final int ENROLL_SCREEN_STOP = 2;
    private final int ENROLL_SCREEN_PASS = 3;
    private final int ENROLL_SCREEN_DONE = 4;

    public static final short NUMENROLL = 3;
    private static final short NUMEFT = 1;
    private static final short NUMUDT = 1;
    public static final short NUMUSERS = NUMEFT + NUMUDT;

    public Audio audioInstance = null;
    public Recog recogInstance = null;
    private SharedPreferences sharedPrefs = null;
    private SharedPreferences mPrivateModeSharedPref;
    private ArrayList<String> userName = null;
    private ArrayList<UserState> userState = null;
    private ArrayList<UserType> userType = null;

    private String choosedLanguage = SupportLanguages.getCurrentLauguage();
    private SupportLanguages mSupportLanguages;
    private UserVoiceManager mUserVoiceManager;
    private String intentLanguageKeyString;

    // Train Screen components
    private ImageView mTrainState;
    private TextView resultsTV;
    private LinearLayout mDescribeView;
    private LinearLayout mSpeechView;
    private ImageView mDescribeImage;
    private RippleView mRippleView;
    private TextView mPromptingText;
    public LangagePackageManager.packClass langPack = null;
    public VocabularyPackageManager.packClass trigPack = null;
    public static short enrollIdx = 1;
    private int mEnrollStatus = 0;
    public static int recsrc;
    private Context mContext;

    protected Thread rthread = null;
    protected Thread athread = null;

    public boolean includeFixed = false;
    public boolean requirePause = false;
    public boolean enrollRequired = true;
    public boolean retryMode = false;

    public int svLevel;
    public int triggerLevel;
    public int wakeupWordsMode = 0;
    public int udtsidMode = 1;
    public String appFilesDir = null;
    public String appCacheDir = null;

    private final int TRIGGER_SENSITIVITY_DEFAULT = 0;
    public int trigSensitivity = TRIGGER_SENSITIVITY_DEFAULT;
    private OnAudioFocusChangeListener mAudioFocusListener = null;
    private AudioManager mAudioManager = null;
    private boolean mHasAudioFocus = false;

    private String triggerName = null;
    private String FTnetFile = null;
    private String FTsearchFile = null;
    private String dspTarget = "rk38";
    // the hint text when user process
    private String firstHintText;
    private String secondHintText;
    private AssetManager mAssetManager;
    private AnimationDrawable frameAnim;

    private enum UserState {
        UNUSED, RECORDED, ENROLLED
    }

    private enum UserType {
        EFT, UDT
    }

    @Override
    public void onCreate(Bundle bundle) {
        Log.d(TAG, "onCreate");
        super.onCreate(bundle);
        mContext = this;
        setContentView(R.layout.voicetrigger_describe);
        mAudioManager = ((AudioManager) getSystemService(Context.AUDIO_SERVICE));
        appFilesDir = getFilesDir().getPath() + File.separator;
        appCacheDir = getCacheDir().getPath() + File.separator;
        mAssetManager = getAssets();
        // the app's sharedPreferences
        sharedPrefs = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        // this activity's sharedPreferences
        mPrivateModeSharedPref = this.getPreferences(Context.MODE_PRIVATE);
        // TODO require_silence is not set , you should set it if needed
        mSupportLanguages = new SupportLanguages(this);
        mUserVoiceManager = new UserVoiceManager(this);

        Intent intent = getIntent();
        wakeupWordsMode = intent.getIntExtra("choosemode", 0);
        intentLanguageKeyString = intent.getStringExtra("languageKey");
        if (intentLanguageKeyString != null) {
            choosedLanguage = intentLanguageKeyString;
        } else {
            choosedLanguage = mSupportLanguages.getCurrentLanguageKey();
        }
        Log.i(TAG, " choosedLanguage = " + choosedLanguage);
        // set hint text
        if (wakeupWordsMode == 0) {
            int index = mSupportLanguages.getIndexWithLanguageKey(choosedLanguage);
            firstHintText = mSupportLanguages.getChoosedLanguageWakeupWordsString(index,
                    getResources().getString(R.string.prompting_text_default));
            secondHintText = mSupportLanguages.getChoosedLanguageWakeupWordsString(index,
                    getResources().getString(R.string.prompting_text_1));
        } else if (wakeupWordsMode == 1) {
            firstHintText = String.format(
                    getResources().getString(R.string.prompting_text_default), getResources()
                            .getString(R.string.prompting_user_defined_text));
            secondHintText = String.format(getResources().getString(R.string.prompting_text_1),
                    getResources().getString(R.string.prompting_user_defined_text));
        } else {
            throw new IllegalArgumentException("choosemode is error");
        }
        Log.i(TAG, " onCreate userID = " + wakeupWordsMode);

        requirePause = sharedPrefs.getBoolean("require_silence", false);
        // the default value is 6; @see MediaRecorder.AudioSource.VOICE_RECOGNITION
        recsrc = sharedPrefs.getBoolean("audio_mode_recognition", true) ? Audio.recsrcMIC
                : Audio.recsrcVR;

        initView();
    }

    private RippleOnTouchListener mRippleOnTouchListener = new RippleOnTouchListener() {
        @Override
        public boolean onTouchEvent(View v, MotionEvent event) {
            return false;
        }

        @Override
        public void onActionUP() {
            // do nothing
        }

        @Override
        public void onStart() {
            startRecord();
        }

        @Override
        public void onStop() {
            // do nothing
        }
    };

    public void initView() {
        ActionBar actionBar = getActionBar();
        actionBar.setTitle(R.string.voicetrigger_input_title);
        actionBar.setDisplayHomeAsUpEnabled(true);
        mDescribeView = (LinearLayout) findViewById(R.id.voicetrigger_describe);
        mSpeechView = (LinearLayout) findViewById(R.id.voicetrigger_speeching);
        mDescribeImage = (ImageView) findViewById(R.id.voicetrigger_describe_image);
        mRippleView = (RippleView) findViewById(R.id.rippleview);
        mRippleView.setTouchActionEnable(true);
        mRippleView.setRippleOnTouchListener(mRippleOnTouchListener);
        resultsTV = (TextView) findViewById(R.id.trainFeedback);
        mTrainState = (ImageView) findViewById(R.id.voicetrigger_speeching_image);
        mDescribeView.setVisibility(View.VISIBLE);
        mSpeechView.setVisibility(View.GONE);
        mPromptingText = (TextView) findViewById(R.id.promptingText);
        // initialize animation
        frameAnim = new AnimationDrawable();
        frameAnim.addFrame(getResources().getDrawable(R.drawable.describe_speech1), 100);
        frameAnim.addFrame(getResources().getDrawable(R.drawable.describe_speech2), 100);
        mDescribeImage.setBackgroundDrawable(frameAnim);
        mPromptingText.setText(firstHintText);
        updateEnrollScreen(ENROLL_SCREEN_RESET);
    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
        frameAnim.start();
        loadConfig();
        // tell devices stop recognition
        ((VoiceTriggerApp) getApplication()).stopRecognition();
        // init packages background
        new mAsyncTask().execute("");
    }

    private class mAsyncTask extends AsyncTask<String, Integer, String> {
        private long animationStartTime = 0;

        @Override
        protected void onPreExecute() {
            animationStartTime = System.currentTimeMillis();
            // initialize audio instance
            if (AudioSystem.isSourceActive(MediaRecorder.AudioSource.MIC) ||
                    AudioSystem.isSourceActive(MediaRecorder.AudioSource.CAMCORDER) ||
                    AudioSystem.isSourceActive(MediaRecorder.AudioSource.VOICE_RECOGNITION)) {
                showDialog(getResources().getString(R.string.dialog_title_notice),
                        getResources().getString(R.string.same_application_running));
                Log.e(TAG, "onPreExecute isAudioRecording");
                return;
            }
        }

        @Override
        protected String doInBackground(String... params) {
            Log.d(TAG, "initPacks start");
            LangagePackageManager.getInstance().initPacks(mAssetManager, appFilesDir);
            VocabularyPackageManager.getInstance().initPacks(mAssetManager, appFilesDir);
            Log.d(TAG, "initPacks end");

            int index = mSupportLanguages.getIndexWithLanguageKey(choosedLanguage);
            langPack = LangagePackageManager.getInstance().getPack(index);
            Log.i(TAG, "Choose package:" + langPack.name);
            if (langPack.name.contains("English")) {
                triggerName = "Trigger: en_us hbg";
            } else {
                triggerName = "Trigger: zh_cn os";
            }

            trigPack = VocabularyPackageManager.getInstance().getPack(triggerName);
            if (trigPack == null) {
                triggerName = COMMANDONLY;
                FTnetFile = null;
                FTsearchFile = null;
            } else {
                Log.i(TAG, "Choose trigPack:" + trigPack.name);
                FTnetFile = trigPack.netFile;
                if (trigSensitivity >= 0 && trigSensitivity < trigPack.search.size()) {
                    FTsearchFile = trigPack.search.get(trigSensitivity).toString();
                } else {
                    FTnetFile = null;
                    FTsearchFile = null;
                }
            }

            Log.i(TAG, "Initial trigger = " + trigPack.name);
            Log.i(TAG, "Initial netfile = " + FTnetFile);
            Log.i(TAG, "Initial search = " + FTsearchFile);
            return null;
        }

        @Override
        protected void onPostExecute(String result) {
            // stop animation
            if (isFinishOrDestroy()){
                Log.d(TAG, "onPostExecute WakeupDescribeActivity is destroyed ");
                return;
            }
            long animationEndTime = System.currentTimeMillis();
            long useTime = animationEndTime - animationStartTime;
            if (useTime > ANIMATION_SHOW_TIME) {
                frameAnim.stop();
                mSpeechView.setVisibility(View.VISIBLE);
                mDescribeView.setVisibility(View.GONE);
            } else {
                new Handler().postDelayed(new Runnable() {
                    public void run() {
                        frameAnim.stop();
                        mSpeechView.setVisibility(View.VISIBLE);
                        mDescribeView.setVisibility(View.GONE);
                    }
                }, ANIMATION_SHOW_TIME - useTime);
            }

            if (audioInstance == null) {
                audioInstance = new Audio(new Audio.AudioStatusCallback() {

                    @Override
                    public void onStatusChanged(int status, String msg) {
                        sendMsg(status, msg);
                    }
                });
            }
            athread = new Thread(audioInstance);
            // start recording if has audio focus
            if (requestAudioFocus()) {
                mHasAudioFocus = true;
                athread.start();
                // TODO should stop record
//                sendMsg(RuntimeStates.CHECKED_REC_FAIL, getString(R.string.info_default));
            } else {
                showDialog(getResources().getString(R.string.dialog_title_notice),
                        getResources().getString(R.string.same_application_running));
                Log.e(TAG, "onPostExecute isAudioRecording");
                return;
            }
            if (recogInstance == null) {
                recogInstance = new Recog(audioInstance, wakeupWordsMode, new Recog.RecogStatusCallback() {
                    @Override
                    public void onStatusChanged(int status, String msg) {
                        sendMsg(status, msg);
                    }

                    @Override
                    public void onStatusChanged(int status, int id) {
                        String msg = getResources().getString(id);
                        sendMsg(status, msg);
                    }
                });
            }
            rthread = new Thread(recogInstance);
            // session needed to save recorded audio

            recogInstance.initSDK(langPack, trigPack, FTsearchFile, appCacheDir, requirePause);
            Log.i(TAG, "expired=" + recogInstance.expired);
            if (!recogInstance.expired) {
                rthread.start();
                recogInstance.setMode(recogInstance.trig, (short) udtsidMode);
                Log.d(TAG, "setMode");
            }
            // the first version need user press audio button to start record, so annotate it
//             startRecord();
            updateEnrollScreen(ENROLL_SCREEN_STOP);
        }
    }

    private boolean isFinishOrDestroy(){
        if(WakeupDescribeActivity.this.isDestroyed() || WakeupDescribeActivity.this.isFinishing()){
            return true;
        }
        return false;
    }

    private synchronized boolean requestAudioFocus() {
        if (mAudioFocusListener == null) {
            mAudioFocusListener = new AudioManager.OnAudioFocusChangeListener() {
                @Override
                public void onAudioFocusChange(int focusChange) {
                    Log.d(TAG, "AudioFocusChanged " + focusChange);
                    switch (focusChange) {
                        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                            mHasAudioFocus = false;
                            stopRecord();
                            break;
                        case AudioManager.AUDIOFOCUS_LOSS:
                            showDialog(getResources().getString(R.string.dialog_title_notice),
                                    getResources().getString(R.string.same_application_running));
                            break;
                        case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK:
                        case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT:
                        case AudioManager.AUDIOFOCUS_GAIN:
                            mHasAudioFocus = true;
                            startRecord();
                            break;
                        default:
                            break;
                    }
                }
            };
        }
        int resultCode = mAudioManager.requestAudioFocus(mAudioFocusListener, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        return resultCode == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "onPause");
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.i(TAG, "onStop");
        stopRecord();
        saveConfig();
        if (recogInstance != null) {
            recogInstance.exitRecog();
            if (!recogInstance.expired && rthread != null) {
                rthread.interrupt();
                recogInstance = null; // References audioInstance so exit first!
                rthread = null;
            }
        }
        if (audioInstance != null) {
            waitForAudioShutdown();
            audioInstance.exitAudio();
            audioInstance = null;
        }
        if (athread != null) {
            athread.interrupt();
            athread = null;
        }
        if (mAudioFocusListener != null) {
            mAudioManager.abandonAudioFocus(mAudioFocusListener);
            mAudioFocusListener = null;
        }
        // tell device to recognition when activity is exit
        // TODO : if function not open ,we should not start it
        ((VoiceTriggerApp) getApplication()).startRecognition();
        finish();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showDialog(String title, String msg) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(title)
                .setMessage(msg)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        finish();
                    }
                });
        // other cancel way ,such as back button ,click other area of dialog;
        builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                finish();
            }
        });
        builder.create().show();
    }

    private AlertDialog alert(String str, final Boolean end, String button) {
        AlertDialog.Builder b = new AlertDialog.Builder(this).setMessage(str);
        if (button != null) {
            b.setPositiveButton(button, new android.content.DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    if (end) {
                        finish();
                    } else {
                        dialog.cancel();
                    }
                }
            });
        }
        AlertDialog a = b.create();
        a.show();
        return a;
    }

    public final Handler myHandler = new Handler() {
        public void handleMessage(Message m) {
            String mstr = m.getData().getString("txt");
            switch (m.what) {
                case RuntimeStates.UPDATE_VU:
                    m = null;
                    break;
                case RuntimeStates.ADDBUFF:
                    Log.i(TAG, "handleMessage:ADDBUFF");
                    break;
                case RuntimeStates.NEED_ENROLL:
//                    alert("Begin by enrolling users", false, "Okay");
                    break;
                case RuntimeStates.STOPREC:
                    resultsTV.setText(mstr);
                    stopRecord();
                    break;
                case RuntimeStates.BAIL:
                    resultsTV.setText(mstr);
                    break;
                case RuntimeStates.ADDED_WAVE:
                    Log.i(TAG, "handleMessage:state.ADDED_WAVE:enrollIdx = " + enrollIdx
                            + ",retryMode=" + retryMode);
                    updateEnrollScreen(ENROLL_SCREEN_PASS);
                    if (enrollIdx == 99) {
                        updateEnrollScreen(ENROLL_SCREEN_DONE);
                    } else {
                        if (retryMode) {
                            enrollIdx = (short) (NUMENROLL + 1);
                        } else {
                            enrollIdx++;
                        }
                        if (enrollIdx > NUMENROLL) {
                            sendMsg(RuntimeStates.CHECKED_REC_FAIL,
                                    getResources().getString(R.string.info_check_rec));
                            updateEnrollScreen(ENROLL_SCREEN_DONE);
                            Log.d(TAG, "handleMessage:state.ADDED_WAVE:enrollIdx = " + enrollIdx);
                            stopRecord();
                            Thread thread = new Thread() {
                                public void run() {
                                    Log.d(TAG, "handleMessage:state.ADDED_WAVE:checkNewRecordings");
                                    if (recogInstance != null) {
                                        int x = recogInstance.checkNewRecordings(recogInstance.trig,
                                                (short) wakeupWordsMode);
                                        Log.d(TAG, "x = " + x + "wakeupWordsMode" + wakeupWordsMode + "recogInstance.trig" + recogInstance.trig);
                                        if (x != 0) {
                                            sendMsg(RuntimeStates.CHECKED_RECS, null);
                                        } else {
                                            sendMsg(RuntimeStates.CHECKED_RECS_FAIL, null);
                                        }
                                    } else {
                                        Log.d(TAG, " recogInstance has been destroyed");
                                    }
                                }
                            };
                            thread.start();
                        } else {
                            startRecord();
                        }
                    }
                    break;
                case RuntimeStates.CHECKED_REC_FAIL:
                    Log.d(TAG, "CHECKED_REC_FAIL");
                    resultsTV.setText(mstr);
                    break;
                case RuntimeStates.CHECKED_RECS_FAIL:
                    Log.d(TAG, "CHECKED_RECS_FAIL");
                    break;
                case RuntimeStates.CHECKED_REC_SUCCESS:
                    resultsTV.setText(mstr);
                    String srcFile = getFilesDir().getPath() + File.separator + "SMicTD1.dat";
                    String srcWaveFile = getCacheDir().getPath() + File.separator + "udt0_2.wav";
                    VoiceTriggerApp app = (VoiceTriggerApp) getApplication();
                    if (!app.isServiceBinded()) {
                        if (!app.bindService()) {
                            Log.i(TAG, "Trigger loadSoundModel failed");
                            return;
                        }
                    }
                    boolean isFirstLoadComPlete = sharedPrefs.getBoolean(
                            SharedPreferencesField.IS_FIRST_LOAD_COMPLETE, false);
                    app.loadSoundModel(isFirstLoadComPlete,srcFile);

                    Log.i(TAG, "RuntimeStates.CHECKED_REC_SUCCESS:choosedLanguage="
                            + choosedLanguage + " + srcFile = " + srcFile);
                    if (wakeupWordsMode == 0) {
                        mUserVoiceManager.addEFTVoiceDataToFile(srcFile, "user0", choosedLanguage);
                    } else {
                        mUserVoiceManager.addUDTVoiceDataToFile(srcFile, "user0");
                        mUserVoiceManager.addWaveDataToFile(srcWaveFile, "udt");
                    }
                    Editor edit = sharedPrefs.edit();
                    edit.putBoolean(SharedPreferencesField.IS_FIRST_LOAD_COMPLETE, true);
                    edit.putString(SharedPreferencesField.CHOOSED_LANGUAGE, choosedLanguage);
                    edit.commit();
                    ContentProviderHelper.setMode(mContext, wakeupWordsMode == 0 ? true : false);
                    ContentProviderHelper.setIsOpenSwitchStatus(mContext, true);
                    Intent intent = new Intent();
                    setResult(RESULT_OK, intent);
                    finish();
                    break;

                case RuntimeStates.CHECKED_RECS:
                    Log.d(TAG, "CHECKED_RECS");
                    int sum = 0;
                    float score;
                    if (recogInstance != null) {
                        score = recogInstance.getPhraseQuality(recogInstance.trig,
                                (short) (wakeupWordsMode));
                        long[] details = recogInstance.getFeedbackDetails(recogInstance.trig,
                                (short) (wakeupWordsMode));
                        for (int x = 0; x < NUMENROLL; x++) {
                            sum += details[x];
                            Log.i(TAG, "Feedback details: " + x + "=" + details[x]);
                        }
                    } else {
                        Log.i(TAG, "Feedback recogInstance has been destroyed" );
                        return;
                    }
                    if (sum == 0) {
                        userState.set(wakeupWordsMode, UserState.RECORDED);
                        enrollRequired = true;
                        Log.i(TAG, "Trigger Quality=" + score);
                        if (enrollRequired == true) {
                            sendMsg(RuntimeStates.ENROLL, null);
                        }
                        //exit audio thread time
                        if (audioInstance != null) {
                            waitForAudioShutdown();
                            audioInstance.exitAudio();
                            audioInstance = null;
                        }
                        if (athread != null) {
                            athread.interrupt();
                            athread = null;
                        }
                        //exit end
                    } else {
                        sendMsg(RuntimeStates.ENROLL_FAILED, null);
                    }
                    break;

                case RuntimeStates.ENROLL:
                    Thread thread = new Thread() {
                        public void run() {
                            if (recogInstance != null) {
                                int enrollResult = recogInstance.doEnroll(recogInstance.trig, appCacheDir,
                                        includeFixed ? FTnetFile : null,
                                        includeFixed ? FTsearchFile : null,
                                        dspTarget.equals("<none>") ? null : dspTarget);
                                if (enrollResult == 0) {
                                    sendMsg(RuntimeStates.ENROLL_FAILED, null);
                                } else {
                                    sendMsg(RuntimeStates.ENROLL_DONE, null);
                                }
                            } else {
                                Log.i(TAG, "ENROLL recogInstance has been destroyed" );
                            }
                        }
                    };
                    thread.start();
                    break;
                case RuntimeStates.ENROLL_FAILED:
                    // recording again when something is error
                    if (!WakeupDescribeActivity.this.isFinishing()) {
                        AlertDialog.Builder b = new AlertDialog.Builder(mContext)
                                .setMessage(getResources().getString(R.string.info_problems_in_rec));
                        b.setPositiveButton(getResources().getString(android.R.string.ok),
                                new android.content.DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        updateEnrollScreen(ENROLL_SCREEN_RESET);
                                        dialog.cancel();
                                    }
                                });
                        AlertDialog a = b.create();
                        a.show();
                    }
                    break;
                case RuntimeStates.ENROLL_DONE:
                    enrollRequired = false;
                    Log.d(TAG, " myHandler ENROLL_DONE create_header");
                    try {
                        File fd_gram = new File(appCacheDir, "gram.bin");
                        File fd_net = new File(appCacheDir, "net.bin");
                        DataCreaterUtil.createHeader(mContext, fd_gram, fd_net);
                        sendMsg(RuntimeStates.CHECKED_REC_SUCCESS,
                                getString(R.string.generate_voicetrigger_words_success));
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    break;
                default:
                    Log.e(TAG, "ERROR: myHandler: undefined msg=" + m.what);
            }
            mstr = null;
            super.handleMessage(m);
        }
    };

    public void sendMsg(int id, String s) {
        Message m = new Message();
        m.what = id;
        if (s != null) {
            Bundle b = new Bundle();
            b.putString("txt", s);
            m.setData(b);
        }
        myHandler.sendMessage(m);
        m = null;
    }

    private void waitForAudioShutdown() {
        if (audioInstance == null)
            return;
        while (!audioInstance.isStopped()) {
            synchronized (this) {
                try {
                    wait(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            Log.i(TAG, "waiting for audio shutdown...");
        }
    }

    private void startRecord() {
        if (audioInstance != null && mHasAudioFocus && !audioInstance.isRecording()) {
            audioInstance.flushAudio();
            recogInstance.resetFlag = true;
            audioInstance.setRecording(true);
            updateEnrollScreen(ENROLL_SCREEN_START);
            Log.i(TAG, "startRecord");
        }
    }

    private void stopRecord() {
        if (audioInstance == null || !audioInstance.isRecording()) {
            return;
        }
        updateEnrollScreen(ENROLL_SCREEN_STOP);
        audioInstance.setRecording(false);
        // Wait for current recordPipe to end before resetting?
        audioInstance.flushAudio();
        recogInstance.resetFlag = true;
        Log.i(TAG, "stopRecord");
    }

    private void saveConfig() {
        Editor e = mPrivateModeSharedPref.edit();
        e.putLong("svLevel", svLevel);
        e.putLong("triggerLevel", triggerLevel);
        e.putBoolean("enrollRequired", enrollRequired);
        e.putLong("trigsensitivity", trigSensitivity);
        e.putString("triggername", triggerName);
        for (int i = 0; i < NUMUSERS; i++) {
            e.putString("userName" + i, userName.get(i));
            e.putInt("userState" + i, userState.get(i).ordinal());
            e.putInt("userType" + i, userType.get(i).ordinal());
        }
        e.commit();
    }

    private void loadConfig() {
        Log.d(TAG, "load config");
        triggerLevel = (int) mPrivateModeSharedPref.getLong("triggerLevel", 3);
        svLevel = (int) mPrivateModeSharedPref.getLong("svLevel", 3);
        enrollRequired = mPrivateModeSharedPref.getBoolean("enrollRequired", true);
        userName = new ArrayList<>();
        userState = new ArrayList<>();
        userType = new ArrayList<>();
        for (int i = 0; i < NUMEFT; i++) {
            userName.add(i,
                    mPrivateModeSharedPref.getString("userName" + i, "EFT " + (i + 1)));
            userState.add(i, UserState.values()[mPrivateModeSharedPref.getInt("userState" + i, 0)]);
            userType.add(i, UserType.values()[mPrivateModeSharedPref.getInt("userType" + i, 0)]);
        }
        for (int i = 0; i < NUMUDT; i++) {
            userName.add(
                    i + NUMEFT,
                    mPrivateModeSharedPref.getString("userName" + (i + NUMEFT), "UDT "
                            + (i + 1)));
            userState
                    .add(i + NUMEFT,
                            UserState.values()[mPrivateModeSharedPref.getInt("userState"
                                    + (i + NUMEFT), 0)]);
            userType.add(i + NUMEFT,
                    UserType.values()[mPrivateModeSharedPref.getInt("userType" + (i + NUMEFT), 1)]);
        }
        trigSensitivity = (int) mPrivateModeSharedPref.getLong("trigsensitivity",
                TRIGGER_SENSITIVITY_DEFAULT);
        triggerName = mPrivateModeSharedPref.getString("triggername", "");
    }

    private void updateEnrollScreen(int mode) {
        Log.i(TAG, "Entering updateEnrollScreen, mode=" + mode);
        if (mode == ENROLL_SCREEN_PASS) {
            if (enrollIdx == 99)
                return;
            switch (enrollIdx) {
                case 1:
                    mEnrollStatus = 1;
                    mTrainState.setBackgroundResource(R.drawable.progress_bg_3_1);
                    break;
                case 2:
                    mEnrollStatus = 2;
                    mTrainState.setBackgroundResource(R.drawable.progress_bg_3_2);
                    break;
                case 3:
                    mEnrollStatus = 3;
                    mTrainState.setBackgroundResource(R.drawable.progress_bg_3_3);
                    break;
            }
            return;
        } else if (mode == ENROLL_SCREEN_RESET) {
            mEnrollStatus = 0;
            enrollIdx = 1;
            mRippleView.setTouchActionEnable(true);
            resultsTV.setText(R.string.info_default);
            mTrainState.setBackgroundResource(R.drawable.progress_bg_3_0);
            mPromptingText.setVisibility(View.VISIBLE);
            mPromptingText.setText(firstHintText);
        } else if (mode == ENROLL_SCREEN_DONE) {
            mPromptingText.setVisibility(View.INVISIBLE);
            mRippleView.setTouchActionEnable(false);
        } else if (mode == ENROLL_SCREEN_START) {
            if (mEnrollStatus == 0) {
                mPromptingText.setText(firstHintText);
            } else if (mEnrollStatus == 1 || mEnrollStatus == 2) {
                mPromptingText.setText(secondHintText);
            } else {
                mPromptingText.setText(R.string.prompting_text_2);
            }
            resultsTV.setText(R.string.info_recording);
            mRippleView.stratRipple();
        } else if (mode == ENROLL_SCREEN_STOP) {
            mRippleView.stopRipple();
            mPromptingText.setText(R.string.info_press_to_start);
        }
    }

}
