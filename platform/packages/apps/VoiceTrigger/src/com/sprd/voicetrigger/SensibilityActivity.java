package com.sprd.voicetrigger;

import android.app.ActionBar;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.sprd.voicetrigger.provider.ContentProviderHelper;

public class SensibilityActivity extends Activity {
    public static final int SENSIBILITY_DEFAULT_VALUE = 50;
    private static final String TAG = "Sensibility";
    private SeekBar mPointer;
    private int mSensibilityValue;
    private final OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            Log.d(TAG, "onProgressChanged progress = " + progress);
            mSensibilityValue = progress;
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            // do nothing
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            Log.d(TAG,"setSensibilityValue = " + mSensibilityValue);
            ContentProviderHelper.setSensibilityValue(getBaseContext(), mSensibilityValue);
            ((VoiceTriggerApp) getApplication()).stopRecognition();
            ((VoiceTriggerApp) getApplication()).startRecognition();
        }
    };

    @Override
    public void onCreate(Bundle bundle) {
        Log.d(TAG, "onCreat ");
        super.onCreate(bundle);
        setContentView(R.layout.voicetrigger_sensibility);
        ActionBar actionBar = getActionBar();
        actionBar.setTitle(R.string.audio_voicetrigger_sensibility);
        actionBar.setDisplayHomeAsUpEnabled(true);

        mPointer = (SeekBar) findViewById(R.id.sensor_indicator);
        mPointer.setMax(100);
        mPointer.setOnSeekBarChangeListener(mSeekListener);
        mPointer.setEnabled(true);

        Log.d(TAG, "onCreate mSensibilityValue = " + mSensibilityValue);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mSensibilityValue = ContentProviderHelper.getSensibilityValue(this);
        mPointer.setProgress(mSensibilityValue);
        Log.d(TAG, "onResume mSensibilityValue = " + mSensibilityValue);
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

}
