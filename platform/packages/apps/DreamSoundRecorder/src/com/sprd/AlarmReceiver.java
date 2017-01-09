package com.sprd.soundrecorder;

import com.android.soundrecorder.SoundRecorder;
import com.android.soundrecorder.RecordService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.Log;

import com.sprd.soundrecorder.RecordSetting;

public class AlarmReceiver extends BroadcastReceiver {

    private final static String TAG = "AlarmReceiver";
    public final static String PREPARE_RECORD = "StartRecord";
    public final static String MISS_RECORD = "MissRecord";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d("AlarmReceiver", " onReceive intent = " + intent);
        if (RecordSetting.TIMER_RECORD_START_ACTION.equals(intent.getAction())) {
            Intent startRecord = new Intent(context, RecordService.class);
            startRecord.putExtra(PREPARE_RECORD, true);
            startRecord.putExtra(RecordSetting.TIMER_RECORD_DURATION, intent.getIntExtra(RecordSetting.TIMER_RECORD_DURATION, 0));
            context.startService(startRecord);
        } else if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            SharedPreferences timerRecordPreferences = context.getSharedPreferences(RecordSetting.SOUNDREOCRD_TYPE_AND_DTA, Context.MODE_PRIVATE);
            boolean isChecked = timerRecordPreferences.getBoolean(RecordSetting.TIMER_RECORD_STATUS, false);
            long recordTime = timerRecordPreferences.getLong(RecordSetting.TIMER_RECORD_TIME, 0);
            Log.d(TAG, "receiveBootCompleted, recordTime = "+recordTime+", isChecked = "+isChecked+", System.currentTimeMillis() = "+System.currentTimeMillis());
            /* SPRD: bug595666 Fail to start timer record after reboot phone. @{ */
            if (isChecked) {
                if (recordTime <= System.currentTimeMillis()) {
                    Intent missRecord = new Intent(context, RecordService.class);
                    missRecord.putExtra(MISS_RECORD, true);
                    missRecord.putExtra(RecordSetting.TIMER_RECORD_TIME, recordTime);
                    context.startService(missRecord);
                } else {
                    String duration = timerRecordPreferences.getString(RecordSetting.TIMER_RECORD_DURATION, null);
                    int recordDuration = (int) (Float.parseFloat(duration) * 60);
                    RecordSetting.resetTimerRecord(context, recordTime, recordDuration);
                }
            }
            /* @} */
        }
    }
}