
package com.sprd.engineermode.debuglog;

import java.util.List;
import java.util.ArrayList;
import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.sprd.engineermode.R;
import com.sprd.engineermode.debuglog.SwitchBaseActivity;

public class SwitchMachineAdapter extends BaseAdapter {

    private static final int MAX_UPDATE_ITEM_NUM = 10;
    private boolean mIsScrolling = false;
    private Context mContext;
    private LayoutInflater mInflater;

    private final String TAG = "SwitchMachineAdapter";

    private List<String> mTimeArray;
    private List<String> mBootModeArray;
    private long mCount = 0;
    private int mMode = SwitchBaseActivity.MODE_POWER_ON;

    private List<String> tempTimeArray = new ArrayList<String>();
    private List<String> tempModeArray = new ArrayList<String>();

    public SwitchMachineAdapter(Context mContext, List<String> timelArray,
            List<String> modeArray, int mode) {

        mInflater = LayoutInflater.from(mContext);
        this.mContext = mContext;

        this.mTimeArray = timelArray;
        this.mBootModeArray = modeArray;
        this.mMode = mode;

        tempTimeArray.clear();
        tempModeArray.clear();

        int markSize = mTimeArray.size();
        if(markSize > modeArray.size()) {
            markSize = modeArray.size();
        }
        Log.d(TAG, "markSize = " + markSize);
        for (int i = 0; i < markSize; i++) {
            Log.d(TAG, "timelArray.get(" + i + ") = " + timelArray.get(i));
            Log.d(TAG, "tempModeArray.get(" + i + ") = " + modeArray.get(i));
            tempTimeArray.add(0, timelArray.get(i));
            tempModeArray.add(0, modeArray.get(i));
        }
    }

    public void setScrollingBlean(boolean scrolling) {
        mIsScrolling = scrolling;
    }

    @Override
    public int getCount() {
        Log.e(TAG, "mIsScrolling is :" + mIsScrolling);

        if (mIsScrolling) {
            int markSize = mTimeArray.size();
            if(markSize > mBootModeArray.size()) {
                markSize = mBootModeArray.size();
            }
            tempTimeArray.clear();
            tempModeArray.clear();
            for (int i = 0; i < markSize; i++) {
                tempTimeArray.add(0, mTimeArray.get(i));
                tempModeArray.add(0, mBootModeArray.get(i));
            }
            return mTimeArray.size();
        } else {
            int markSize = mTimeArray.size();
            if(markSize > mBootModeArray.size()) {
                markSize = mBootModeArray.size();
            }
            if (markSize <= 0) {
                return 0;
            } else if (markSize > MAX_UPDATE_ITEM_NUM) {
                tempTimeArray.clear();
                tempModeArray.clear();
                for (int i = 0; i < MAX_UPDATE_ITEM_NUM; i++) {
                    tempTimeArray.add(mTimeArray.get(markSize - i - 1));
                    tempModeArray.add(mBootModeArray.get(markSize - i - 1));
                }
                return tempTimeArray.size();
            } else {
                tempTimeArray.clear();
                tempModeArray.clear();
                for (int i = 0; i < markSize; i++) {
                    tempTimeArray.add(0, mTimeArray.get(i));
                    tempModeArray.add(0, mBootModeArray.get(i));
                }
                return tempTimeArray.size();
            }
        }
    }

    @Override
    public Object getItem(int position) {
        return tempTimeArray.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup viewGroup) {
        Log.d(TAG, "position = " + position + " time = " + tempTimeArray.get(position) + " mode = "
                + tempModeArray.get(position));
        ViewHolder holder;
        if (convertView == null) {
            holder = new ViewHolder();
            convertView = mInflater.inflate(R.layout.item, null);
            holder.num = (TextView) convertView.findViewById(R.id.num);
            holder.time = (TextView) convertView.findViewById(R.id.time);
            holder.mode = (TextView) convertView.findViewById(R.id.mode);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }
        String bootMode = tempModeArray.get(position);

        switch (mMode) {
            case SwitchBaseActivity.MODE_POWER_ON: {
                int modeId = R.string.unknown_power_on;
                if ( bootMode == null ) {
                    modeId = R.string.unknown;
                } else if (bootMode.contains("engtest")) {
                    modeId = R.string.engtest;
                } else if (bootMode.contains("alarm")) {
                    modeId = R.string.alarm;
                } else if (bootMode.contains("charger")) {
                    modeId = R.string.charger;
                } else if (bootMode.contains("unknown")) {
                    modeId = R.string.unknown;
                } else if (bootMode.contains("wdgreboot")) {
                    modeId = R.string.wdgreboot;
                } else if (bootMode.contains("panic")) {
                    modeId = R.string.panic;
                } else if (bootMode.contains("special")) {
                    modeId = R.string.special;
                }
                holder.mode.setText(modeId);
                break;
            }
            case SwitchBaseActivity.MODE_POWER_OFF: {
                int modeId = R.string.unknown_power_off;
                if ( bootMode == null ) {
                    modeId = R.string.unknown;
                } else if (bootMode.contains("timer")) {
                    modeId = R.string.timer;
                } else if (bootMode.contains("power_shutdown")) {
                    modeId = R.string.power_shutdown_key;
                } else if (bootMode.contains("power_longpress_reboot")) {
                    modeId = R.string.power_longpress_reboot_key;
                } else if (bootMode.contains("power_reboot")) {
                    modeId = R.string.power_reboot_key;
                } else if (bootMode.contains("recovery")) {
                    modeId = R.string.recovery;
                } else if (bootMode.contains("no_power")) {
                    modeId = R.string.battery;
                }else if (bootMode.contains("over_temp")) {
                    modeId = R.string.over_temp;
                }
                holder.mode.setText(modeId);
                break;
            }
            case SwitchBaseActivity.MODE_MODEM_ASSERT:
                holder.mode.setText(bootMode);
                break;
            default:
                holder.mode.setText(bootMode);

        }

        holder.num.setText(Integer.toString(position + 1));
        holder.time.setText(tempTimeArray.get(position));

        return convertView;
    }

    static class ViewHolder {
        public TextView num;
        public TextView time;
        public TextView mode;
    }
}
