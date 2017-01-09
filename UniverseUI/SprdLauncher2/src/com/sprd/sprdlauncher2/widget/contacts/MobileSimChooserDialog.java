/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.widget.contacts;

import java.util.ArrayList;
import com.android.sprdlauncher2.R;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.sim.Sim;
import android.sim.SimManager;
import android.sim.SimManager.*;
import android.sim.SimListAdapter.ViewHolder;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.RelativeLayout;

public class MobileSimChooserDialog extends AlertDialog implements OnSimsUpdateListener,OnItemClickListener {
    protected MobileSimChooserDialog(Context context) {
        super(context);
        mContext = context;
        setTitle(context.getResources().getString(R.string.choose_sub));
        mList=new ListView(context);
        refreshList();
    }

    public interface OnSimPickedListener {
        void onSimPicked(int phoneId) ;
    }
    public static final String SUB_ID = "phoneid";
    SimListAdapter adapter = null;
    Sim[] sims = null;
    private ListView mList;
    private Context mContext;
    private OnSimPickedListener mListener;
    public void setListener(OnSimPickedListener listener) {
        mListener=listener;
    }

    public class SimListAdapter extends BaseAdapter {
        public final class ViewHolder {
            public RelativeLayout color;
            public TextView name;
        }

        private LayoutInflater mInflater;
        private Sim[] mData;
        private OnClickListener mListener;
        private int mLayoutId;

        public SimListAdapter(Context context, Sim[] data,
                OnClickListener listener, int layoutId) {
            this.mInflater = LayoutInflater.from(context);
            this.mData = data;
            this.mListener = listener;
            this.mLayoutId = layoutId;
        }

        public int getCount() {
            return mData.length;
        }

        public Object getItem(int position) {
            return mData[position];
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder = null;
            if (convertView == null) {
                holder = new ViewHolder();
                convertView = mInflater.inflate(mLayoutId, null);
                holder.color = (RelativeLayout) convertView
                        .findViewById(R.id.sim_color);
                holder.name = (TextView) convertView
                        .findViewById(R.id.sim_name);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            if(position <= mData.length && mData[position] != null){
                if(holder.color != null){
                    holder.color.setBackgroundResource(SimManager.COLORS_IMAGES[mData[position]
                            .getColorIndex()]);
                }
                if(holder.name != null){
                    holder.name.setText(mData[position].getName());
                }
            }
            return convertView;
        }
    }

    public void onSimUpdated(Sim[] sims) {
        refreshList();
    }


    private void refreshList(){
        Log.i("refreshList", "BEFORE sims = PhoneUtils.getInsertSims(getContext());");
        sims = getInsertSims(mContext);
        Log.i("refreshList", "AFTER sims = PhoneUtils.getInsertSims(getContext());");
        if (adapter == null) {
            adapter = new SimListAdapter(getContext(), sims, null,R.layout.select_sim);
            mList.setAdapter(adapter);
            mList.setOnItemClickListener(this);
            setView(mList);
        } else {
            adapter.notifyDataSetChanged();
        }

    }

    @Override
    public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
        this.dismiss();
        if (mListener!=null) {
            mListener.onSimPicked(((Sim)adapter.getItem(position)).getPhoneId());
        }
    }

    public  Sim[] getInsertSims(Context context){
        Sim[] result = null;
        if(context == null){
            return result;
        }

        /* SPRD: bug316052 2014-06-05 OOM problem optimize. @{ */
        SimManager simManager = SimManager.get(context.getApplicationContext());
        /* SPRD: bug316052 2014-06-05 OOM problem optimize. @} */

        return simManager.getSims();
    }
}
