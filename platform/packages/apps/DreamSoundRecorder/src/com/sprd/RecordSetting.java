package com.sprd.soundrecorder;

import java.io.File;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.ListActivity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.app.ActionBar;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ToggleButton;
import android.widget.Switch;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.TimePicker;
import android.widget.Toast;
import android.widget.Button;
import android.widget.CompoundButton;
import android.content.Context;
import android.view.LayoutInflater;
import android.content.res.Resources;

import com.android.soundrecorder.R;
import com.android.soundrecorder.RecordService;
import com.android.soundrecorder.Recorder;
import com.android.soundrecorder.SoundRecorder;
import com.sprd.soundrecorder.Utils;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.content.Intent;
import android.widget.Toast;
import android.app.AlarmManager;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.TimePickerDialog;
import android.app.PendingIntent;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;

import android.view.View;

public class RecordSetting extends ListActivity implements
        TimeAndDurationPickerDialog.OnTimeSetListener,
        CompoundButton.OnCheckedChangeListener {

    private static final String TAG = "RecordSetting";
    private static SettingAdapter mAdapter;
    private Context mContext;
    private String mType = SoundRecorder.AUDIO_AMR;
    private int mIndex = 0;
    private String mHour;
    private String mMinute;
    private String mDuration;
    private static boolean mIsChecked;
    private TextView mItemDetail;
    /** SPRD:Bug611127 Timing recording display error in recording machine setup interface@{ */
    private TextView mClickView;
    /** @} */
    private Switch mSwitchButton;
    private Toast mToast;
    private static String mStartTime;
    private final static int PATHSELECT_RESULT_CODE = 1;
    public final static String SAVE_RECORD_TYPE = "recordType";
    public final static String SAVE_RECORD_TYPE_ID = "recordTypeId";
    public final static String SAVE_STORAGE_PATH = "storagePath";
    public final static String SAVE_STORAGE_PATH_ID = "storagePathId";
    public final static String SOUNDREOCRD_TYPE_AND_DTA = "soundrecord.type.and.data";
    public final static String FRAG_TAG_TIME_PICKER = "time_dialog";
    public final static String TIMER_RECORD_HOUR = "recordHour";
    public final static String TIMER_RECORD_MINUTE = "recordMinute";
    public final static String TIMER_RECORD_DURATION = "recordDuration";
    public final static String TIMER_RECORD_STATUS = "recordStatus";
    public final static String TIMER_RECORD_START_ACTION = "com.android.soundrecorder.timerrecord.START";
    public final static String TIMER_RECORD_TIME = "recordTime";

    public static final int INTERNAL_STORAGE_ITEM = 0;
    public static final int EXTERNAL_STORAGE_ITEM = 1;
    private int mCheckedItem = INTERNAL_STORAGE_ITEM;
    private String mStoragePath = "";
    private String mRecordInternalStorage;
    private String mRecordExternalStorage;
    // List for storage path name
    private List mStoragePathList = new ArrayList();
    // Map for storage path and choice item
    private HashMap<String, Integer> mStoragePathMap = new HashMap<String, Integer>();
    private AlertDialog mPathDialog;
    private BroadcastReceiver mMountEventReceiver = null;;


    public static OnTimerStateChangedListener mOnTimerStateChangedListener = null;

    public interface OnTimerStateChangedListener {
        public void onTimerStateChanged(boolean state, String time);
    }

    public static void setOnTimerStateChangedListener(OnTimerStateChangedListener listener) {
        mOnTimerStateChangedListener = listener;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().requestFeature(Window.FEATURE_ACTION_BAR);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        getWindow().getDecorView().setBackgroundColor(Color.parseColor("#ffffff"));

        mRecordInternalStorage = String.valueOf(getResources().getString(R.string.internal_storage));
        mRecordExternalStorage = String.valueOf(getResources().getString(R.string.external_storage));
        initResource();
        registerExternalStorageListener();
        // SPRD: bug594049 SoundRecorder happens JavaCrash(NullPointerException).
        startService(new Intent(this, RecordService.class));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            onBackPressed();
            break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void initResource() {
        getCurrentTypeId();
        getStoragePathAndId();
        getTimerRecordSetting();

        getStoragePathList();
        mAdapter = new SettingAdapter(this);
        setListAdapter(mAdapter);
        String storePath = getStorePath(this);
        if (StorageInfos.haveEnoughStorage(storePath) && !StorageInfos.isPathExistAndCanWrite(storePath)){
            mStoragePath = changeStorePath(this);
            Toast.makeText(this, R.string.use_default_path, Toast.LENGTH_LONG).show();
        }
    }
    //bug 613816 Set the recording interface to switch the font size the timer recording can not open
    @Override
    public void onStop(){
        Log.d(TAG,"onstop");
        FragmentManager manager = this.getFragmentManager();
        final FragmentTransaction ft = manager.beginTransaction();
        final Fragment prev = manager.findFragmentByTag(FRAG_TAG_TIME_PICKER);
        if (prev != null) {
            ft.remove(prev);
            ft.commitAllowingStateLoss();
        }
        super.onStop();
    }
    //bug 613816 end
    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy");
        if (mMountEventReceiver != null) {
            unregisterReceiver(mMountEventReceiver);
            mMountEventReceiver = null;
        }
        super.onDestroy();
    }

    private class SettingAdapter extends BaseAdapter {
        SettingAdapter(Context context) {
            super();
            mContext = context;
        }

        @Override
        public int getCount() {
            return 3;
        }

        @Override
        public Object getItem(int position) {
            return position;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder vHolder = null;
            if (convertView == null) {
                LayoutInflater flater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                convertView = flater.inflate(R.layout.setting_list_item, null);
                if (convertView == null)
                    throw new RuntimeException(
                            "inflater \"settings_list_item.xml\" failed; pos == "
                                    + position);

                vHolder = new ViewHolder();
                vHolder.item = (TextView) convertView
                        .findViewById(R.id.setting_item);
                vHolder.itemDetail = (TextView) convertView
                        .findViewById(R.id.setting_item_detail);
                vHolder.switchButton = (Switch) convertView
                        .findViewById(R.id.onoff);
                convertView.setTag(vHolder);
            } else {
                vHolder = (ViewHolder) convertView.getTag();
            }
            if (position == 0) {
                vHolder.item.setText(getResources().getString(
                        R.string.file_type));
                if (mType.equals(SoundRecorder.AUDIO_AMR)) {
                    vHolder.itemDetail.setText(getResources().getString(
                            R.string.type_amr));
                } else {
                    vHolder.itemDetail.setText(getResources().getString(
                            R.string.type_3gpp));
                }
            } else if (position == 1) {
                vHolder.item.setText(getResources().getString(
                        R.string.save_path));
                vHolder.itemDetail.setText((String)mStoragePathList.get(mCheckedItem));
            } else if (position == 2) {
                vHolder.item.setText(getResources().getString(
                        R.string.set_timer_recording));
                if (mHour == null || mMinute == null || mDuration == null) {
                    vHolder.itemDetail.setVisibility(View.GONE);
                } else {
                    int duration = (int) (Float.parseFloat(mDuration) * 60);
                    String[] durationHours = getResources().getStringArray(R.array.duration_hours);
                    int index = duration > 60 ? 1 : 0;
                    mStartTime = String.format(
                            getResources().getString(
                                    R.string.timer_recording_detail), mHour,
                            mMinute, mDuration + durationHours[index]);
                    vHolder.itemDetail.setText(mStartTime);
                }
                if (mIsChecked) {
                    vHolder.itemDetail
                            .setTextColor(Color.parseColor("#f24a09"));
                }
                vHolder.switchButton.setVisibility(View.VISIBLE);
                vHolder.switchButton.setChecked(mIsChecked);
            }
            vHolder.switchButton.setOnCheckedChangeListener(RecordSetting.this);
            return convertView;
        }
    }

    static class ViewHolder {
        public TextView item;
        public TextView itemDetail;
        public Switch switchButton;
    }

    public void saveRecordTypeAndSetting() {
        SharedPreferences recordSavePreferences = this.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = recordSavePreferences.edit();
        edit.putString(RecordSetting.SAVE_RECORD_TYPE, mType);
        edit.putInt(RecordSetting.SAVE_RECORD_TYPE_ID, mIndex);
        edit.commit();
        Log.i(TAG, "mType is saved:" + mType);
        Log.i(TAG, "mTypeId is saveds:" + mIndex);
    }

    private void getCurrentTypeId() {
        SharedPreferences recordSavePreferences = this.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        mType = recordSavePreferences.getString(RecordSetting.SAVE_RECORD_TYPE,
                mType);
        mIndex = recordSavePreferences.getInt(RecordSetting.SAVE_RECORD_TYPE_ID,
                mIndex);
    }

    public static String getStorePath(Context context) {
        String path = "";
        SharedPreferences fileSavePathShare = context.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        path = fileSavePathShare.getString(SAVE_STORAGE_PATH, "");
        if("".equals(path)) {
            File pathDir = null;
            if(StorageInfos.isExternalStorageMounted()) {
                pathDir = StorageInfos.getExternalStorageDirectory();
            } else {
                pathDir = StorageInfos.getInternalStorageDirectory();
            }
            if(pathDir != null) {
                path = createStorePath(pathDir.getPath());
            }
        } else {
            path = createStorePath(path);
        }
        Log.d(TAG, "getStorePath path = "+path);
        return path;
    }

    public static String createStorePath(String path){
        File pathFile = new File(path + Recorder.DEFAULT_STORE_SUBDIR);
        if (!pathFile.exists() && SoundRecorder.mCanCreateDir) {
            pathFile.mkdirs();
        }
        return pathFile.toString();
    }

    public static String changeStorePath(Context context){
        int item = RecordSetting.INTERNAL_STORAGE_ITEM;
        String path = "";
        if (StorageInfos.isExternalStorageMounted()) {
            item = RecordSetting.EXTERNAL_STORAGE_ITEM;
            path = StorageInfos.getExternalStorageDirectory().getAbsolutePath().toString();
        } else {
            item = RecordSetting.INTERNAL_STORAGE_ITEM;
            path = StorageInfos.getInternalStorageDirectory().getAbsolutePath().toString();
        }
        SharedPreferences fileSavePathShare = context.getSharedPreferences(RecordSetting.SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = fileSavePathShare.edit();
        edit.putString(RecordSetting.SAVE_STORAGE_PATH, path);
        edit.putInt(RecordSetting.SAVE_STORAGE_PATH_ID, item);
        edit.commit();

        Log.d(TAG, "changeStorePath path = "+path + ", item = "+item);
        notifyServicePathChanged(context, path);
        return path;
    }

    private static void notifyServicePathChanged(Context context, String path) {
        Intent intent = new Intent();
        intent.setAction(RecordService.PATHSELECT_BROADCAST);
        intent.putExtra("newPath", path + Recorder.DEFAULT_STORE_SUBDIR);
        context.sendBroadcast(intent);
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        Log.d(TAG, "onListItemClick position = "+position);
        mItemDetail = (TextView) v.findViewById(R.id.setting_item_detail);
        /** SPRD:Bug611127 Timing recording display error in recording machine setup interface@{ */
        mClickView = mItemDetail;
        /** @} */
        mSwitchButton = (Switch) v.findViewById(R.id.onoff);
        if (position == 0) {
            AlertDialog dialog = new AlertDialog.Builder(RecordSetting.this)
                    .setTitle(R.string.select_file_type)
                    .setSingleChoiceItems(
                            new String[] {
                                    String.valueOf(getResources().getString(
                                            R.string.record_amr)),
                                    String.valueOf(getResources().getString(
                                            R.string.record_3gpp)) }, mIndex,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    switch (which) {
                                    case 0:
                                        mType = SoundRecorder.AUDIO_AMR;
                                        /** SPRD:Bug611127 Timing recording display error in recording machine setup interface@{ */
                                        mClickView.setText(getResources()
                                                .getString(R.string.type_amr));
                                        /** @} */
                                        mIndex = which;
                                        break;
                                    case 1:
                                        mType = SoundRecorder.AUDIO_3GPP;
                                        /** SPRD:Bug611127 Timing recording display error in recording machine setup interface@{ */
                                        mClickView.setText(getResources()
                                                .getString(R.string.type_3gpp));
                                        /** @} */
                                        mIndex = which;
                                        break;
                                    default:
                                        mType = SoundRecorder.AUDIO_AMR;
                                    }
                                    saveRecordTypeAndSetting();
                                    dialog.dismiss();
                                }
                            })
                    .setNegativeButton(R.string.button_cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                }
                            }).show();
        } else if (position == 1) {
            if (!StorageInfos.isExternalStorageMounted()
                    && !StorageInfos.isInternalStorageMounted()) {
                Toast.makeText(mContext, R.string.stroage_not_mounted,
                        Toast.LENGTH_LONG).show();
            } else {
                /*Intent intent = new Intent(RecordSetting.this, PathSelect.class);
                startActivityForResult(intent, PATHSELECT_RESULT_CODE);*/
                getStoragePathList();
                mPathDialog = new AlertDialog.Builder(RecordSetting.this)
                .setTitle(R.string.path_label)
                .setSingleChoiceItems((String[]) (mStoragePathList.toArray(new String[mStoragePathList.size()])),mCheckedItem,
                        new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mCheckedItem = which;
                        mStoragePath = getCheckedStoragePath(which);
                        saveStoragePathAndId();
                        dialog.dismiss();
                        /** SPRD:Bug611127 Timing recording display error in recording machine setup interface@{ */
                        mClickView.setText((String)mStoragePathList.get(which));
                        /** @} */
                        notifyServicePathChanged(getApplicationContext(), mStoragePath);
                    }
                })
                .setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                }).show();
            }
        } else if (position == 2) {
            FragmentManager manager = this.getFragmentManager();
            showTimeEditDialog(manager);
        }
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        View view = (View) buttonView.getParent();
        mItemDetail = (TextView) view.findViewById(R.id.setting_item_detail);
        mSwitchButton = (Switch) view.findViewById(R.id.onoff);
        if (mItemDetail.getVisibility() != View.VISIBLE) {
            FragmentManager manager = this.getFragmentManager();
            showTimeEditDialog(manager);
            mSwitchButton.setChecked(false);
            return;
        }
        if (isChecked) {
            mIsChecked = true;
            mItemDetail.setTextColor(Color.parseColor("#f24a09"));
            int hour = Integer.parseInt(mHour);
            int minute = Integer.parseInt(mMinute);
            int duration = (int) (Float.parseFloat(mDuration) * 60);
            Log.d(TAG, "onCheckedChanged setTimerRecord hour=" + hour + ", minute=" + minute
                    + ", duration=" + duration);
            setTimerRecord(hour, minute, duration);
        } else {
            mIsChecked = false;
            mItemDetail.setTextColor(Color.parseColor("#afafaf"));
            cancelTimerRecord(this);
        }
        saveTimerRecordSetting();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (PATHSELECT_RESULT_CODE == requestCode) {
            Bundle bundle = null;
            if (data != null && (bundle = data.getExtras()) != null) {
                String selectPath = bundle.getString("path");
                mItemDetail.setText(selectPath);
            }
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void showTimeEditDialog(FragmentManager manager) {
        // final FragmentManager manager = fragment.getFragmentManager();
        final FragmentTransaction ft = manager.beginTransaction();
        final Fragment prev = manager.findFragmentByTag(FRAG_TAG_TIME_PICKER);
        if (prev != null) {
            ft.remove(prev);
        }
        ft.commitAllowingStateLoss();
        final TimePickerFragment timePickerFragment = new TimePickerFragment();
        // timePickerFragment.setTargetFragment(fragment, 0);
        timePickerFragment
                .setOnTimeSetListener((TimeAndDurationPickerDialog.OnTimeSetListener) this);
        // timePickerFragment.setAlarm(alarm);
        if (timePickerFragment != null && !timePickerFragment.isAdded()) {
            timePickerFragment.showAllowingStateLoss(manager,
                    FRAG_TAG_TIME_PICKER);
        }
    }

    @Override
    public void onTimeSet(TimePicker timePicker, int hourOfDay, int minute,
            int duration) {
        Log.d(TAG, "onTimeSet: hourOfDay=" + hourOfDay + ", minute=" + minute
                + ", duration=" + duration);
        mHour = hourOfDay < 10 ? "0" + hourOfDay : "" + hourOfDay;
        mMinute = minute < 10 ? "0" + minute : "" + minute;
        if (duration/60 == 0) {
            mDuration = "" + (float)duration/60;
        } else {
            mDuration = "" + duration/60;
        }

        String[] durationHours = getResources().getStringArray(R.array.duration_hours);
        int index = duration > 60 ? 1 : 0;
        mStartTime = String.format(
                getResources().getString(R.string.timer_recording_detail),
                mHour, mMinute, mDuration + durationHours[index]);
        mItemDetail.setText(mStartTime);
        mItemDetail.setVisibility(View.VISIBLE);
        saveTimerRecordSetting();
        if (!mSwitchButton.isChecked()) {
            mSwitchButton.setChecked(true);
        }
        setTimerRecord(hourOfDay, minute, duration);
    }

    public void saveTimerRecordSetting() {
        SharedPreferences recordSavePreferences = this.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = recordSavePreferences.edit();
        edit.putString(RecordSetting.TIMER_RECORD_HOUR, mHour);
        edit.putString(RecordSetting.TIMER_RECORD_MINUTE, mMinute);
        edit.putString(RecordSetting.TIMER_RECORD_DURATION, mDuration);
        edit.putBoolean(RecordSetting.TIMER_RECORD_STATUS, mIsChecked);
        edit.commit();
    }

    private void getTimerRecordSetting() {
        SharedPreferences recordSavePreferences = this.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        mHour = recordSavePreferences.getString(
                RecordSetting.TIMER_RECORD_HOUR, mHour);
        mMinute = recordSavePreferences.getString(
                RecordSetting.TIMER_RECORD_MINUTE, mMinute);
        mDuration = recordSavePreferences.getString(
                RecordSetting.TIMER_RECORD_DURATION, mDuration);
        mIsChecked = recordSavePreferences.getBoolean(
                RecordSetting.TIMER_RECORD_STATUS, mIsChecked);
    }

    private void setTimerRecord(int hour, int minute, int duration) {
        Calendar c = Calendar.getInstance();
        int nowHour = c.get(Calendar.HOUR_OF_DAY);
        int nowMinute = c.get(Calendar.MINUTE);
        if (hour < nowHour || hour == nowHour && minute < nowMinute || hour == nowHour
                && minute == nowMinute) {
            Log.d(TAG, "setTimerRecord add 1 day");
            c.add(Calendar.DAY_OF_YEAR, 1);
        }
        c.set(Calendar.HOUR_OF_DAY, hour);
        c.set(Calendar.MINUTE, minute);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);

        long timeInMillis = c.getTimeInMillis();
        Log.d(TAG, "setTimerRecord, timeInMillis = "+timeInMillis+", duration = "+duration);

        Intent intent = new Intent(TIMER_RECORD_START_ACTION);
        intent.putExtra(TIMER_RECORD_DURATION, duration);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(
                this, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);
        AlarmManager am = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        am.setExact(AlarmManager.RTC_WAKEUP, timeInMillis, pendingIntent);

        SharedPreferences timerRecordPreferences = this.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = timerRecordPreferences.edit();
        edit.putLong(RecordSetting.TIMER_RECORD_TIME, timeInMillis);
        edit.commit();

        long timerDelta = timeInMillis - System.currentTimeMillis();
        String text = Utils.formatElapsedTimeUntilAlarm(this, timerDelta);
        Toast.makeText(this, text, Toast.LENGTH_LONG).show();

        mOnTimerStateChangedListener.onTimerStateChanged(true, mStartTime);
    }

    public static void cancelTimerRecord(Context context) {
        Log.d(TAG, "cancelTimerRecord");
        Intent intent = new Intent(TIMER_RECORD_START_ACTION);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(
                context, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        am.cancel(pendingIntent);
        pendingIntent.cancel();

        SharedPreferences recordSavePreferences = context.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = recordSavePreferences.edit();
        edit.putBoolean(RecordSetting.TIMER_RECORD_STATUS, false);
        edit.commit();

        if (mAdapter != null && mIsChecked) {
            mIsChecked = false;
            mAdapter.notifyDataSetChanged();
        }

        mOnTimerStateChangedListener.onTimerStateChanged(false, mStartTime);
    }

    /* SPRD: bug595666 Fail to start timer record after reboot phone. @{ */
    public static void resetTimerRecord(Context context, long time, int duration) {
        Intent intent = new Intent(TIMER_RECORD_START_ACTION);
        intent.putExtra(TIMER_RECORD_DURATION, duration);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(
                context, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        am.setExact(AlarmManager.RTC_WAKEUP, time, pendingIntent);

        SharedPreferences timerRecordPreferences = context.getSharedPreferences(
                SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = timerRecordPreferences.edit();
        edit.putLong(RecordSetting.TIMER_RECORD_TIME, time);
        edit.commit();
    }
    /* @} */

    public void saveStoragePathAndId(){
        SharedPreferences recordSavePreferences = this.getSharedPreferences(SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = recordSavePreferences.edit();
        edit.putString(SAVE_STORAGE_PATH, mStoragePath);
        edit.putInt(SAVE_STORAGE_PATH_ID, mCheckedItem);
        edit.commit();
        Log.i(TAG,"saveStoragePathAndId mStoragePath=" + mStoragePath+", mCheckedItem="+mCheckedItem);
    }

    private void getStoragePathAndId(){
        SharedPreferences recordSavePreferences = this.getSharedPreferences(SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        mStoragePath = recordSavePreferences.getString(SAVE_STORAGE_PATH, mStoragePath);
        mCheckedItem = recordSavePreferences.getInt(SAVE_STORAGE_PATH_ID, mCheckedItem);
    }

    private void getStoragePathList() {
        int choiceItem = INTERNAL_STORAGE_ITEM;
        String internalStoragePath = StorageInfos.getInternalStorageDirectory().getPath();
        String externalStoragePath = StorageInfos.getExternalStorageDirectory().getPath();
        mStoragePathList.clear();
        mStoragePathMap.clear();
        if (StorageInfos.isExternalStorageMounted()) {
            mStoragePathList.add(mRecordInternalStorage);
            mStoragePathMap.put(internalStoragePath, choiceItem++);

            mStoragePathList.add(mRecordExternalStorage);
            mStoragePathMap.put(externalStoragePath, choiceItem++);
            mCheckedItem = EXTERNAL_STORAGE_ITEM;
        } else {
            mStoragePathList.add(mRecordInternalStorage);
            mStoragePathMap.put(internalStoragePath, choiceItem++);
            mCheckedItem = INTERNAL_STORAGE_ITEM;
        }
        StorageManager storageManager = getSystemService(StorageManager.class);
        List<VolumeInfo> volumes = storageManager.getVolumes();
        Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
        for (VolumeInfo vol : volumes) {
            File file = vol.getPath();
            if (file != null && Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(file))) {
                if (vol.disk != null && vol.disk.isUsb()) {
                    mStoragePathList.add(storageManager.getBestVolumeDescription(vol));
                    mStoragePathMap.put(file.getPath(), choiceItem++);
                }
            }
        }
        if (!"".equals(mStoragePath)) {
            if (mStoragePathMap.containsKey(mStoragePath)) {
                mCheckedItem = mStoragePathMap.get(mStoragePath);
            } /*else {
                if (StorageInfos.isExternalStorageMounted()) {
                    mCheckedItem = EXTERNAL_STORAGE_ITEM;
                    mStoragePath = externalStoragePath;
                } else {
                    mCheckedItem = INTERNAL_STORAGE_ITEM;
                    mStoragePath = internalStoragePath;
                }
            }
            saveStoragePathAndId();
            notifyServicePathChanged(this, mStoragePath);*/
        }
    }

    private String getCheckedStoragePath(int checkedItem) {
        String checkedStoragePath = "";
        Iterator it = mStoragePathMap.entrySet().iterator();
        while(it.hasNext()) {
            Map.Entry<String, Integer> entry = (Map.Entry) it.next();
            if (checkedItem == entry.getValue()) {
                checkedStoragePath = entry.getKey();
            }
        }
        return checkedStoragePath;
    }

    private void registerExternalStorageListener() {
        if (mMountEventReceiver == null) {
            mMountEventReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Log.d(TAG, "onReceive intent = "+intent);
                    if(intent == null){
                        return;
                    }
                    String action = intent.getAction();
                    String path = intent.getData().getPath();
                    if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                        if (mPathDialog != null && mPathDialog.isShowing()){
                            mPathDialog.dismiss();
                        }
                        Log.d(TAG, "onReceive path = "+path+ ", mStoragePath="+mStoragePath);
                        if (mStoragePath.equals(path) || "".equals(mStoragePath) && path.equals(StorageInfos.getExternalStorageDirectory().getPath())) {
                            mStoragePath = changeStorePath(context);
                            getStoragePathList();
                            if (mAdapter != null) {
                                mAdapter.notifyDataSetChanged();
                            }
                            Toast.makeText(context, R.string.use_default_path, Toast.LENGTH_LONG).show();
                        }
                    } else if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                        if ("".equals(mStoragePath) && path.equals(StorageInfos.getExternalStorageDirectory().getPath())){
                            getStoragePathList();
                            // SPRD: bug603749 Record save in internal storage after insert sdcard.
                            notifyServicePathChanged(context, StorageInfos.getExternalStorageDirectory().getPath());
                            if (mAdapter != null) {
                                mAdapter.notifyDataSetChanged();
                            }
                        }
                    }
                }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
            iFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
            iFilter.addDataScheme("file");
            registerReceiver(mMountEventReceiver, iFilter);
        }
    }
}
