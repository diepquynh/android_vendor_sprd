
package com.sprd.voicetrigger;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.net.Uri;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.voicetrigger.controller.RecogniseController;
import com.sprd.voicetrigger.controller.RecogniseController.OnRecogniseStatusChangedListener;
import com.sprd.voicetrigger.view.RippleView;

public class HelloTriggerActivity extends Activity implements OnRecogniseStatusChangedListener {

    // set default how many item will show in text content
    private static int textShowItemNum = 6;
    // activity views
    private TextView mTitleText;
    private TextView mContentText;
    private RippleView mRippleView;
    LayoutInflater layout;
    View rootView;
    // View decorView;

    private String[] mTextTitleArray;
    private String[] mTextContentArray;

    private Animation textTitleInAnim;
    private Animation textContentInAnim;
    private Animation textTitleOutAnim;
    private Animation textContentOutAnim;

    private RecogniseController mRecogniseController;
    private AudioManager mAudioManager = null;
    private boolean mNeedRequestPermissions = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);
        mNeedRequestPermissions = PermissionUtils.checkAndBuildPermissions(this,PermissionUtils.HELLOTRIGGER_PERMISSIONS_REQUEST_CODE);

        setContentView(R.layout.hello_voice_trigger);
        // get the title text content from string.xml
        mTextTitleArray = getResources().getStringArray(R.array.hello_trigger_text_title_array);
        mTextContentArray = getResources()
                .getStringArray(R.array.hello_trigger_text_content_array1);

        // init view
        mTitleText = (TextView) findViewById(R.id.text_title);
        mContentText = (TextView) findViewById(R.id.text_content);
        mRippleView = (RippleView) findViewById(R.id.hello_voice_trigger_rippleview);
        mRippleView.setTouchActionEnable(true);

        // init animation
        textTitleInAnim = AnimationUtils.loadAnimation(this, R.anim.hello_trigger_title_anim_in);
        textTitleOutAnim = AnimationUtils.loadAnimation(this, R.anim.hello_trigger_title_anim_out);
        textContentInAnim = AnimationUtils
                .loadAnimation(this, R.anim.hello_trigger_content_anim_in);
        textContentOutAnim = AnimationUtils.loadAnimation(this,
                R.anim.hello_trigger_content_anim_out);

        // creat RecogniseController
        mRecogniseController = new RecogniseController(this);
        //mRecogniseController.setOnRecogniseStatusChangedListener(this);

        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

        rootView = ((ViewGroup) (getWindow().getDecorView().findViewById(android.R.id.content)))
                .getChildAt(0);
        // decorView = getWindow().getDecorView();
        layout = getLayoutInflater();

        mVibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);

    }

    @Override
    protected void onStart() {
        // for test

        super.onStart();
        if (mNeedRequestPermissions) {
            Log.d(TAG, "onStart need request permissions before start RecordService!");
            return;
        }
    }

    private Vibrator mVibrator;

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume ");
        if (mNeedRequestPermissions) {
            Log.d(TAG, "onResume need request permissions before start RecordService!");
            return;
        }
        mAudioManager.requestAudioFocus(mAudioFocusListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        mRecogniseController.setOnRecogniseStatusChangedListener(this);
        mRecogniseController.start();
        mRecogniseController.setRecordingState(true);
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
        if (mNeedRequestPermissions) {
            Log.d(TAG, "need request permissions before start RecordService!");
            return;
        }
        mRecogniseController.stop();
        mRecogniseController.unRegistRecogniseStatusChangedListener();
        mAudioManager.abandonAudioFocus(mAudioFocusListener);
        Log.d(TAG, "onPause close recog");
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private static String TAG = "HelloTriggerActivity";

    @Override
    public void onRecogniseInit() {
        /*if (mTitleText != null && mTextTitleArray != null) {
            mTitleText.setText(mTextTitleArray[0]);
            mTitleText.startAnimation(textTitleInAnim);
        }*/
        if (mContentText != null) {

            mContentText.setText(R.string.trigger_prompt);
        }

        Log.d(TAG, "setRecording true");
        mRecogniseController.setRecordingState(true);
    }

    @Override
    public void onRecogniseStart() {
        Log.d(TAG, "onRecogniseStart stratRipple");
        mRippleView.stratRipple();
    }

    @Override
    public void onRecogniseWait() {
        if (mTitleText != null && mTextTitleArray != null) {
            mTitleText.setText(mTextTitleArray[1]);
        }
        if (mContentText != null) {
            mContentText.setText(null);
        }
    }

    @Override
    public void onRecogniseUnkown() {
        if (mTitleText != null && mTextTitleArray != null) {
            mTitleText.setText(mTextTitleArray[2]);
            mTitleText.startAnimation(textTitleInAnim);
        }
    }

    @Override
    public void onRecogniseStop() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onRecogniseSuccess() {
        if (mTitleText != null && mTextTitleArray != null) {
            mTitleText.setText(mTextTitleArray[2]);
            mTitleText.startAnimation(textTitleInAnim);
        }
        if (mContentText != null) {
            StringBuffer sb = new StringBuffer();
            int tag = 0;
            int index = 0;
            for (int i = index; i <= mTextContentArray.length; i++) {
                sb.append(mTextContentArray[i]);
                sb.append("\n");
                tag++;
                if (tag >= textShowItemNum) {
                    index += textShowItemNum;
                    tag = 0;
                    break;
                }
            }
            sb.append("......");
            mContentText.setText(sb);

            mContentText.startAnimation(textContentInAnim);
        }
        mRecogniseController.startCommand();
        mVibrator.vibrate(25);

    }

    @Override
    public void onRecogniseFailed() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onRecogniseError(String errorStr) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onCommandSuccess(Bundle command) {
        String commandString = command.getString("command_result");
        mVibrator.vibrate(35);
        if (mRippleView != null) {
            mRippleView.stopRipple();
        }
        if (commandString == null) {
            Toast.makeText(this, "default ", Toast.LENGTH_LONG).show();
            Log.d(TAG, "commandString == null)");
            finish();
        } else {
            Toast.makeText(this, commandString, Toast.LENGTH_LONG).show();
            Uri uri = Uri.parse("smsto:13800000000");
            Intent intent = new Intent(Intent.ACTION_SENDTO, uri);
            intent.putExtra("sms_body", "The SMS text");
            startActivity(intent);
            finish();
        }
    }

    private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange) {

            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_LOSS:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    if (mRecogniseController != null) {
                        Log.d(TAG, "onAudioFocusChange  loss");
                        finish();
                    }

                    break;
                default:
                    break;
            }

        }
    };

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult");
        switch (requestCode) {
            case PermissionUtils.HELLOTRIGGER_PERMISSIONS_REQUEST_CODE: {
                mNeedRequestPermissions = PermissionUtils.requestPermissionsResult(requestCode,permissions,grantResults,this);
                return;
            }
        }
    }
}
