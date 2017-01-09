
package com.sprd.engineermode.debuglog;

import android.widget.ListView;
import android.app.ListActivity;
import android.widget.ArrayAdapter;
import android.os.Bundle;
import java.io.InputStreamReader;
import android.util.Log;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView;
import android.view.View;
import android.content.Intent;
//import com.sprd.engineermode.R;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.PrintStream;

public class CpuFreqListActivity extends ListActivity implements AdapterView.OnItemClickListener {
    private static final String TAG = "CpuFreqListActivity";
    private static final String KEY_SET_FREQ = "set_freq";
    private ListView mCpuFreqList;
    private ArrayAdapter<String> mCpuFreqAdapter;
    private String[] mCpuFreq;
    private String[] mCpuFreqSet;
    private String mAvaliableFreq;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAvaliableFreq = getAvaliableFreq();
        mCpuFreqSet = mAvaliableFreq.split(" ");
        mCpuFreq = changeUnit();
        mCpuFreqAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_single_choice,
                mCpuFreq);
        mCpuFreqList = this.getListView();
        mCpuFreqList.setAdapter(mCpuFreqAdapter);
        mCpuFreqList.setOnItemClickListener(this);
        mCpuFreqList.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        initCpuFreq();
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        String curFreq = mCpuFreqSet[position];
        setFreq(curFreq);
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString(KEY_SET_FREQ, Integer.valueOf(curFreq).intValue() / 1000 + "mHz");
        intent.putExtras(bundle);
        setResult(RESULT_OK, intent);
        finish();
    }

    private String getAvaliableFreq() {
        //begin bug598849 modify by bo.yan 20160914
        String min_freq=readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
        String max_freq=readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
        String avaliable_freq=readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies");
        Log.d(TAG,"min freq is:"+min_freq+"max freq is:"+max_freq+"avaliable freq is:"+avaliable_freq);
        String a_freq_result = "";
        String[] freq_array = avaliable_freq.split(" ");
        for(int i=0;i<freq_array.length;i++) {
            if(Integer.parseInt(freq_array[i])>=Integer.parseInt(min_freq)&&
                    Integer.parseInt(freq_array[i])<=Integer.parseInt(max_freq)){
                a_freq_result = a_freq_result+freq_array[i]+" ";
            }
        }
        return a_freq_result.trim();
      //end bug598849 modify by bo.yan 20160914
    }

    private String[] changeUnit() {
        mCpuFreq = new String[mCpuFreqSet.length];
        for (int i = 0; i < mCpuFreqSet.length; i++) {
            mCpuFreq[i] = Integer.valueOf(mCpuFreqSet[i]).intValue() / 1000 + "mHz";
        }
        return mCpuFreq;
    }

    private void initCpuFreq() {
        String curFreq = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        if (!"".equals(curFreq) && !"readError".equals(curFreq)) {
            for (int i = 0; i < mCpuFreqSet.length; i++) {
                if (curFreq.equals(mCpuFreqSet[i])) {
                    mCpuFreqList.setItemChecked(i, true);
                }
            }
        }
    }

    private void setFreq(String freq) {
        execShellStr("echo 1 > /sys/devices/system/cpu/cpu0/online");
        execShellStr("echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        execShellStr("echo " + freq + " > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
        readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
    }

    // exec shell cmd
    private String execShellStr(String cmd) {
        String[] cmdStrings = new String[] {
                "sh", "-c", cmd
        };
        StringBuffer retString = new StringBuffer("");

        try {
            Process process = Runtime.getRuntime().exec(cmdStrings);
            BufferedReader stdout = new BufferedReader(new InputStreamReader(
                    process.getInputStream(), "UTF-8"), 7777);
            BufferedReader stderr = new BufferedReader(new InputStreamReader(
                    process.getErrorStream(), "UTF-8"), 7777);

            String line = null;

            while ((null != (line = stdout.readLine()))
                    || (null != (line = stderr.readLine()))) {
                if ("" != line) {
                    retString = retString.append(line).append("\n");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, cmd + ":" + retString.toString() + "");
        return retString.toString();
    }

    // read file
    public String readFile(String path) {
        File file = new File(path);
        String str = new String("");
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                str = str + line;
            }
        } catch (Exception e) {
            Log.d(TAG, "Read file error!!!");
            str = "readError";
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
            }
        }
        Log.d(TAG, "read " + path + " value is " + str.trim());
        return str.trim();
    }
}
