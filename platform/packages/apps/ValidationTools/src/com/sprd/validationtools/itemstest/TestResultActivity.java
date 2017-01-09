
package com.sprd.validationtools.itemstest;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.Const;
import com.sprd.validationtools.TestItem;
import com.sprd.validationtools.sqlite.EngSqlite;
import com.sprd.validationtools.R;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import com.sprd.validationtools.IATUtils;

public class TestResultActivity extends Activity {
    private static final String TAG = "TestResultActivity";
    private Context mContext;
    public TextView mTestResultView;
    private EngSqlite mEngSqlite;
    private String mBit;
    private static final int READ_BIT = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.test_result);
        mContext = this;
        mEngSqlite = EngSqlite.getInstance(mContext);
        ListView listView = (ListView) findViewById(R.id.listview_layout);
        ListAdapter listAdapter = new ListAdapter(mContext, mEngSqlite.queryData(Const
                    .getSupportList(false, this)));
        listView.setAdapter(listAdapter);
        mTestResultView = (TextView) findViewById(R.id.test_result_text);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        int failCount = mEngSqlite.querySystemFailCount();
        if (failCount >= 1) {
            mTestResultView.setText(getResources().getString(R.string.TestResultTitleStringFail));
            mTestResultView.setTextColor(Color.RED);
        } else {
            setSuccess();
        }
        super.onResume();
    }

    public final class ViewHolder {
        public TextView textID;
        public TextView textCase;
        public TextView textResult;
    }

    private class ListAdapter extends BaseAdapter {
        private Context mContext;
        private LayoutInflater mLayoutInflater;
        private ViewHolder mViewHolder;
        private ArrayList<TestItem> mItems;

        public ListAdapter(Context context, ArrayList<TestItem> items) {
            mContext = context;
            mLayoutInflater = (LayoutInflater) mContext
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            mViewHolder = new ViewHolder();
            mItems = items;
        }

        @Override
        public int getCount() {
            return mItems.size();
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
            convertView = mLayoutInflater.inflate(R.layout.table_row_layout, parent, false);
            mViewHolder.textID = (TextView) convertView.findViewById(R.id.id_text);
            mViewHolder.textCase = (TextView) convertView.findViewById(R.id.test_case);
            mViewHolder.textResult = (TextView) convertView.findViewById(R.id.test_result);
            mViewHolder.textID.setText(String.valueOf(position + 1));
            mViewHolder.textCase.setText(mItems.get(position).getTestTitle());
            if (mItems.get(position).getResult() == Const.FAIL) {
                mViewHolder.textResult.setText(getResources().getString(R.string.text_fail));
                mViewHolder.textResult.setTextColor(Color.RED);
                mTestResultView.setText(getResources()
                        .getString(R.string.TestResultTitleStringFail));
                mTestResultView.setTextColor(Color.RED);
            } else if (mItems.get(position).getResult() == Const.SUCCESS) {
                mViewHolder.textResult.setText(getResources().getString(R.string.text_pass));
                mViewHolder.textResult.setTextColor(Color.GREEN);
            } else {
                mViewHolder.textResult.setText(getResources().getString(R.string.text_na));
                mViewHolder.textResult.setTextColor(Color.BLACK);
            }
            return convertView;
        }
    }

//    @Override
//    public void onBackPressed() {
//        finish();
//    }

    private void setSuccess(){
        new Thread(new Runnable(){
            public void run(){
                String str = IATUtils.sendATCmd("AT+SGMR=0,0,4","atchannel0");
                Log.d(TAG, "setSuccess get result str = " + str);
                if(str.contains(IATUtils.AT_OK)){
                    String[] paser = str.split("\n");
                    String[] paser1 = paser[0].split(":");
                    mBit = getBitStr(paser1[1].trim());
                    Log.d(TAG, "mBit: " + mBit);

                    long cc = Long.parseLong(mBit, 16) | Long.parseLong("08000000", 16);
                    str = IATUtils.sendATCmd("AT+SGMR=0,1,4,\"" +
                            get8Bit("" + Long.toHexString(cc)) + "\"","atchannel0");
                    Log.d(TAG, "setSuccess set result cc = "
                            + Long.toHexString(cc) + ", str = " + str);
                }
            }

            private String getBitStr(String str) {
                String result = null;
                int ind = str.indexOf("0x");
                result = str.substring(ind + 2);
                return result.trim();
            }

            private String get8Bit(String src) {
                return (src.length() < 8) ? "0" + src : src;
            }
        }).start();
    }
}
