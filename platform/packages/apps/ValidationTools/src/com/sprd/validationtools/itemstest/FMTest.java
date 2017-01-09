
package com.sprd.validationtools.itemstest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.ArrayList;

import org.apache.http.util.EncodingUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnKeyListener;
import android.graphics.Color;
import android.os.Bundle;
import android.os.EnvironmentEx;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.Const;
import com.sprd.validationtools.R;
import com.android.fmradio.FmManagerSelect;
import com.android.fmradio.FmNative;
import com.android.fmradio.FmConstants.AudioPath;
import com.sprd.validationtools.sqlite.EngSqlite;
import android.os.Handler;
import android.media.AudioManager;
import android.os.Message;
import android.os.HandlerThread;
import android.os.Debug;

import android.media.AudioDevicePort;
import android.media.AudioDevicePortConfig;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.AudioManager.OnAudioPortUpdateListener;
import android.media.AudioMixPort;
import android.media.AudioPatch;
import android.media.AudioPort;
import android.media.AudioPortConfig;
import android.media.AudioRecord;
import android.media.AudioSystem;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.app.Activity;

public class FMTest extends BaseActivity {

    private TextView waveBandText;
    private Button searchButton;
    private static final String RADIO_DEVICE = "/dev/radio0";
    private int freq = 88700;
    private static final int TRANS_MULT = 10;
    private static final String SPRD_FM_TEST_FILE = "fm2.txt";
    private static final String PHONE_STORAGE_PATH = "/data/data/com.sprd.validationtools/";
    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
    private boolean mIsNativeScanning = false;
    // Headset plug state (0:long antenna plug in, 1:long antenna plug out)
    private int mValueHeadSetPlug = 1;
    // Headset
    private static final int HEADSET_PLUG_IN = 1;
    private Dialog mDialog = null;
    private static final int HEADSET_DIALOG = 1;
    protected Button mPassButton;
    protected Button mFailButton;
    private static final int TEXT_SIZE = 30;
    protected String mTestCaseName = null;
    private int isFullTest = 0;
    private int fullTestActivityId;
    private static String TAG = "StartFMTest";
    private static final int MSG_SEARCH_FINISH = 3;
    private static final int MSG_POWER_UP = 4;
    ProgressDialog mSearchStationDialog = null;
    private Context mContext;

    private EngSqlite mEngSqlite;
    private int groupId;

    private static final int MSG_SET_VOLUME = 1;
    private int freqSearch = 0;
    private boolean mHasPowerUp = false;

    private static final float POWER_UP_START_FREQUENCY = 87.5f;

    private FmManagerSelect mFmManager = null;
    private AudioManager mAudioManager = null;
    private boolean mIsRender = false;

    boolean isOpen = false;
    private Runnable mR = new Runnable() {
        public void run() {
            showResultDialog(getString(R.string.wb_text_info));
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.fm_test);
        setTitle(R.string.fm_test);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
        mContext = this.getApplicationContext();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        mFmManager = new FmManagerSelect(this);
        this.setVolumeControlStream(AudioManager.STREAM_MUSIC);
        groupId = this.getIntent().getIntExtra("groupId", 0);
        mPassButton = (Button) findViewById(R.id.btn_pass);
        mFailButton = (Button) findViewById(R.id.btn_fail);
        mPassButton.setVisibility(View.GONE);
        mFailButton.setVisibility(View.GONE);
        mEngSqlite = EngSqlite.getInstance(this);
        findViewById(R.id.btn_retry).setVisibility(View.GONE);
        waveBandText = (TextView) findViewById(R.id.wave_band_textView);
        searchButton = (Button) findViewById(R.id.search_button);
        searchButton.setTextSize(TEXT_SIZE);
        searchButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "searchButton, onClick");
                //SPRD: for bug537927, Did not insert the headset, click the search button, prompt Not power up.
                //if (!isHeadsetExists()) {
                if (!isHeadSetIn()) {
                    showDialog(HEADSET_DIALOG);
                    return;
                }
                if (mHasPowerUp) {
                    stopRender();
                    waveBandText.setText(getResources().getString(R.string.fm_test_search_station));
                    //FmNative.setMute(true);
                    mFmManager.setMute(true);
                    startSearchstation();
                } else {
                    Toast.makeText(FMTest.this, R.string.fm_not_power_up, Toast.LENGTH_SHORT)
                            .show();
                    return;
                }
            }
        });
        mSearchStationDialog = new ProgressDialog(this);
        mSearchStationDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mSearchStationDialog.setMessage(getResources().getString(R.string.fm_test_search_station));
        mSearchStationDialog.setIndeterminate(false);
        mSearchStationDialog.setCancelable(false);
        mSearchStationDialog.setOnKeyListener(new OnKeyListener() {
            public boolean onKey(DialogInterface dialoge, int keyCode,
                    KeyEvent event) {
                if (KeyEvent.KEYCODE_SEARCH == keyCode || KeyEvent.KEYCODE_HOME == keyCode) {
                    return true;
                }
                return false;
            }
        });
        //if (!isHeadsetExists()) {
        if (!isHeadSetIn()) {
            showDialog(HEADSET_DIALOG);
        }
    }

    private synchronized void startRender() {
        Log.d(TAG, "startRender ");
        //FmNative.setMute(true);
        mFmManager.setMute(true);
        //AudioSystem.setDeviceConnectionState(
        //        AudioManager.DEVICE_OUT_FM_HEADSET,
        //        AudioSystem.DEVICE_STATE_AVAILABLE, "", "");
        mFmManager.setAudioPathEnable(AudioPath.FM_AUDIO_PATH_HEADSET, true);
        AudioSystem.setForceUse(AudioSystem.FOR_FM, AudioSystem.FORCE_NONE);
        mIsRender = true;
    }

    class SearchStationThread extends Thread {
        public void run() {
            searchStation();
        };
    };

    private void startSearchstation() {
        if (!mSearchStationDialog.isShowing()) {
            SearchStationThread mSearchStationThread = new SearchStationThread();
            mSearchStationDialog.show();
            mSearchStationThread.start();
        }
    }

    public boolean setVolume(int volume) {
        boolean value = false;
        Log.d(TAG, "setVolume FM_Volume=" + volume);
        if (isOpen) {
            mAudioManager.setParameter("FM_Volume", "" + volume);
            if (0 == volume) {
                //FmNative.setMute(true);
                mFmManager.setMute(true);
            } else {
                //FmNative.setMute(false);
                mFmManager.setMute(false);
            }
            value = true;
        }
        return value;
    }

    private BroadcastReceiver volumeReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            if (intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE)
                    && intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE)) {
                int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                if (streamType == AudioManager.STREAM_MUSIC) {
                    int index = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, -1);
                    if (index != -1) {
                        Log.d(TAG, "stream type " + streamType + "value " + index);
                        setVolume(index);
                    }
                }
            }
        }
    };

    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case HEADSET_DIALOG: {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
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
                return mDialog;
            }
        }
        return null;
    }

    private BroadcastReceiver earphonePluginReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent earphoneIntent) {
            if (earphoneIntent != null && earphoneIntent.getAction() != null) {
                if (earphoneIntent.getAction().equalsIgnoreCase(
                        Intent.ACTION_HEADSET_PLUG)) {
                    Log.d(TAG, "earphonePluginReceiver onReceiver");
                    //int st = 0;
                    //st = earphoneIntent.getIntExtra("state", 0);
                    mValueHeadSetPlug = (earphoneIntent.getIntExtra("state", -1) == HEADSET_PLUG_IN) ? 0 : 1;
                    //if (st > 0) {
                    if (mValueHeadSetPlug == 0) {
                        if (mDialog != null) {
                            mDialog.cancel();
                        }
                        startPowerUpFm();
                    }
                    //else if (st == 0) {
                    else if (mValueHeadSetPlug == 1) {
                        stopRender();
                        powerOffFM();
                        mHasPowerUp = false;
                        waveBandText.setText("");
                        //SPRD: for bug538007, the serach dialog is existing all the time.
                        mSearchStationDialog.cancel();
                        showDialog(HEADSET_DIALOG);
                    }
                }
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        registerReceiver(earphonePluginReceiver, filter);
        IntentFilter filter2 = new IntentFilter();
        filter2.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        registerReceiver(volumeReceiver, filter2);
        //startPowerUpFm();
    }

    /* SPRD: fix bug 363908 change FM power up protect method @{ */
    private void startPowerUpFm() {
        new StartPowerUpThread().start();
    }

    class StartPowerUpThread extends Thread {
        public void run() {
            startPowerUp();
        };
    };

    private void startPowerUp() {
        Log.d(TAG, "startPowerUp");
        boolean value = false;
        //FmNative.setMute(true);
        mFmManager.setMute(true);
        //value = FmNative.openDev();
        value = mFmManager.openDev();
        if (!value) {
            isOpen = false;
            Log.e(TAG, "openDev fail ");
            return;
        }
        //value = FmNative.powerUp(POWER_UP_START_FREQUENCY);
        value = mFmManager.powerUp(POWER_UP_START_FREQUENCY);
        if (!value) {
            isOpen = false;
            Log.e(TAG, "powerUp fail ");
            return;
        }
        isOpen = true;
        Log.d(TAG, "sendMessage MSG_POWER_UP");
        mHandler.sendMessage(mHandler.obtainMessage(MSG_POWER_UP));
    }

    /* @} */

    /* SPRD: fix bug350932 changer FmManager @{ */
    public void powerUpComplete() {
        Log.d(TAG, " powerUpComplete");
        mHasPowerUp = true;
        //if (isHeadsetExists()) {
        if (isHeadSetIn()) {
            startPlayFm();
        }
    }

    private void startPlayFm() {
        Log.d(TAG, "startPlayFm");
        int freq = -1;
        if (hasSDCard()) {
            freq = readFromSdcard();
            if (freq != -1) {
                saveToPhone(freq);
                Log.d(TAG, "sync freq from sdcard,freq=" + freq);
            }
        } else {
            freq = readFromPhone();
        }
        if (freq == -1) {
            waveBandText.setText(getResources().getString(R.string.fm_test_search_station));
            startSearchstation();
        } else {
            try {
                Thread.sleep(600);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            try {
                float fmFre = (float) (freq * 1.0 / 10);
                //FmNative.setMute(true);
                mFmManager.setMute(true);
                //if (FmNative.tune(fmFre)) {
                if (mFmManager.tuneRadio(fmFre)) {
                    int volume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                    mAudioManager.setParameter("FM_Volume", "" + 0);
                    startRender();
                    //FmNative.setMute(false);
                    mFmManager.setMute(false);
                    mHandler.sendEmptyMessageDelayed(MSG_SET_VOLUME, 200);
                }
                waveBandText.setText(getResources().getString(R.string.wb_text) + " "
                        + fmFre + " MHz");
                mHandler.removeCallbacks(mR);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    /* @} */

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_SEARCH_FINISH:
                    //SPRD: for bug538007, the serach dialog is existing all the time.
                    if (!isHeadSetIn()) {
                        break;
                    }
                    Bundle data = msg.getData();
                    int freq = data.getInt("freq");
                    saveToPhone(freq);
                    saveToSdCard(freq);
                    try {
                        Thread.sleep(600);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    float fmFreq = (float) (freq * 1.0 / 10.0);
                    //FmNative.setMute(true);
                    mFmManager.setMute(true);
                    //if (FmNative.tune(fmFreq)) {
                    if (mFmManager.tuneRadio(fmFreq)) {
                        startRender();
                        int volume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                        mAudioManager.setParameter("FM_Volume", "" + volume);
                        //FmNative.setMute(false);
                        mFmManager.setMute(false);
                    }
                    waveBandText.setText(getResources().getString(R.string.wb_text) + " "
                            + fmFreq + " MHz");
                    break;
                case MSG_POWER_UP:
                    powerUpComplete();
                    break;
                case MSG_SET_VOLUME:
                    int volume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                    mAudioManager.setParameter("FM_Volume", "" + volume);
                    break;
                default:
            }
        }
    };

    private synchronized void stopRender() {
        //AudioSystem.setDeviceConnectionState(
        //        AudioManager.DEVICE_OUT_FM_HEADSET,
        //        AudioSystem.DEVICE_STATE_UNAVAILABLE, "", "");
        mFmManager.setAudioPathEnable(AudioPath.FM_AUDIO_PATH_NONE, false);
        Log.d(TAG, "stopRender");
        mIsRender = false;
    }

    private boolean isRender() {
        return mIsRender;
    }

    public void powerOffFM() {
        Log.d(TAG,"power off fm device");
        new Thread(new Runnable() {
            public void run() {
                //FmNative.setMute(true);
                //FmNative.setRds(false);
                //FmNative.powerDown(0);
                //FmNative.closeDev();
                mFmManager.setMute(true);
                mFmManager.setRdsMode(false, false);
                if(mIsNativeScanning){
                    mFmManager.stopScan();
                    mIsNativeScanning =false;
                }
                mFmManager.powerDown();
                mFmManager.closeDev();
                isOpen = false;
            }
        }).start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG,"onPause");
        stopRender();
        powerOffFM();
        mHasPowerUp = false;
        mSearchStationDialog.cancel();
        unregisterReceiver(earphonePluginReceiver);
        unregisterReceiver(volumeReceiver);
    }

    private boolean isHeadsetExists() {
        char[] buffer = new char[1024];
        int newState = 0;
        FileReader file = null;
        try {
            file = new FileReader(HEADSET_STATE_PATH);
            int len = file.read(buffer, 0, 1024);
            newState = Integer.valueOf((new String(buffer, 0, len)).trim());
        } catch (FileNotFoundException e) {
            Log.e("FMTest", "This kernel does not have wired headset support");
        } catch (Exception e) {
            Log.e("FMTest", "", e);
        } finally {
            if (file != null) {
                try {
                    file.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        return newState != 0;
    }

    /**
     * Check the headset is plug in or plug out
     *
     * @return true for plug in; false for plug out
     */
    private boolean isHeadSetIn() {
        return (0 == mValueHeadSetPlug);
    }

    private void searchStation() {
        //short[] stationsInShort = null;
        int[] stationsInShort = null;
        int saveFreq = 0;
        //FmNative.setMute(true);
        //FmNative.setRds(false);
        //stationsInShort = FmNative.autoScan(0);
        //FmNative.setRds(true);
        mFmManager.setMute(true);
        mFmManager.setRdsMode(false, false);
        stationsInShort = mFmManager.autoScan(0);
        mIsNativeScanning = true;
        mFmManager.setRdsMode(true, false);
        if (stationsInShort == null) {
            Log.e(TAG, "cannot find any freq");
            mSearchStationDialog.cancel();
            mHandler.post(new Runnable() {
                public void run() {
                    waveBandText.setText("n/a");
                }
            });
            return;
        }

        Bundle data = new Bundle();
        if (mHandler != null) {
            //data.putInt("freq", (int)stationsInShort[stationsInShort.length - 1]);
            data.putInt("freq", stationsInShort[stationsInShort.length - 1]);
            Message msg = mHandler.obtainMessage(MSG_SEARCH_FINISH);
            msg.setData(data);
            mHandler.sendMessage(msg);
            Log.d(TAG, "MSG_SEARCH_FINISH");
        }
        mSearchStationDialog.cancel();
    }

    private boolean hasSDCard() {
        boolean hasSDCard = false;
        hasSDCard = Environment.getExternalStorageState(
                new File(System.getenv("PHYSICAL_STORAGE"))).equals(
                Environment.MEDIA_MOUNTED);
        return hasSDCard;
    }

    private int readFromPhone() {
        String readString = null;
        File storefile = new File(PHONE_STORAGE_PATH
                + SPRD_FM_TEST_FILE);
        FileInputStream fis = null;
        int freq = -1;
        try {
            if (!storefile.exists()) {
                Log.d(TAG, "file not exists,return -1");
            } else {
                fis = new FileInputStream(storefile);
                int length = fis.available();
                byte buffer[] = new byte[length];
                fis.read(buffer);
                readString = EncodingUtils.getString(buffer, "UTF-8");
                if (readString.equals("nofm") || readString.length() < 4)
                    readString = null;
            }
            Log.d(TAG, "readFromPhone freq=" + readString);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        if (readString != null) {
            try {
                freq = Integer.parseInt(readString.trim());
            } catch (Exception e) {
                storefile.delete();
                freq = -1;
            }
        }
        return freq;
    }

    private int readFromSdcard() {
        int freq = -1;
        if (hasSDCard()) {
            String readString = null;
            File storefile = new File(EnvironmentEx.getExternalStoragePath(), SPRD_FM_TEST_FILE);
            FileInputStream fis = null;
            try {
                if (!storefile.exists()) {
                    Log.d(TAG, "file not exists,return -1");
                } else {
                    fis = new FileInputStream(storefile);
                    int length = fis.available();
                    byte buffer[] = new byte[length];
                    fis.read(buffer);
                    readString = EncodingUtils.getString(buffer, "UTF-8");
                    Log.d(TAG, readString);
                    if (readString.equals("nofm") || readString.length() < 4) {
                        readString = null;
                    }
                }
                Log.d(TAG, "readFromSdcard freq=" + readString);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (fis != null) {
                    try {
                        fis.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
            if (readString != null) {
                try {
                    freq = Integer.parseInt(readString.trim());
                } catch (Exception e) {
                    storefile.delete();
                    freq = -1;
                }
            }
        }
        return freq;
    }

    private void saveToPhone(int freq) {
        String s = String.valueOf(freq);
        File storefile = new File(PHONE_STORAGE_PATH
                + SPRD_FM_TEST_FILE);
        FileOutputStream fos = null;
        try {
            if (!storefile.exists()) {
                Log.d(TAG, "file not exists,create");
                storefile.createNewFile();
            }
            fos = new FileOutputStream(storefile);
            byte[] buffer = s.getBytes();
            fos.write(buffer);
            Log.d(TAG, "saveToPhone path=" + storefile.getPath() + " freq=" + freq);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void saveToSdCard(int freq) {
        if (hasSDCard()) {
            String s = String.valueOf(freq);
            File storefile = new File(EnvironmentEx.getExternalStoragePath(), SPRD_FM_TEST_FILE);
            FileOutputStream fos = null;
            try {
                if (!storefile.exists()) {
                    Log.d(TAG, "file not exists,create");
                    storefile.createNewFile();
                }
                fos = new FileOutputStream(storefile);
                byte[] buffer = s.getBytes();
                fos.write(buffer);
                Log.d(TAG, "saveToSdCard path=" + storefile.getPath() + " freq=" + freq);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (fos != null) {
                    try {
                        fos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    @Override
    public void onDestroy() {
        mHandler.removeCallbacks(mR);
        super.onDestroy();
    }
}
