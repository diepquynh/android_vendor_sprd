package com.sprdroid.note;

import java.io.IOException;
import java.io.File;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.lang.Integer;

import com.sprdroid.note.AlarmSettingAdapter;
import com.sprdroid.note.CallAlarm;
import com.sprdroid.note.DBOpenHelper;
import com.sprdroid.note.DBOperations;
import com.sprdroid.note.R;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.DatePickerDialog;
import android.app.PendingIntent;
import android.app.TimePickerDialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.View;
import android.widget.AdapterView;
import android.widget.DatePicker;
import android.widget.ListView;
import android.widget.TimePicker;
import android.widget.Toast;
import android.util.Log;

public class AlarmActivity extends Activity {

    private ListView lv;
    private List<String> items;
    private List<String> values;
    private Calendar cal = Calendar.getInstance();
    private int id = 0;
    // private int clockId;
    // clockid == id; 使得闹钟ID与便签ID一致
    private int isOpen = 0;
    private String date = "";
    private String time = "";
    private int isRepeat = 0;
    private int isVibrate = 0;
    private String rings = "";
    private String rings_uri = "";
    private int[] yesOrNo;
    private DBOperations dbo;
    private AlarmManager am;

    Date mDate;
	SimpleDateFormat dateFormat;
	String strTimeFormat ="";
	String str = "";
	String strCtime = "";
    boolean isTime_24_12 ;
    
    private int checkedEnable =1;  //默認选择开启鬧鈴和震動为：“否”
    private int checkedVibrate= 1;
    // 铃声文件夹
    //private String strAlarmFolder = "/system/media/audio/";

    private static final String TAG = "AlarmActivity";

    private static String[] CURSOR_COLS = new String[] {
            MediaStore.Audio.Media._ID, MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.DISPLAY_NAME, MediaStore.Audio.Media.DATA,
            MediaStore.Audio.Media.ALBUM, MediaStore.Audio.Media.ARTIST,
            MediaStore.Audio.Media.ARTIST_ID, MediaStore.Audio.Media.DURATION,
            MediaStore.Audio.Media.TRACK };

    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setContentView(R.layout.alarm_layout);
        am = (AlarmManager) getSystemService(ALARM_SERVICE);

        //yesOrNo = new String[] { getResources().getString(R.string.yes),
        //       getResources().getString(R.string.no) };
        yesOrNo = new int[] {1,0};

        dbo = new DBOperations();
        lv = (ListView) findViewById(R.id.alarm_listview);
        // 接受到便签的ID
        id = this.getIntent().getIntExtra("noteid", 0);
        Log.v(TAG, "onCreate() :noteid= " + id
                + "*************************************");
        // Cursor cursorNote = dbo.queryOneNote(AlarmActivity.this, id);
        // clockId = cursorNote.getColumnIndex(DBOpenHelper.ALARM_ID);

		ListCusorSet();
        // dbo.close();
        // 将数据库中读出的书据，加到ListItem，然后显示出来。
        setListItem();
        Log.v(TAG, "setListItem() " + "*************************************");
        lv.setOnItemClickListener(new ListView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View view,
                    int position, long arg3) {
                switch (position) {
                case 0:
                    setAlarmState();
                    break;
                case 1:
                    setDate();
                    break;
                case 2:
                    setTime();
                    break;
                // case 3:
                // setRepeat();
                // break;
                case 3:
                    setVibrate();
                    break;
                case 4:
                    setRings();
                    break;
                }
            }

        });
    }

    public void setListItem() {
        items = new ArrayList<String>();
        values = new ArrayList<String>();
        items.add(getResources().getString(R.string.start_alarm));
		if(isOpen== 1) {
	        	values.add(getResources().getString(R.string.yes));
		}else{
	        	values.add(getResources().getString(R.string.no));
		}
        items.add(getResources().getString(R.string.alarm_date_setting));
        //根据系统日期格式，显示日期设置
		ContentResolver cv = this.getContentResolver();
		strTimeFormat = android.provider.Settings.System.getString(cv,
				android.provider.Settings.System.TIME_12_24);
		String strDateFormat = android.provider.Settings.System.getString(
				cv, android.provider.Settings.System.DATE_FORMAT);	
		if (!date.equals("")) {
			mDate = new Date();
			if (strDateFormat != null)
				dateFormat = new SimpleDateFormat(strDateFormat);
			Log.i("Date",
					Integer.parseInt(date.substring(0, 4)) + " "
							+ Integer.parseInt(date.substring(5, 7)) + " "
							+ Integer.parseInt(date.substring(8, 10)));
			mDate.setYear(Integer.parseInt(date.substring(0, 4)) - 1900);// 0代表1900年
			mDate.setMonth(Integer.parseInt(date.substring(5, 7)) - 1);// 从0开始
			mDate.setDate(Integer.parseInt(date.substring(8, 10)));

			if (dateFormat != null)
				str = dateFormat.format(mDate);
			if (!str.equals("")) 
			{
				values.add(str);
			} 
			else
			{
				values.add(date.substring(0));
			}
		} 
		else {      
	        values.add(date);
		}
		
		//根据系统时间设置格式，显示时间
		if (!time.equals("")) {
			strCtime = new String(time.toString());
			Log.v("you", " strTimeFormat =" + strTimeFormat);
			Log.v("you", " ctime =" + time);
			if (strTimeFormat != null && strTimeFormat.equals("12")) {
				// 获取小时数
				strCtime = strCtime.substring(0, 5);
				String strhour = strCtime.substring(0, 2);
				int hour = Integer.parseInt(strhour);
				// 判断是上午还是下午
				if (hour >= 12) {
					if (hour > 12) {
						hour = hour % 12;
						strCtime = hour + strCtime.substring(2, 5)
								+ getString(R.string.PM);
					} else {

						strCtime = strCtime.substring(0, 5)
								+ getString(R.string.PM);
					}

				} else {
					if (hour == 0)
						hour = 12;
					strCtime = hour + strCtime.substring(2, 5)
							+ getString(R.string.AM);
				}

				if (strCtime.substring(0, 2).equals("12")
						|| strCtime.substring(0, 2).equals("11")
						|| strCtime.substring(0, 2).equals("10")) {
					if (strCtime.contains("上午")) {
						strCtime = getString(R.string.AM)
								+ strCtime.substring(0, 5);
					} else if (strCtime.contains("下午")) {
						strCtime = getString(R.string.PM)
								+ strCtime.substring(0, 5);
					}
				} else {
					if (strCtime.contains("上午")) {
						strCtime = getString(R.string.AM)
								+ strCtime.substring(0, 4);
					} else if (strCtime.contains("下午")) {
						strCtime = getString(R.string.PM)
								+ strCtime.substring(0, 4);
					}
				}
			} else {
				strCtime = strCtime.substring(0, 5);
			}
		} 
        items.add(getResources().getString(R.string.alarm_time_setting));
        //values.add(time);
		values.add(strCtime);
		
        // items.add(getResources().getString(R.string.alarm_is_repeat));
        // values.add(isRepeat);
        items.add(getResources().getString(R.string.alarm_is_vibrate));
        //values.add(isVibrate);
		if(isVibrate== 1) {
	        	values.add(getResources().getString(R.string.yes));
		}else{
	        	values.add(getResources().getString(R.string.no));
		}

        items.add(getResources().getString(R.string.alarm_rings));
		if(rings.equals("")) {
	        	values.add(getResources().getString(R.string.default_rings_name));
		}else{
	        	values.add(rings);
		}
        lv.setAdapter(new AlarmSettingAdapter(this, items, values));
        Log.v("you", "values= " + values);
    }

    public void onPause() {

        saveChange();
        super.onPause();
    }

    private void setAlarmState() {

        OnClickListener listener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                case 0:
			openAlarm();
			dialog.dismiss();
                    break;
                case 1:
			closeAlarm();
			dialog.dismiss();
                    break;
                }
                saveChange();
            }

        };
	String[] choices = new String[] {getResources().getString(R.string.yes),
				getResources().getString(R.string.no)};
	if( isOpen== 1){
		checkedEnable=0; //position 为0是选中YES
	}else {
		checkedEnable=1;
	}

        AlertDialog.Builder builder = new AlertDialog.Builder(AlarmActivity.this)
                .setTitle(R.string.enableAlarm)
                //.setItems(yesOrNo, listener)
		.setSingleChoiceItems(choices, checkedEnable,listener)
                .setNegativeButton(R.string.Cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
				dialog.dismiss();
                            }

                        });
	builder.show();
    }

    private void openAlarm() {
        //wangsl
        //cal.setTimeInMillis(System.currentTimeMillis());
        //TODO
        if(date == null || time == null || date.length() == 0 || time.length() == 0) {
            Toast.makeText(AlarmActivity.this, R.string.set_time_frist,
                    Toast.LENGTH_LONG).show();
            return;
        }
        //wangsl

	//查询所有便簽鬧鈴date和time，如果存在和当前設置日期时间一样，那么不允许开启该鬧鈴，并提示用户。
	String date_setted ="";
	String time_setted ="";
	Cursor cursor_ClocksEnable = dbo.queryAllClocksEnable(AlarmActivity.this);
	Log.v("you", "cursor_ClocksEnable= " + cursor_ClocksEnable);
	int cnt =cursor_ClocksEnable.getCount();
	cursor_ClocksEnable.moveToFirst();
	//逐一得到每个已开启鬧鈴的id，得到日期和时间
        if(cnt > 0 && cursor_ClocksEnable != null){ //for bugzilla 10501
		do{
			int alarm_id = cursor_ClocksEnable.getInt(cursor_ClocksEnable
					    .getColumnIndex(DBOpenHelper.ID));
			Log.v("you", "********alarm_id= " + alarm_id);
			Log.v("you", "********id= " + id);
		    if(alarm_id != id) {  //避免当前設置的鬧鈴时间和自己比较
			Cursor cursor_Clock = dbo.getClock(AlarmActivity.this, alarm_id);
			cursor_Clock.moveToFirst();
			date_setted =cursor_Clock.getString(cursor_Clock
					    .getColumnIndex(DBOpenHelper.CLOCK_DATE));
			time_setted =cursor_Clock.getString(cursor_Clock
					    .getColumnIndex(DBOpenHelper.CLOCK_TIME));
			if(cursor_Clock !=null)	
				cursor_Clock.close();
			
			if(date_setted.equals(date) && time_setted.equals(time) ){
				Toast.makeText(AlarmActivity.this, R.string.date_time_setted,
					    Toast.LENGTH_LONG).show();
				return;
			}
		    }
		}while(cursor_ClocksEnable.moveToNext() );
	}
		if(cursor_ClocksEnable !=null)
			cursor_ClocksEnable.close();
        // 将以字符串形式的时间设到闹钟服务中
        String[] temp = time.split(":");
        Calendar dateCalendar = praseDate();
        
        //cal.set(Calendar.HOUR_OF_DAY, Integer.parseInt(temp[0]));
        //cal.set(Calendar.MINUTE, Integer.parseInt(temp[1]));
        //cal.set(Calendar.SECOND, 0);
        //cal.set(Calendar.MILLISECOND, 0);
        dateCalendar.set(Calendar.HOUR_OF_DAY, Integer.parseInt(temp[0]));
        dateCalendar.set(Calendar.MINUTE, Integer.parseInt(temp[1]));
        dateCalendar.set(Calendar.SECOND, 0);
        dateCalendar.set(Calendar.MILLISECOND, 0);

        Intent intent = new Intent(AlarmActivity.this, CallAlarm.class);
        // 用intent发送CLOCK_ID
        intent.putExtra(DBOpenHelper.CLOCK_ID, id);
        PendingIntent pi = PendingIntent.getBroadcast(AlarmActivity.this,
                    id, intent, 0);
        //am.set(AlarmManager.RTC_WAKEUP, cal.getTimeInMillis(), pi);
        am.set(AlarmManager.RTC_WAKEUP, dateCalendar.getTimeInMillis(), pi);
        // am.setRepeating(AlarmManager.RTC_WAKEUP, cal.getTimeInMillis(), 10*1000, pi);

        isOpen = yesOrNo[0];
        setListItem();
        // 将设置写入到items表中ALARMENABLE项
        Log.v("you", "openalarm: id =" + id);
        dbo.update_alarm_enable(this, id, isOpen);

    }

    private void closeAlarm() {
        Intent intent = new Intent(AlarmActivity.this, CallAlarm.class);
        intent.putExtra(DBOpenHelper.CLOCK_ID, id);
        PendingIntent pi = PendingIntent.getBroadcast(AlarmActivity.this, id,
                intent, 0);
        am.cancel(pi);
        isOpen = yesOrNo[1];
        setListItem();

        dbo.update_alarm_enable(this, id, isOpen);
    }

    private void setDate() {
        //wangsl
        int year = 2012;
        int month = 01;
        int day = 01;  
        if(date.equals("")) {
            cal.setTimeInMillis(System.currentTimeMillis());
            year = cal.get(Calendar.YEAR);
            month = cal.get(Calendar.MONTH);
            day = cal.get(Calendar.DAY_OF_MONTH);
        } else {
            Calendar calendar = praseDate();
            if(calendar != null) {
                year = calendar.get(Calendar.YEAR);
                month = calendar.get(Calendar.MONTH);
                day = calendar.get(Calendar.DAY_OF_MONTH);
            }
        }

        new DatePickerDialog(AlarmActivity.this,
                new DatePickerDialog.OnDateSetListener() {
                    @Override
                    public void onDateSet(DatePicker view, int year,
                            int monthOfYear, int dayOfMonth) {                      
                        //SimpleDateFormat dateFormat = new SimpleDateFormat("hh:mm");
                        //try {
                        //    Date settingDate = dateFormat.parse(time);
                        //    Calendar settingCal = Calendar.getInstance();
                        //    settingCal.setTime(settingDate);
                        //    settingCal.set(Calendar.YEAR, year);
                        //    settingCal.set(Calendar.MONTH, monthOfYear);
                        //    settingCal.set(Calendar.DAY_OF_MONTH, dayOfMonth);
                            
                        //    if(settingCal.getTimeInMillis() < System.currentTimeMillis()) {
                        //        Log.e("wangsl","setDate out of range");
                        //        return;
                        //    }
                        Calendar calendar = praseTime();
                        if(calendar != null) {
                            calendar.set(Calendar.YEAR, year);
                            calendar.set(Calendar.MONTH, monthOfYear);
                            calendar.set(Calendar.DAY_OF_MONTH, dayOfMonth);     
                            if(calendar.getTimeInMillis() < System.currentTimeMillis()) {
                                Toast.makeText(getApplicationContext(), R.string.set_date_fail, 
                                        Toast.LENGTH_LONG).show();
                                return;
                            }                                    
                        }else{  // 如果没有设置时间，日期只能设置为当天或者以后
				cal.setTimeInMillis(System.currentTimeMillis());
				int cur_year = cal.get(Calendar.YEAR);
				int cur_month = cal.get(Calendar.MONTH);
				int cur_day = cal.get(Calendar.DAY_OF_MONTH);

				Calendar sys_current =Calendar.getInstance();
				sys_current.set(Calendar.YEAR, cur_year);
				sys_current.set(Calendar.MONTH, cur_month);
				sys_current.set(Calendar.DAY_OF_MONTH, cur_day);
				sys_current.set(Calendar.HOUR_OF_DAY, 0);
				sys_current.set(Calendar.MINUTE, 0);

				Calendar set_date =Calendar.getInstance();
				set_date.set(Calendar.YEAR, year);
				set_date.set(Calendar.MONTH, monthOfYear);
				set_date.set(Calendar.DAY_OF_MONTH, dayOfMonth);
				set_date.set(Calendar.HOUR_OF_DAY, 0);
				set_date.set(Calendar.MINUTE, 0);
                                if(set_date.getTimeInMillis() < sys_current.getTimeInMillis()) {
                                	Toast.makeText(getApplicationContext(), R.string.set_date_fail, 
                                        Toast.LENGTH_LONG).show();
                                return;
                            } 

			}                           
                        //}catch (ParseException e) {Log.e(TAG,"setTime fail!!!");}                                        
                        date = year + "-" + format(monthOfYear + 1) + "-"
                                + format(dayOfMonth);
                        saveChange();
                        if (isOpen==1) {
                            openAlarm();
                        }
                        setListItem();
                    }

                }, year, month, day).show();
        //wangsl
    }

    private void setTime() {
        //wangsl
        int hour = 0;
        int minute = 0;
            cal.setTimeInMillis(System.currentTimeMillis());
            hour = cal.get(Calendar.HOUR_OF_DAY);
                Log.v("you", "setTime():hour =" + hour);
            minute = cal.get(Calendar.MINUTE);
        
        if(time == null) {
            cal.setTimeInMillis(System.currentTimeMillis());
            hour = cal.get(Calendar.HOUR_OF_DAY);
            minute = cal.get(Calendar.MINUTE);
        }else {
            //SimpleDateFormat dateFormat = new SimpleDateFormat("hh:mm");
            //try {
            //    Date settingDate = dateFormat.parse(time);
            //    Calendar settingCal = Calendar.getInstance();
            //    settingCal.setTime(settingDate);
            //    hour = settingCal.get(Calendar.HOUR_OF_DAY);
            //    minute = settingCal.get(Calendar.MINUTE);
            //}catch (ParseException e) {Log.e(TAG,"setTime fail!!!");}
            Calendar calendar = praseTime();
            if(calendar != null) {
                hour = calendar.get(Calendar.HOUR_OF_DAY);
                Log.v("you", "hour =" + hour);
                minute = calendar.get(Calendar.MINUTE);         
            }
        }
        
        ContentResolver cv = this.getContentResolver();
    	String strTimeFormat = android.provider.Settings.System.getString(cv, 
				android.provider.Settings.System.TIME_12_24);
        if (strTimeFormat != null) {
        	isTime_24_12= strTimeFormat.equals("24");
        }
        
        new TimePickerDialog(AlarmActivity.this,
                new TimePickerDialog.OnTimeSetListener() {
                    @Override
                    public void onTimeSet(TimePicker view, int hourOfDay,
                            int minute) {
                        
                        //SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
                        //try {
                        //    Date settingDate = dateFormat.parse(date);
                        //    Calendar settingCal = Calendar.getInstance();
                        //    settingCal.setTime(settingDate);
                        //    settingCal.set(Calendar.HOUR_OF_DAY, hourOfDay);
                        //    settingCal.set(Calendar.MINUTE, minute);
                        //    Log.d("wangsl","settingDate is " + settingDate);
                        //    if(settingCal.getTimeInMillis() < System.currentTimeMillis()) {
                        //        //TODO
                        //        Log.e("wangsl","setTime out of range");
                        //        return;
                        //    }
                        //    
                        //}catch (ParseException e) {Log.e(TAG,"setTime fail!!!");}
                        Calendar calendar = praseDate();
                        if(calendar != null) {
                            calendar.set(Calendar.HOUR_OF_DAY, hourOfDay);
                            calendar.set(Calendar.MINUTE, minute);
                            if(calendar.getTimeInMillis() < System.currentTimeMillis()) {
                                Toast.makeText(getApplicationContext(), R.string.set_time_fail, Toast.LENGTH_LONG).show();
                                return;
                            }
                        }
                        
                        Log.d("wangsl","hourOfDay is" + hourOfDay);
                        time = format(hourOfDay) + ":" + format(minute);
                        saveChange();
                        if (isOpen==1) {
                            openAlarm();
                        }
                        setListItem();
                    }

                }, hour, minute,  isTime_24_12).show();
        //wangsl
    }

    public void setRepeat() {
        OnClickListener listener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                isRepeat = yesOrNo[which];
                if (isOpen==1) {
                    openAlarm();
                }
                saveChange();
                setListItem();
            }

        };
	String[] choices = new String[] {getResources().getString(R.string.yes),
				getResources().getString(R.string.no)};
        new AlertDialog.Builder(AlarmActivity.this)
                .setItems(choices, listener)
                .setNegativeButton(R.string.Cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                            }
                        }).show();
    }

    public void setVibrate() {
        OnClickListener listener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                //isVibrate = yesOrNo[which];
                //saveChange();
                //setListItem();
                switch (which) {
		        case 0:
			    isVibrate = yesOrNo[0];
			    dialog.dismiss();
		            break;
		        case 1:
			    isVibrate = yesOrNo[1];
			    dialog.dismiss();
		            break;
		}
		saveChange();
		setListItem();
            }

        };

	String[] choices = new String[] {getResources().getString(R.string.yes),
				getResources().getString(R.string.no)};

	if( isVibrate==1){
		checkedVibrate=0;
	}else {
		checkedVibrate=1;
	}

        new AlertDialog.Builder(AlarmActivity.this)
                .setTitle(R.string.setVibrate)
                //.setItems(yesOrNo, listener)
		.setSingleChoiceItems(choices, checkedVibrate,listener)
                .setNegativeButton(R.string.Cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
				dialog.dismiss();
                            }

                        }).show();

    }

    public void setRings() {
        // Intent intent = new Intent();
        // intent.setAction(Intent.ACTION_GET_CONTENT);
        // intent.setType("audio/mp3"); 
        //原本是想任意设置mp3为铃声，但是由于sdCard限制
        // intent.setData(Uri.fromFile(new File(strAlarmFolder)));
        // intent.setDataAndType(Uri.parse("file:///system/media/audio/*"), "audio/ogg");
        // startActivity(Intent.createChooser(intent, "Select Ringtone"));
        // this.startActivityForResult(Intent.createChooser(intent,
        // "Select Ringtone"), 1);

        Intent intent = new Intent(RingtoneManager.ACTION_RINGTONE_PICKER);
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_TYPE,
                RingtoneManager.TYPE_ALARM);
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_TITLE,
                R.string.set_alarm_ringtone);

        // Allow user to pick 'Default'
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_DEFAULT, true);
        // Don't show 'Silent'
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_SILENT, false);

        Uri ringtoneUri;
        
        if (rings_uri != null && rings_uri.length() != 0) {
            ringtoneUri = Uri.parse(rings_uri);
        } else {
            // Otherwise pick default ringtone Uri so that something is selected.
            ringtoneUri = RingtoneManager
                    .getDefaultUri(RingtoneManager.TYPE_RINGTONE);
        

        }
        // 将上次选择的铃声，设为默认（再次进入时被选则）
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_EXISTING_URI,
                ringtoneUri);
        startActivityForResult(intent, 1);
    }

    /*
     * protected Uri onRestoreRingtone(int mRingtoneType) {
     * Uri actualUri = RingtoneManager.getActualDefaultRingtoneUri(this,
     * mRingtoneType); 
     * Log.i("URL", actualUri.getPath() + " "); 
     * return actualUri != null ? actualUri : null; }
     */
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode != RESULT_OK) {
            return;
        }

        if (requestCode == 1) {
            try {
                // Uri uri = data!=null ? data.getData() : null;
                // 得到我们选择的铃声
                Uri uri = data
                        .getParcelableExtra(RingtoneManager.EXTRA_RINGTONE_PICKED_URI);
                // 将我们选择的铃声选择成默认
                if (uri != null) {
                    RingtoneManager
                            .setActualDefaultRingtoneUri(AlarmActivity.this,
                                    RingtoneManager.TYPE_ALARM, uri);
                } else
                    return;

                rings_uri = uri.toString();
                Log.v("you", "rings_uri= " + uri.toString()
                        + " **********************************");
                //通过uri查询闹铃文件
                Cursor cursor = this.getContentResolver().query(uri,
                        CURSOR_COLS, null, null, null);
                cursor.moveToFirst();
                // 显示的闹钟铃声
                String rings_name = cursor.getString(cursor
                        .getColumnIndex(MediaStore.Audio.Media.DISPLAY_NAME));
                Log.v("you", "rings_name= " + rings_name);  
                
                rings = rings_name.substring(0,rings_name.lastIndexOf(".") ).toLowerCase();
                cursor.close();

                saveChange();
                setListItem();

            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    // 将int类型转化为string
    public String format(int i) {
        String s = "" + i;
        if (s.length() == 1)
            s = "0" + s;
        return s;
    }

    // 保存闹钟数据
    public void saveChange() {

        ContentValues cv = new ContentValues();
        cv.put(DBOpenHelper.CLOCK_ISOPEN, isOpen);
        cv.put(DBOpenHelper.CLOCK_DATE, date);
        cv.put(DBOpenHelper.CLOCK_TIME, time);
        cv.put(DBOpenHelper.CLOCK_ISREPEAT, isRepeat);
        cv.put(DBOpenHelper.CLOCK_ISVIBRATE, isVibrate);
        cv.put(DBOpenHelper.CLOCK_RINGS, rings);
        cv.put(DBOpenHelper.CLOCK_URI, rings_uri);

        if (id < 0)
            return;
        Cursor c = dbo.getClock(AlarmActivity.this, id);
        if (!c.moveToFirst()) {
            cv.put(DBOpenHelper.CLOCK_ID, id);
            dbo.insertClock(AlarmActivity.this, id, cv);
        } else {
            dbo.updateClock(AlarmActivity.this, id, cv);
        }
        c.close();
        //dbo.close();
    }
    
    //wangsl
    private Calendar praseDate() {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        try {
            Date settingDate = dateFormat.parse(date);
            Calendar settingCal = Calendar.getInstance();
            settingCal.setTime(settingDate);
            return settingCal;
        }catch (ParseException e) {Log.e(TAG,"setDate fail!!!");}
        
        return null;
    }
    
    private Calendar praseTime() {
        SimpleDateFormat dateFormat = new SimpleDateFormat("HH:mm");
        try {
            Date settingDate = dateFormat.parse(time);
            Calendar settingCal = Calendar.getInstance();
            settingCal.setTime(settingDate);
            
            return settingCal;
        }catch (ParseException e) {Log.e(TAG,"setTime fail!!!");}
        
        return null;
    }   
    //wangsl
    	
	@Override
	protected void onStop() {
		// TODO Auto-generated method stub
		//if(cursor_Clock!=null)cursor_Clock.close();
		//if(cursor_ClocksEnable!=null)cursor_ClocksEnable.close();
		super.onStop();
	}
	
	public void ListCusorSet(){
		Cursor cursor = dbo.getClock(AlarmActivity.this, id);
		
		if (cursor.moveToFirst()) {
			
			isOpen = cursor.getInt(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_ISOPEN));
			date = cursor.getString(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_DATE));
			time = cursor.getString(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_TIME));
			isRepeat = cursor.getInt(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_ISREPEAT));
			isVibrate = cursor.getInt(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_ISVIBRATE));
			rings = cursor.getString(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_RINGS));
			rings_uri = cursor.getString(cursor
					.getColumnIndex(DBOpenHelper.CLOCK_URI));
		} else { // 闹钟项不存在时
			isOpen = 0;
			isRepeat = 0;
			// isVibrate = getResources().getString(R.string.no);
			isVibrate = 0;
			// rings = getResources().getString(R.string.default_rings_name);
			rings = "";
			Log.v("you", "onCreate() :rings= " + rings);
		}
		if(cursor !=null){
			cursor.close();
		}
	}
		@Override
		public void onResume() {
			ListCusorSet();
			setListItem();

			super.onResume();
		}

		@Override
		protected void onDestroy() {
			// TODO Auto-generated method stub
			super.onDestroy();
		}
}
