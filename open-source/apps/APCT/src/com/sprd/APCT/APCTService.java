package com.sprd.APCT;

import com.sprd.APCT.R;
import com.sprd.APCT.APCTContextUtil;
import java.util.Timer;

import android.app.Activity;
import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.widget.ImageView;
import android.widget.TextView;
import java.io.FileWriter;
import java.io.FileReader;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import android.widget.Button;
import android.widget.ImageButton;
import android.graphics.PixelFormat;
import android.content.Context;
import android.provider.Settings;
import android.content.ContentResolver;
import android.graphics.Color;
import java.util.HashMap;
import java.util.Map;
import android.os.Binder;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.content.res.Resources;
import android.util.SparseArray;
import android.os.Message;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.WindowManager;

public class APCTService extends Service {

    static final String TAG = "APCTService";
    private static WindowManager wm;
    TextView     app_txt;
    TextView     fps_txt;
    int          delaytime = 500;
    static final int FLOAT_WIN_NB = 5;
    static final int FLOAT_VIEW_CLOSE = 0;
    static final int FLOAT_VIEW_RUNING_MAX = 1;
    static final int FLOAT_VIEW_RUNING_MIN = 2;
    boolean mIslaunchTime;
    boolean mIsFps;
    boolean mIsMeminfo;
    boolean mIsBootTime;
    boolean mIsCamTime;
    boolean mIsNetTime;
    boolean mIsAppData;
    boolean mIsBootData;
    boolean mIsPowerOff;
    boolean mIsChargeTime;
    boolean mIsRetunToIdleTime;
    boolean mIsTopRecorder;
    boolean mIsProcrankRecorder;
    boolean mIsTopThreadStarted = false;
    boolean mIsProcrankThreadStarted = false;
    boolean mIsChipTemp;
    String  mTopStr;
    String  mProcStr;
    serviceReceiver receiver;
    Context mContext;
    SharedPreferences sp;
    Drawable mSmallIcon;
    class float_win_data
    {
        Button maxView;
        WindowManager.LayoutParams maxParams;
        ImageButton minView;
        WindowManager.LayoutParams minParams;
        int win_stat;
    };

    private SparseArray<float_win_data> float_data = new SparseArray<float_win_data>(FLOAT_WIN_NB);

	@Override
	public void onCreate() {
		Log.d("APCTService", "onCreate");
		super.onCreate();

        mContext = APCTContextUtil.getInstance();
        sp = mContext.getSharedPreferences("checkbox", Context.MODE_PRIVATE);

        mIslaunchTime   = sp.getBoolean("APP_LAUNCH_TIME", false);
        mIsFps          = sp.getBoolean("FPS_DISPLAY",false);
        mIsMeminfo      = sp.getBoolean("MEMINFO_DISPLAY",false);
        mIsBootTime     = sp.getBoolean("BOOT_TIME",false);
        mIsCamTime      = sp.getBoolean("CAM_INIT_TIME",false);
        mIsNetTime      = sp.getBoolean("NET_TIME",false);
        mIsAppData      = sp.getBoolean("APP_DATA",false);
        mIsBootData     = sp.getBoolean("BOOT_DATA",false);
        mIsPowerOff     = sp.getBoolean("PWR_OFF_TIME",false);
        mIsChargeTime   = sp.getBoolean("PWR_CHARGE_TIME",false);
        mIsRetunToIdleTime   = sp.getBoolean("HOME_IDLE_TIME",false);
        mIsTopRecorder       = sp.getBoolean("TOP_RECORDER",false);;
        mIsProcrankRecorder  = sp.getBoolean("PROCRANK_RECORDER",false);
        mIsChipTemp     = sp.getBoolean("CHIP_TEMP",false);

        for (int i = 0; i < FLOAT_WIN_NB; i++)
        {
            float_data.put(i, new float_win_data());
        }

        Resources r = getResources();
        int w = 100;
        int h = 30;
        mSmallIcon = r.getDrawable(R.drawable.popup_smaller);
        mSmallIcon.setBounds(0, 0, w, h);

        receiver = new serviceReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction("com.sprd.APCT.APCTService.status_changed");
        filter.addAction("com.sprd.APCT.APCTService.app_data");
        registerReceiver(receiver, filter);

        wm = (WindowManager) getApplicationContext().getSystemService(Context.WINDOW_SERVICE);
        displayBaseData();
        displayAppData();
        displayBootData();
        displayTopData();
        displayProcrankData();
    }

    private void displayMinView(int index)
    {
        if (float_data.get(index).win_stat != FLOAT_VIEW_RUNING_MIN)
        {
            destroyMaxView(index);
            float_data.get(index).win_stat = FLOAT_VIEW_RUNING_MIN;
            createMinView(index);
        }
    }

    private boolean isBaseDataChecked()
    {
        if (mIslaunchTime || mIsFps || mIsMeminfo || mIsBootTime 
            || mIsCamTime || mIsNetTime || mIsPowerOff || mIsChargeTime
            || mIsRetunToIdleTime || mIsChipTemp)
        {
            return true;
        }
        return false;    
    }

    private void displayView(int index)
    {
        switch (index)
        {
        case 0:
            displayBaseData();
            break;

        case 1:
            displayAppData();
            break;

        case 2:
            displayBootData();
            break;

        case 3:
            displayTopData();
            break;

        case 4:
            displayProcrankData();
            break;

        }
    }

    private void displayBaseData()
    {
        if (isBaseDataChecked())
        {
            if (float_data.get(0).win_stat != FLOAT_VIEW_RUNING_MIN)
            {
                displayMaxView(0);
            }
            else
            {
                displayMinView(0);
            }    
        }
        else
        {
            destroyMaxView(0);
            destroyMinView(0);
            float_data.get(0).win_stat = FLOAT_VIEW_CLOSE;
        }
    }

    private void displayAppData()
    {
        if (mIsAppData)
        {
            if (float_data.get(1).win_stat != FLOAT_VIEW_RUNING_MIN)
            {
                displayMaxView(1);
            }
            else
            {
                displayMinView(1);
            }
        }
        else
        {
            destroyMaxView(1);
            destroyMinView(1);
            float_data.get(1).win_stat = FLOAT_VIEW_CLOSE;
        }
    }

    private void displayBootData()
    {
        if (mIsBootData)
        {
            if (float_data.get(2).win_stat != FLOAT_VIEW_RUNING_MIN)
            {
                displayMaxView(2);
            }
            else
            {
                displayMinView(2);
            }    
        }
        else
        {
            destroyMaxView(2);
            destroyMinView(2);
            float_data.get(2).win_stat = FLOAT_VIEW_CLOSE;
        }
    }

    private void displayTopData()
    {
        if (mIsTopRecorder)
        {
            if (float_data.get(3).win_stat != FLOAT_VIEW_RUNING_MIN)
            {
                displayMaxView(3);
            }
            else
            {
                displayMinView(3);
            }    
        }
        else
        {
            destroyMaxView(3);
            destroyMinView(3);
            float_data.get(3).win_stat = FLOAT_VIEW_CLOSE;
        }
    }

    private void displayProcrankData()
    {
        if (mIsProcrankRecorder)
        {
            if (float_data.get(4).win_stat != FLOAT_VIEW_RUNING_MIN)
            {
                displayMaxView(4);
            }
            else
            {
                displayMinView(4);
            }    
        }
        else
        {
            destroyMaxView(4);
            destroyMinView(4);
            float_data.get(4).win_stat = FLOAT_VIEW_CLOSE;
        }
    }

    private void displayMaxView(int index)
    {
        if (getItemCheckedStat(index))
        {
            createMaxView(index);
            setFloatViewtext(index);
            wm.updateViewLayout(float_data.get(index).maxView, float_data.get(index).maxParams);
        }
    }

    private boolean getItemCheckedStat(int index)
    {
        boolean checked = false;

        switch (index)
        {
        case 0:
            checked = isBaseDataChecked();
            break;
        case 1:
            checked = mIsAppData;
            break;
        case 2:
            checked = mIsBootData;
            break;
        case 3:
            checked = mIsTopRecorder;
            break;
        case 4:
            checked = mIsProcrankRecorder;
            break;
        }
        return checked;
    }

    private void createMinView(int idx)
    {
        final int index = idx;
        float_data.get(index).minView   = new ImageButton(getApplicationContext());
        float_data.get(index).minParams = new WindowManager.LayoutParams();

        float_data.get(index).minParams.type    = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
        float_data.get(index).minParams.format  = PixelFormat.RGBA_8888;
        float_data.get(index).minParams.flags   = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL |
                                                                            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        float_data.get(index).minParams.gravity = Gravity.LEFT | Gravity.TOP;
        float_data.get(index).minParams.width   = 80;
        float_data.get(index).minParams.height  = 80;
        float_data.get(index).minParams.x       = wm.getDefaultDisplay().getWidth() - float_data.get(index).minParams.width;
        float_data.get(index).minParams.y       = (index + 1) * 100;
        float_data.get(index).minParams.format  = 1;
        wm.addView(float_data.get(index).minView, float_data.get(index).minParams);

        float_data.get(index).minView.setImageDrawable(getResources().getDrawable(R.drawable.minal_icon));

        float_data.get(index).minView.setOnTouchListener(new OnTouchListener()
        {
            int lastX, lastY;
            int paramX, paramY;
            public boolean onTouch(View v, MotionEvent event)
            {
                switch(event.getAction())
                {
                case MotionEvent.ACTION_DOWN:
                    lastX = (int) event.getRawX();
                    lastY = (int) event.getRawY();
                    paramX = float_data.get(index).minParams.x;
                    paramY = float_data.get(index).minParams.y;
                    break;

                case MotionEvent.ACTION_MOVE:
                    int dx = (int) event.getRawX() - lastX;
                    int dy = (int) event.getRawY() - lastY;
                    if (dx <= 8 && dy <= 8)
                    {
                        break;
                    }
                    float_data.get(index).minParams.x = paramX + dx;
                    float_data.get(index).minParams.y = paramY + dy;
                    wm.updateViewLayout(float_data.get(index).minView, float_data.get(index).minParams);
                    break;

                case MotionEvent.ACTION_UP:
                    int x = (int)event.getRawX();
                    int y = (int)event.getRawY();

                    if ((x - lastX <= 8) && (y - lastY) <= 8)
                    {
                        destroyMinView(index);
                        displayView(index);
                        float_data.get(index).win_stat = FLOAT_VIEW_RUNING_MAX;
                    }
                    break;
             }
             return true;
         }
	 });        
    }

    private void createMaxView(int idx)
    {
        if (float_data.get(idx).win_stat == FLOAT_VIEW_RUNING_MAX)
        {
            return;
        }

        final int index = idx;
        int x      = 0;
        int y      = 0;
        int width  = 350;
        int height = 200;
        float_data.get(index).win_stat = FLOAT_VIEW_RUNING_MAX;
        float_data.get(index).maxView = new Button(getApplicationContext());
        float_data.get(index).maxView.setBackgroundColor(Color.GRAY);
        float_data.get(index).maxView.getBackground().setAlpha(200);
        float_data.get(index).maxView.setTextSize(8);
        float_data.get(index).maxView.setGravity(Gravity.LEFT|Gravity.TOP);
        float_data.get(index).maxView.setTextColor(Color.WHITE);
        float_data.get(index).maxView.setShadowLayer(3, 3.0f, 3.0f, Color.BLACK); 
        float_data.get(index).maxView.setCompoundDrawables(null, mSmallIcon, null, null);

        float_data.get(index).maxParams         = new WindowManager.LayoutParams();
        float_data.get(index).maxParams.type    = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
        float_data.get(index).maxParams.format  = PixelFormat.RGBA_8888;
        float_data.get(index).maxParams.flags   = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL |          
                                                                              WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;

        float_data.get(index).maxParams.gravity = Gravity.LEFT | Gravity.TOP;

        if (index == 0)
        {
            width  = 350;
            height = 300;
            x      = wm.getDefaultDisplay().getWidth() - width;
            y      = 0;
        }
        else
        if (index == 1)
        {
            width  = 400;
            height = 300;
            x      = 0;
            y      = 0;
        }
        else
        if (index == 2)
        {
            width  = wm.getDefaultDisplay().getWidth() > 450 ? 450 : wm.getDefaultDisplay().getWidth();
            height = (wm.getDefaultDisplay().getHeight() - 20)  > 750 ? 750 : (wm.getDefaultDisplay().getHeight() - 20);
            x      = 0;
            y      = 0;
        }
        else
        if (index == 4)
        {
            DisplayMetrics dm = new DisplayMetrics();
            dm = getResources().getDisplayMetrics();
            width = Math.min(dm.widthPixels, dm.heightPixels);
            height = dm.heightPixels / 2;
        }
        float_data.get(index).maxParams.width   = width;
        float_data.get(index).maxParams.height  = height;
        float_data.get(index).maxParams.x       = wm.getDefaultDisplay().getWidth() - float_data.get(index).maxParams.width;
        float_data.get(index).maxParams.y       = 0;
        float_data.get(index).maxParams.format  = 1;
        wm.addView(float_data.get(index).maxView, float_data.get(index).maxParams);
        
        if (index == 0)
        {
            handler.postDelayed(task, delaytime);
        }

        float_data.get(index).maxView.setOnTouchListener(new OnTouchListener()
        {
            int lastX, lastY;
            int paramX, paramY;

            public boolean onTouch(View v, MotionEvent event)
            {
                switch(event.getAction())
                {
                case MotionEvent.ACTION_DOWN:
                    lastX = (int) event.getRawX();
                    lastY = (int) event.getRawY();
                    paramX = float_data.get(index).maxParams.x;
                    paramY = float_data.get(index).maxParams.y;
                    break;
                case MotionEvent.ACTION_MOVE:
                    int dx = (int) event.getRawX() - lastX;
                    int dy = (int) event.getRawY() - lastY;
                    if (dx <= 8 && dy <= 8)
                    {
                        break;
                    }

                    float_data.get(index).maxParams.x = paramX + dx;
                    float_data.get(index).maxParams.y = paramY + dy;
                    wm.updateViewLayout(float_data.get(index).maxView, float_data.get(index).maxParams);
				    break;

                case MotionEvent.ACTION_UP:
                    int x = (int)event.getRawX();
                    int y = (int)event.getRawY();

                    if ((x - lastX <= 8) && (y - lastY) <= 8)
                    {
                        displayMinView(index);
                    }
                    break;
             }
             return true;
         }
	 });
    }

    private void updateItemStatus(int index)
    {
       switch (index)
       {
            case 0: mIslaunchTime = sp.getBoolean("APP_LAUNCH_TIME", false);
                    displayBaseData();
                    break;

            case 1: mIsFps = sp.getBoolean("FPS_DISPLAY",false);
                    displayBaseData();
                    break;

            case 2: mIsMeminfo = sp.getBoolean("MEMINFO_DISPLAY",false);
                    displayBaseData();
                    break;

            case 3: mIsBootTime = sp.getBoolean("BOOT_TIME",false);
                    displayBaseData();
                    break;

            case 4: mIsCamTime = sp.getBoolean("CAM_INIT_TIME",false);
                    displayBaseData();
                    break;

            case 5: mIsNetTime = sp.getBoolean("NET_TIME",false);
                    displayBaseData();
                    break;

            case 6: mIsAppData = sp.getBoolean("APP_DATA",false);
                    displayAppData();
                    break;

            case 7: mIsBootData = sp.getBoolean("BOOT_DATA",false);
                    displayBootData();
                    break;

            case 8: mIsPowerOff = sp.getBoolean("PWR_OFF_TIME",false);
                    displayBaseData();
                    break;

            case 9: mIsChargeTime = sp.getBoolean("PWR_CHARGE_TIME",false);
                    displayBaseData();
                    break;

            case 10:
                    mIsRetunToIdleTime = sp.getBoolean("HOME_IDLE_TIME",false);
                    displayBaseData();
                    break;

            case 11:
                    mIsTopRecorder = sp.getBoolean("TOP_RECORDER",false);
                    displayTopData();
                    break;

            case 12:
                    mIsProcrankRecorder = sp.getBoolean("PROCRANK_RECORDER",false);
                    displayProcrankData();
                    break;

            case 13:
                    mIsChipTemp = sp.getBoolean("CHIP_TEMP",false);
                    displayBaseData();
                    break;

            case 100:
                    mIslaunchTime   = sp.getBoolean("APP_LAUNCH_TIME", false);
                    mIsFps          = sp.getBoolean("FPS_DISPLAY",false);
                    mIsMeminfo      = sp.getBoolean("MEMINFO_DISPLAY",false);
                    mIsBootTime     = sp.getBoolean("BOOT_TIME",false);
                    mIsCamTime      = sp.getBoolean("CAM_INIT_TIME",false);
                    mIsNetTime      = sp.getBoolean("NET_TIME",false);
                    mIsAppData      = sp.getBoolean("APP_DATA",false);
                    mIsBootData     = sp.getBoolean("BOOT_DATA",false);
                    mIsPowerOff     = sp.getBoolean("PWR_OFF_TIME",false);
                    mIsChargeTime   = sp.getBoolean("PWR_CHARGE_TIME",false);
                    mIsRetunToIdleTime   = sp.getBoolean("HOME_IDLE_TIME",false);
                    mIsTopRecorder       = sp.getBoolean("TOP_RECORDER",false);;
                    mIsProcrankRecorder  = sp.getBoolean("PROCRANK_RECORDER",false);
                    mIsChipTemp   = sp.getBoolean("CHIP_TEMP",false);
                    displayBaseData();
                    displayTopData();
                    displayTopData();
                    displayProcrankData();
                    break;

            default:
                    break;
        }
    }

    private String getChipTemp()
    {
        final String FPS_PROC = "/sys/class/thermal/thermal_zone0/temp";
        FileReader fr = null;
        BufferedReader reader = null;
        String str = "Chip Temp Degree: ";
        String str_tmp = null;

        try {
            reader = new BufferedReader(new FileReader(FPS_PROC));
            str_tmp = reader.readLine();
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }finally {
            try {
                if (fr != null)
                {
                    fr.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        str += str_tmp;

        return str;
    }

    private String getFps()
    {
        final String FPS_PROC = "/proc/benchMark/fps";
        FileReader fr = null;
        BufferedReader reader = null;
        String str = "";

        try {
            reader = new BufferedReader(new FileReader(FPS_PROC));
            str = reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        return str;
    }

    private String getMemInfo()
    {
        String line = "";
        String result = null;
        int line_count  = 0;
        boolean isOnlyDisp6Line = false;

        try{
            Process p = Runtime.getRuntime().exec("cat /proc/meminfo");
            BufferedReader in = null;
            in = new BufferedReader(new InputStreamReader(p.getInputStream()));

            if (in != null)
            {
                while ((line = in.readLine()) != null)
                {
                    if (result == null)
                        result = line;
                    else
                        result += "\n" + line;
                    line_count++;

                    if (line_count == 5)
                    {
                        break;
                    }
                 }
                 in.close();
             }
        }
        catch(Throwable t)
        {
            t.printStackTrace();
        }
        return result;
    }

    private String getAppLaunchTime()
    {
        final String APP_PROC = "/proc/benchMark/app_launch_time";
        BufferedReader reader = null;
        String str = null;

        try {
            reader = new BufferedReader(new FileReader(APP_PROC));
            str = reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        if (str == null ||str.charAt(0) == '\0')
        {
            str = "App Launch Time: null";
        }
        return str;
    }

    private String getAppLaunchData()
    {
        final String APP_PROC = "/proc/benchMark/app_launch_data";
        BufferedReader reader = null;
        String str = "ACTIVITY LAUNCH PROCESS:";
        String str_tmp = "";
        try {
            reader = new BufferedReader(new FileReader(APP_PROC));
            while ((str_tmp = reader.readLine()) != null)
            {
                str += "\n" + str_tmp;
            }
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }
        return str;
    }

    public static void writeBootDataToProc(String str)
    {
        final String BOOT_DATA_PROC = "/proc/bootperf";
        char[] buffer = str.toCharArray();

        FileWriter fr = null;
         try {
            fr = new FileWriter(BOOT_DATA_PROC, true);
            if (fr != null)
            {
                fr.write(buffer);
            }
        }catch (IOException e) {
            e.printStackTrace();
        }finally {
            try{
                if (fr != null){
                    fr.close();
                }
            }catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private String getBootData()
    {
        final String APP_PROC = "/proc/bootperf";
        BufferedReader reader = null;
        String str = "BOOT PROCESS:";
        String str_tmp = "";
        String read_flg = "1";

        writeBootDataToProc(read_flg);

        try {
            reader = new BufferedReader(new FileReader(APP_PROC));
            while ((str_tmp = reader.readLine()) != null)
            {
                str += "\n" + str_tmp;
            }
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        return str;
    }

    private String getPwronTime()
    {
        final String BOOT_TIME_PROC = "/proc/benchMark/boot_time";
        BufferedReader reader = null;
        String str = null;

        try {
            reader = new BufferedReader(new FileReader(BOOT_TIME_PROC));
            str = reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        if (str == null ||str.charAt(0) == '\0')
        {
            str = "Boot Time: null";
        }

        return str;
    }

    private String getNetSearchTime()
    {
        final String NET_TIME_PROC = "/proc/benchMark/net_time";
        FileReader fr = null;
        BufferedReader reader = null;
        String str = null;
        String tmp_str;

        try {
            reader = new BufferedReader(new FileReader(NET_TIME_PROC));
            while ((tmp_str = reader.readLine()) != null) {
                if (str == null)
                    str = tmp_str;
                else
                    str += "\n" + tmp_str;
            }
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        if (str == null ||str.charAt(0) == '\0')
        {
            str = "Net Search Time: null";
        }
        return str;
    }

    private String getCameraInitTime()
    {
        final String CAM_TIME_PROC = "/proc/benchMark/cam_time";
        BufferedReader reader = null;
        String str = null;

        try {
            reader = new BufferedReader(new FileReader(CAM_TIME_PROC));
            str = reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        if (str == null  || str.charAt(0) == '\0')
        {
            str = "Camera Init Time: null";
        }
        return str;
    }

    private String getRereturnToIdleTime()
    {
        String line = null;
        BufferedReader reader = null;
        final String filename = "/proc/benchMark/gohome_time";
        String str = null;

        try {
            reader = new BufferedReader(new FileReader(filename));
            str=reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        if (str == null  || str.charAt(0) == '\0')
        {
            str = "Home to Idle Time: null";
        }

        return str;
    }

    private String getChargeTime()
    {
        String line = null;
        BufferedReader reader = null;
        final String filename = "/data/apct/chargetime";
        String str = null;

        try {
            reader = new BufferedReader(new FileReader(filename));
            str=reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

       if (str == null ||str.charAt(0) == '\0')
        {
            str = "Power Off Charge Time: null";
        }

        return str;
    }

    private String getPowerOffTime()
    {
        String line = null;
        BufferedReader reader = null;
        final String filename = "/data/apct/shutdowntime";
        String str = null;

        try {
            reader = new BufferedReader(new FileReader(filename));
            str=reader.readLine();
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try {
                if (reader != null)
                {
                    reader.close();
                }
             }catch (IOException e) {
                    e.printStackTrace();
             }
        }

        if (str == null  || str.charAt(0) == '\0')
        {
            str = "Power Off Time: null";
        }

        return str;
    }

    //private Handler handler = new Handler();
    private Runnable task = new Runnable()
    {
        public void run() 
        {
            if (float_data.get(0).win_stat == FLOAT_VIEW_RUNING_MAX)
            {
                setFloatView0Text();
                wm.updateViewLayout(float_data.get(0).maxView, float_data.get(0).maxParams);
                handler.postDelayed(this, delaytime);
            }
        }
    };

    public void setFloatViewtext(int index)
    {
        switch (index)
        {
        case 0:
            setFloatView0Text();
            break;

        case 1:
            setFloatView1Text();
            break;

        case 2:
            setFloatView2Text();
            break;

        case 3:
            setFloatView3Text();
            break;

        case 4:
            setFloatView4Text();
            break;
        }
    }

    public void setFloatView0Text()
    {
        String float_info = "";

        if (mIsChipTemp)
        {
            float_info += getChipTemp() + "\n";
        }

        if (mIslaunchTime)
        {
            float_info += getAppLaunchTime() + "\n";
        }

        if (mIsFps)
        {
            float_info += getFps() + "\n";
        }

        if (mIsBootTime)
        {
            float_info += getPwronTime() + "\n";
        }

        if (mIsCamTime)
        {
            float_info +=  getCameraInitTime() + "\n";
        }

        if (mIsNetTime)
        {
            float_info += getNetSearchTime() + "\n";
        }

        if (mIsPowerOff)
        {
            float_info += getPowerOffTime() + "\n";
        }

        if (mIsChargeTime)
        {
            float_info += getChargeTime() + "\n";
        }

        if (mIsRetunToIdleTime)
        {
            float_info += getRereturnToIdleTime() + "\n";
        }

        if (mIsMeminfo)
        {
            float_info += getMemInfo();
        }

        float_data.get(0).maxView.setText(float_info);
    }

    public void setFloatView1Text()
    {
        String float_info = "";

        float_info = getAppLaunchData();
        float_data.get(1).maxView.setText(float_info);        
    }

    public void setFloatView2Text()
    {
        String float_info = "";

        float_info = getBootData();
        float_data.get(2).maxView.setText(float_info);
    }

    public void setFloatView3Text()
    {
        float_data.get(3).maxView.setText(mTopStr);
        if (!mIsTopThreadStarted)
        {
            mIsTopThreadStarted = true;
            top_thread.start();
        }
    }

    public void setFloatView4Text()
    {
        float_data.get(4).maxView.setText(mProcStr);
        if (!mIsProcrankThreadStarted)
        {
            mIsProcrankThreadStarted = true;
            proc_thread.start();
        }
    }

    Thread top_thread = new Thread()
    {  
         public void run()
         {
              String cmd_str = "top -m 5";
              try{  
                   Process p = Runtime.getRuntime().exec(cmd_str);  
                   BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));   
                   
                   String line = "";    
                   String result = "";
                   int line_count = 0;

                   while ((line = in.readLine()) != null) {
                      String[] str = line.split("\\s+");
                                           
                      if (str.length == 11)
                      {
                          if (line_count == 0)
                          {
                              result = "";
                          }

                          line_count++;
                          result += str[1] + "   " + str[2] + "   " + str[3] + "   " + str[4] + "   " + str[10] + "\n";
                          if (line_count % 6 == 0)
                          {
                              Message msg = handler.obtainMessage(0, result);
 	                          handler.sendMessage(msg);
                              line_count = 0;
                          }                          
                      }
                  }
                  in.close();
              }catch(IOException ioe){
                  System.out.println(ioe.getMessage());
              }
         }
     };

     Thread proc_thread = new Thread()
     {  
         public void run()
         {
              String cmd_str = "procrank";
              try{
                   Process p = Runtime.getRuntime().exec(cmd_str);  
                   BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));   
                   
                   String line = null;    
                   String result = null;
                   int line_count = 0;
                   while ((line = in.readLine()) != null) {
                      if (result == null)
                          result = line;
                      else
                          result += "\n" + line;
                      line_count++;
                      
                      if (line_count == 30)
                      {
                          break;
                      }
                  }
                  in.close();
                  Message msg = handler.obtainMessage(1, result);
                  handler.sendMessage(msg);
              }catch(IOException ioe){
                  System.out.println(ioe.getMessage());
              }
         }
     };

    public Handler handler = new Handler()
    {
        @SuppressWarnings("unchecked")
        @Override
        public void handleMessage(Message msg)
        {
            String str;

            switch (msg.what)
            {
                case 0:
                mTopStr = (String)msg.obj;
                float_data.get(3).maxView.setText(mTopStr);
                if (float_data.get(3).win_stat == FLOAT_VIEW_RUNING_MAX)
                {
                     wm.updateViewLayout(float_data.get(3).maxView, float_data.get(3).maxParams);
                }
                break;

                case 1:
                mProcStr = (String)msg.obj;
                float_data.get(4).maxView.setText(mProcStr);
                if (float_data.get(4).win_stat == FLOAT_VIEW_RUNING_MAX)
                {
                    wm.updateViewLayout(float_data.get(4).maxView, float_data.get(4).maxParams);
                }
                break;

                default:
                break;
            }
        }
     };

	@Override
	public void onStart(Intent intent, int startId) {
		Log.d("APCTService", "onStart");
		super.onStart(intent, startId);
	}

	@Override
	public void onDestroy() {
		handler.removeCallbacks(task);
		Log.d("APCTService", "onDestroy");
		destroyMaxView(0);
		destroyMaxView(1);
		destroyMaxView(2);
		destroyMaxView(3);		
		destroyMaxView(4);
		destroyMinView(0);
		destroyMinView(1);
		destroyMinView(2);
		destroyMinView(3);		
		destroyMinView(4);
        float_data.get(0).win_stat = FLOAT_VIEW_CLOSE;
        float_data.get(1).win_stat = FLOAT_VIEW_CLOSE;
        float_data.get(2).win_stat = FLOAT_VIEW_CLOSE;
        float_data.get(3).win_stat = FLOAT_VIEW_CLOSE;
        float_data.get(4).win_stat = FLOAT_VIEW_CLOSE;
        unregisterReceiver(receiver);
		super.onDestroy();
	}

    private void destroyMaxView(int index)
    {
        if (float_data.get(index).win_stat == FLOAT_VIEW_RUNING_MAX)
        {
            wm.removeView(float_data.get(index).maxView);
            float_data.get(index).win_stat = FLOAT_VIEW_CLOSE;
        }
    }

    private void destroyMinView(int index)
    {
        if (float_data.get(index).win_stat == FLOAT_VIEW_RUNING_MIN)
        {
            wm.removeView(float_data.get(index).minView);
            float_data.get(index).win_stat = FLOAT_VIEW_CLOSE;
        }              
    }

    private class serviceReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            if ("com.sprd.APCT.APCTService.status_changed".equals(intent.getAction()))
            {
                int index = intent.getIntExtra("cmd", -1);
                updateItemStatus(index);
            }
            else
            if ("com.sprd.APCT.APCTService.app_data".equals(intent.getAction()))
            {
                displayAppData();
            }
        }
    }

    private int getStatusBarHeight()
    {
        Class<?> c = null;
        Object obj = null;
        java.lang.reflect.Field field = null;
        int x = 0;
        int statusBarHeight = 0;
        try
        {
            c = Class.forName("com.android.internal.R$dimen");
            obj = c.newInstance();
            field = c.getField("status_bar_height");
            x = Integer.parseInt(field.get(obj).toString());
            statusBarHeight = getResources().getDimensionPixelSize(x);
         }
         catch (Exception e)
         {
             e.printStackTrace();
         }

         return statusBarHeight;
    }

    @Override
    public IBinder onBind(Intent intent) {
  	return null;
    }    
}
