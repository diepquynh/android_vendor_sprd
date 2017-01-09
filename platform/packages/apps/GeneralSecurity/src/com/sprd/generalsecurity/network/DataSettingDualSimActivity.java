
package com.sprd.generalsecurity.network;

import android.app.ListActivity;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;
import android.content.Context;
import android.widget.BaseAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import android.util.Log;
import com.sprd.generalsecurity.R;

import java.lang.Integer;
import java.lang.Override;
import java.lang.String;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.MenuItem;

import com.sprd.generalsecurity.utils.DateCycleUtils;


public class DataSettingDualSimActivity extends ListActivity implements View.OnClickListener {



    int mCheckedPostionSim1, mCheckedPostionSim2 = -1;
    SettingAdapter adapter;

    SharedPreferences mSharedPref;
    String[] values;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Be sure to call the super class.
        super.onCreate(savedInstanceState);

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);

        setContentView(R.layout.data_setting_dual_sim);
        adapter = new SettingAdapter(this);
        mSharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        mCheckedPostionSim1 = Integer.parseInt(mSharedPref.getString(DateCycleUtils.KEY_DATA_CYCLE_SIM1, "0")) + 1;
        mCheckedPostionSim2 = Integer.parseInt(mSharedPref.getString(DateCycleUtils.KEY_DATA_CYCLE_SIM2, "0")) + 5;

        values = new String[] {
                null,
                getResources().getString(R.string.pref_data_cycle_default),
                getResources().getString(R.string.data_cycle_week),
                getResources().getString(R.string.data_cycle_day),
                null,
                getResources().getString(R.string.pref_data_cycle_default),
                getResources().getString(R.string.data_cycle_week),
                getResources().getString(R.string.data_cycle_day)
        };
        ListView lv = getListView();
        lv.setAdapter(adapter);
    }

    class ViewTag {
        int position;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }

    @Override
    public void onClick(View v) {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        SharedPreferences.Editor edit = sharedPref.edit();
        int position = (int)v.getTag();
        if (position >= 1 && position <= 4) {
            mCheckedPostionSim1 = position;
            edit.putString(DateCycleUtils.KEY_DATA_CYCLE_SIM1, String.valueOf(position % 4 - 1));

        } else if (position > 4 && position <=7 ){
            mCheckedPostionSim2 = position;
            edit.putString(DateCycleUtils.KEY_DATA_CYCLE_SIM2, String.valueOf(position % 4 - 1));
        }
        edit.commit();
        adapter.notifyDataSetChanged();

    }

    class SettingAdapter extends BaseAdapter {
        private LayoutInflater mInflater;

        public SettingAdapter(Context context) {
            this.mInflater = LayoutInflater.from(context);
        }

        @Override
        public int getCount() {
            return 8;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (position == 0) {
                View v = mInflater.inflate(R.layout.data_setting_header, null);
                TextView tv = (TextView)v.findViewById(R.id.head);
                tv.setText(getResources().getString(R.string.data1));

                return v;
            }

            if (position == 4) {
                View v = mInflater.inflate(R.layout.data_setting_header, null);
                TextView tv = (TextView)v.findViewById(R.id.head);
                tv.setText(getResources().getString(R.string.data2));

                return v;
            }

                convertView = mInflater.inflate(R.layout.data_setting_item, null);
//            }
            TextView tv = (TextView) convertView.findViewById(R.id.cycle);
            tv.setText(values[position]);

            RadioButton rd = (RadioButton) convertView.findViewById(R.id.radioButton);
            rd.setTag(position);
            rd.setOnClickListener(DataSettingDualSimActivity.this);
            if (mCheckedPostionSim1 == position ||mCheckedPostionSim2 == position) {
                rd.setChecked(true);
            } else {
                rd.setChecked(false);
            }


            return convertView;
        }
    }
}
