
package com.sprd.engineermode.connectivity.fm;

import java.util.ArrayList;

import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import com.android.fmradio.FmNative;

import android.media.AudioDevicePort;
import android.media.AudioDevicePortConfig;
import android.media.AudioFormat;
import android.media.AudioMixPort;
import android.media.AudioPatch;
import android.media.AudioPort;
import android.media.AudioPortConfig;
import android.media.AudioSystem;
import android.media.MediaRecorder;
import android.app.Activity;
/*BEGIN BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
import com.android.fmradio.FmManagerSelect;
import com.android.fmradio.FmConstants.AudioPath;
import com.android.fmradio.FmConstants;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
/*END BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
import android.widget.TextView;
import com.sprd.engineermode.R;

public class EarphoneFragment extends AbsFMFragment implements OnClickListener {

    private static final String TAG = "EarphoneFragment";
    private static final int RADIO_AUDIO_DEVICE_WIRED_HEADSET = 0;

    private static final float POWER_UP_START_FREQUENCY = 87.5f;
    /*BEGIN BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
    private static final int FM_HEADSET = 0;
    private static final int FM_SPEAKER = 1;
    private static final int HEADSET_DIALOG = 1;
    private static final int HEADSET_PLUG_IN = 1;

    private int mSwitchCounts = 0;
    private int mValueHeadSetPlug = 1;

    private FmManagerSelect mFmManager = null;
    private AudioManager mAudioManager = null;

    private Dialog mDialog = null;
    /*END BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
    private Button mEarphonePlayButton;
    private Button mExtrovertedPlayButton;
    private Button mSwitchEarphoneaAndExtroverButton;
    private Button mRdsBlerButton;
    private EditText mSwitchEarphoneaAndExtroverEditText;

    private ProgressDialog mSearchStationDialog = null;
    private boolean isFmOn = false;
    private Context mContext;

    //begin 562936 add by suyan.yang 2016.05.19
    private EditText mChannelEdit;
    private Button mChannelSet;
    private String mChannelValue;

    //end 562936 add by suyan.yang 2016.05.19
    private Handler switchEarphoneaAndExtroverHandler = new Handler();

    public Runnable switchEarphoneaAndExtroverRunnable = new Runnable() {

        @Override
        public void run() {
            /*BEGIN BUG570326 zhijie.yang 2016/06/07*/
            if (mSwitchCounts >= getSwitchCounts() * 2) {
            /*END BUG570326 zhijie.yang 2016/06/07*/
                mSwitchCounts = 0;
                mSearchStationDialog.cancel();
                return;
            }
            setFMPlayerRoute(mSwitchCounts % 2);
            mSwitchCounts++;
            switchEarphoneaAndExtroverHandler.postDelayed(switchEarphoneaAndExtroverRunnable, 2000);
        }
    };

    /*BEGIN BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
    private BroadcastReceiver earphonePluginReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent earphoneIntent) {
            if (earphoneIntent != null && earphoneIntent.getAction() != null) {
                if (earphoneIntent.getAction().equalsIgnoreCase(
                        Intent.ACTION_HEADSET_PLUG)) {
                    Log.d(TAG, "earphonePluginReceiver onReceiver");
                    mValueHeadSetPlug = (earphoneIntent.getIntExtra("state", -1) == HEADSET_PLUG_IN) ? 0
                            : 1;
                    if (mValueHeadSetPlug == 0) {
                        if (mDialog != null) {
                            mDialog.cancel();
                        }
                    } else if (mValueHeadSetPlug == 1) {
                        if (mSearchStationDialog != null) {
                            mSearchStationDialog.cancel();
                        }
                        mSwitchCounts = 0;
                        mExtrovertedPlayButton.setEnabled(true);
                        mEarphonePlayButton.setEnabled(true);
                        new Thread(new Runnable() {
                            public void run() {
                                powerOffFM();
                            }
                        }).start();
                        CreateDialog(HEADSET_DIALOG).show();
                    }
                }
            }
        }
    };

    /*BEGIN 566718 zhijie.yang 2016/05/26*/
    private BroadcastReceiver volumeReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            if (intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE)
                    && intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE)) {
                int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                //begin bug588672 modify by suyan.yang 20160816
                if (streamType == FmConstants.STREAM_MUSIC) {
                //end bug588672 modify by suyan.yang 20160816
                    int index = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, -1);
                    if (index != -1) {
                        Log.d(TAG, "stream type " + streamType + "value " + index);
                        setVolume(index);
                    }
                }
            }
        }
    };

    public boolean setVolume(int volume) {
        boolean value = false;
        Log.d(TAG, "setVolume FM_Volume=" + volume);
        if (isFmOn) {
            mAudioManager.setParameter("FM_Volume", "" + volume);
            if (0 == volume) {
                mFmManager.setMute(true);
            } else {
                mFmManager.setMute(false);
            }
            value = true;
        }
        return value;
    }
    /*END 566718 zhijie.yang 2016/05/26*/

    /**
     * Check the headset is plug in or plug out
     *
     * @return true for plug in; false for plug out
     */
    private boolean isHeadSetIn() {
        return (0 == mValueHeadSetPlug);
    }

    protected Dialog CreateDialog(int id) {
        switch (id) {
            case HEADSET_DIALOG: {
              //begin 564042 modify by suyan.yang 2016.05.18
                if (mDialog == null) {
                    AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
                    mDialog = builder.setTitle(R.string.fm_dialog_tittle)
                            .setMessage(R.string.fm_dialog_message).create();
                    mDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
                        public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                            if (keyCode == KeyEvent.KEYCODE_BACK) {
                                if (mDialog != null)
                                    mDialog.cancel();
                                return true;
                            } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
                                return true;
                            }
                            return false;
                        }
                    });
                }
              //end 564042 modify by suyan.yang 2016.05.18
                return mDialog;
            }
        }
        return null;
    }
    /*END BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        mContext = getActivity();
        //begin bug588672 modify by suyan.yang 20160816
        getActivity().setVolumeControlStream(FmConstants.STREAM_MUSIC);
        //begin bug588672 modify by suyan.yang 20160816
        mFmManager = new FmManagerSelect(mContext);
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        mContext.registerReceiver(earphonePluginReceiver, filter);
        /** BEGIN 566718 zhijie.yang 2016/05/26 **/
        IntentFilter filter2 = new IntentFilter();
        filter2.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        mContext.registerReceiver(volumeReceiver, filter2);
        /** END 566718 zhijie.yang 2016/05/26 **/
        if (!isHeadSetIn()) {
            CreateDialog(HEADSET_DIALOG).show();
        }
        startRender();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return initView(inflater);
    }

    @SuppressWarnings("deprecation")
    private View initView(LayoutInflater inflater) {
        View view = inflater.inflate(R.layout.earphone_fragment_main, null);
        mEarphonePlayButton = (Button) view.findViewById(R.id.earphone_play);
        mExtrovertedPlayButton = (Button) view.findViewById(R.id.extroverted_play);
        mSwitchEarphoneaAndExtroverEditText = (EditText) view
                .findViewById(R.id.earphone_and_extroverted_switch_edittext);
        mEarphonePlayButton.setOnClickListener(this);
        mExtrovertedPlayButton.setOnClickListener(this);
        mSwitchEarphoneaAndExtroverButton = (Button) view
                .findViewById(R.id.earphone_and_extroverted_switch_start_switch);
        mSwitchEarphoneaAndExtroverButton.setOnClickListener(this);
        mRdsBlerButton=(Button)view.findViewById(R.id.rds_bler_btn);
        mRdsBlerButton.setOnClickListener(this);
        //begin 562936 add by suyan.yang 2016.05.19
        mChannelEdit=(EditText) view
                .findViewById(R.id.channel_edittext);
        mChannelSet=(Button) view
              .findViewById(R.id.channel_switch);
        mChannelSet.setOnClickListener(this);
        disableControl();
        //end 562936 add by suyan.yang 2016.05.19
        initProgressDialog();
        return view;
    }

    public void initProgressDialog() {
        mSearchStationDialog = new ProgressDialog(getActivity());
        mSearchStationDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mSearchStationDialog.setMessage(getResources().getString(
                R.string.earphone_and_extroverted_switch_underway));
        mSearchStationDialog.setIndeterminate(false);
        mSearchStationDialog.setCancelable(false);
        mSearchStationDialog.setButton(getResources().getString(R.string.fm_cancel),
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        switchEarphoneaAndExtroverHandler
                                .removeCallbacks(switchEarphoneaAndExtroverRunnable);
                        //begin bug570866 add by suyan.yang 20160608
                        mSwitchCounts = 0;
                        //end bug570866 add by suyan.yang 20160608
                        dialog.dismiss();
                    }
                });
        mSearchStationDialog.setOnKeyListener(new OnKeyListener() {
            public boolean onKey(DialogInterface dialoge, int keyCode,
                    KeyEvent event) {
                if (KeyEvent.KEYCODE_SEARCH == keyCode || KeyEvent.KEYCODE_HOME == keyCode) {
                    return true;
                }
                return false;
            }
        });
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            /*BEGIN BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
            case R.id.earphone_play:
                if (!isHeadSetIn()) {
                    CreateDialog(HEADSET_DIALOG).show();
                    return;
                }
                mExtrovertedPlayButton.setEnabled(true);
                mEarphonePlayButton.setEnabled(false);
                new Thread(new Runnable() {
                    public void run(){
                        if (!isFmOn) {
                            powerOnFM();
                        }
                        setFMPlayerRoute(FM_HEADSET);
                    }
                }).start();
                break;
            case R.id.extroverted_play:
                if (!isHeadSetIn()) {
                    CreateDialog(HEADSET_DIALOG).show();
                    return;
                }
                mEarphonePlayButton.setEnabled(true);
                mExtrovertedPlayButton.setEnabled(false);
                new Thread(new Runnable() {
                    public void run() {
                        if (!isFmOn) {
                            powerOnFM();
                        }
                        setFMPlayerRoute(FM_SPEAKER);
                    }
                }).start();
                break;
            case R.id.earphone_and_extroverted_switch_start_switch:
                if (!isHeadSetIn()) {
                    CreateDialog(HEADSET_DIALOG).show();
                    return;
                }
                if (mSwitchEarphoneaAndExtroverEditText.getText().toString().equals("")) {
                    Toast.makeText(
                            getActivity(),
                            getResources().getString(
                                    R.string.earphone_and_extroverted_switch_counts_string),
                            Toast.LENGTH_LONG).show();
                    return;
                }
                mExtrovertedPlayButton.setEnabled(true);
                mEarphonePlayButton.setEnabled(true);
                new Thread(new Runnable() {
                    public void run() {
                        if (!isFmOn) {
                            powerOnFM();
                        }
                        startSwitchEarphoneAndExtroverted();
                    }
                }).start();
                if (!mSearchStationDialog.isShowing()) {
                    mSearchStationDialog.show();
                }
                break;
                /*END BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
              //begin 562936 add by suyan.yang 2016.05.19
            case R.id.channel_switch:
                mChannelValue=mChannelEdit.getText().toString().trim();
                if(mChannelValue.equals("")){
                    Toast.makeText(
                            getActivity(),
                            "please input channel value",
                            Toast.LENGTH_LONG).show();
                    return;
                }
                enableControl();
                break;
              //end 562936 add by suyan.yang 2016.05.19
            case R.id.rds_bler_btn:
                Intent bler_intent=new Intent(mContext,FMRdsBlerActivity.class);
                startActivity(bler_intent);
                break;
            default:
                break;
        }
    }
    //begin 562936 add by suyan.yang 2016.05.19
    private void disableControl(){
        mEarphonePlayButton.setEnabled(false);
        mExtrovertedPlayButton.setEnabled(false);
        mSwitchEarphoneaAndExtroverEditText.setEnabled(false);
        mSwitchEarphoneaAndExtroverButton.setEnabled(false);
        mChannelEdit.setEnabled(true);
        mChannelSet.setEnabled(true);
    }
    private void enableControl(){
        mEarphonePlayButton.setEnabled(true);
        mExtrovertedPlayButton.setEnabled(true);
        mSwitchEarphoneaAndExtroverEditText.setEnabled(true);
        mSwitchEarphoneaAndExtroverButton.setEnabled(true);
        mChannelEdit.setEnabled(false);
        mChannelSet.setEnabled(false);
    }
    //end 562936 add by suyan.yang 2016.05.19

    public void startSwitchEarphoneAndExtroverted() {
        switchEarphoneaAndExtroverHandler.post(switchEarphoneaAndExtroverRunnable);
    }

    public int getSwitchCounts() {
        return Integer.parseInt(mSwitchEarphoneaAndExtroverEditText.getText().toString());
    }

    /*BEGIN BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
    public void setFMPlayerRoute(int headsetOrSpeaker) {
        Log.d(TAG, "setFMPlayerRoute: " + headsetOrSpeaker);
        if (mFmManager != null) {
            if (headsetOrSpeaker == FM_HEADSET) {
                mFmManager.setSpeakerEnable(AudioPath.FM_AUDIO_PATH_HEADSET, false);
            } else {
                mFmManager.setSpeakerEnable(AudioPath.FM_AUDIO_PATH_SPEAKER, true);
            }
            mFmManager.setMute(true);
          //begin 562936 modify by suyan.yang 2016.05.19
            mFmManager.setRdsMode(true, false);
            Log.d(TAG,"FM channel is:"+mChannelValue);
            mFmManager.tuneRadio(Float.valueOf(mChannelValue));
            mFmManager.setRdsMode(true, true);
          //end 562936 modify by suyan.yang 2016.05.19
            mFmManager.setMute(false);
        } else {
            Log.d(TAG, "mFmManager is null");
        }
    }

    public void powerOnFM() {
        Log.d(TAG, "startPowerUp");
        boolean value = false;
        if (mFmManager != null) {
            mFmManager.setMute(true);
            value = mFmManager.openDev();
            if (!value) {
                Log.d(TAG, "powerUp fail");
                isFmOn = false;
                return;
            }
            value = mFmManager.powerUp(POWER_UP_START_FREQUENCY);
            if (!value) {
                Log.e(TAG, "powerUp fail ");
                isFmOn = false;
                return;
            }
            isFmOn = true;
            Log.d(TAG, "FM open suceess!");
        } else {
            Log.d(TAG, "mFmManager is null");
        }
        /** BEGIN 566718 zhijie.yang 2016/05/26 **/
        int volume = mAudioManager.getStreamVolume(FmConstants.STREAM_MUSIC);
        Log.d(TAG,"get current valoume is: " + volume);
        setVolume(volume);
        /** END 566718 zhijie.yang 2016/05/26 **/
    }

    private synchronized void startRender() {
        Log.d(TAG, "startRender ");
        if (mFmManager != null) {
            mFmManager.setAudioPathEnable(AudioPath.FM_AUDIO_PATH_HEADSET, true);
        } else {
            Log.d(TAG, "mFmManager is null");
        }
    }

    private synchronized void stopRender() {
        Log.d(TAG, "stopRender");
        if (mFmManager != null) {
            mFmManager.setAudioPathEnable(AudioPath.FM_AUDIO_PATH_NONE, false);
        } else {
            Log.d(TAG, "mFmManager is null");
        }
    }

    public void powerOffFM() {
        Log.d(TAG, "power off fm");
        if (mFmManager != null) {
            mFmManager.setMute(true);
            mFmManager.setRdsMode(false, false);
            mFmManager.powerDown();
            mFmManager.closeDev();
            isFmOn = false;
        } else {
            Log.d(TAG, "mFmManager is null");
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy...");
        stopRender();
        powerOffFM();
        switchEarphoneaAndExtroverHandler.removeCallbacks(switchEarphoneaAndExtroverRunnable);
        if (earphonePluginReceiver != null) {
            mContext.unregisterReceiver(earphonePluginReceiver);
        }
        /** BEGIN 566718 zhijie.yang 2016/05/26 **/
        if (volumeReceiver != null) {
            mContext.unregisterReceiver(volumeReceiver);
        }
        /** END 566718 zhijie.yang 2016/05/26 **/
        //begin 564042 add by suyan.yang 2016.05.18
        if (mDialog != null) {
            mDialog.cancel();
        }
        //end 564042 add by suyan.yang 2016.05.18

    }
    /*END BUG560516 zhijie.yang 2016/05/09 sprd:modify to fit the botong and marlin of fm */
}
