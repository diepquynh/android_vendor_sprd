package com.android.server;

import android.util.Slog;
import android.text.format.DateFormat;
import android.text.format.Time;
import android.app.PowerGuruAlarmInfo;
import android.app.AbsPowerGuru;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import java.util.List;

public class AlarmAlignHelper {

  private static final String TAG = "AlarmManagerHelper";
  private boolean mAlignEnable;
  private int mAlignLength;
  private AbsPowerGuru mPowerGuruService;
  private List<PowerGuruAlarmInfo> mBeatlist;

  public AlarmAlignHelper(Context context) {}

  protected void updateBeatlist(){}

  protected boolean getAlignEnable(){
    return false;
  }

  protected void setAlignEnable(boolean enable){
    mAlignEnable = false;
  }

  protected boolean isEnabled(){
    return false;
  }

  protected boolean checkAlignEnable(int type, final PowerGuruAlarmInfo palarm){
    return false;
  }

  protected boolean isUnavailableGMS(final PowerGuruAlarmInfo guruAlarm){
    Slog.w(TAG, "isUnavailableGMS return true immediately for no-powerguru feature!!!");
    return true;
  }

  private int getAlignLength(){
    return mAlignLength;
  }

  protected boolean setAlignLength(int length){
    return true;
  }

  protected long adjustTriggerTime(long triggerTime, int type){
    return 0;
  }

  private boolean isAlignPoint(long rtcTime, int length){
    return false;
  }

  private long adjustTriggerTimeInternal(long rtcTime, int alignLength){
    return 0;
  }

  protected PowerGuruAlarmInfo matchBeatListPackage(final PendingIntent pi){
    return null;
  }

  protected boolean notifyPowerGuru(int type, long when, long whenElapsed, long windowLength,long maxWhen, long interval, PendingIntent operation){
    Slog.w(TAG, "notifyPowerGuru() -> PowerGuruService is not running results from no heartbeat feature ???");
    return false;
  }

}
