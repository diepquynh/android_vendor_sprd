package com.spreadst.validator;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

public class TestResultActivity extends Activity{
    private static final String TAG = "TestResultActivity";

    private TextView mTestResultView = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.test_result_view);

        mTestResultView = (TextView) findViewById(R.id.test_result_text);
        ListView listView = (ListView) findViewById(R.id.reslut_listview);
        ResultAdapter resultAdapter = new ResultAdapter(UIDialog.mListItem);
        listView.setAdapter(resultAdapter);
        Button reTestButton = (Button) findViewById(R.id.retest);
        reTestButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                Log.d(TAG, "reTestButton onClick");
                    Intent i = new Intent();
                    i.setClass(TestResultActivity.this, UIDialog.class);
                    i.putExtra(UIDialog.ACTION_ID, UIDialog.DIALOG_BEGIN);
                    i.putExtra(UIDialog.RE_TEST, true);
                    startActivity(i);
                    finish();
            }
        });

        Button endTestButton = (Button) findViewById(R.id.test_end);
        endTestButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                Log.d(TAG, "endTestButton onClick");
                    Intent i = new Intent();
                    i.setClass(TestResultActivity.this, UIDialog.class);
                    i.putExtra(UIDialog.ACTION_ID, UIDialog.DIALOG_END);
                    startActivity(i);
                    finish();
            }
        });
    }

    @Override
    public void onBackPressed() {
        Log.d(TAG, "onBackPressed");
        Intent i = new Intent();
        i.setClass(TestResultActivity.this, UIDialog.class);
        i.putExtra(UIDialog.ACTION_ID, UIDialog.DIALOG_END);
        startActivity(i);
        finish();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private class ResultAdapter extends BaseAdapter {

        private int[] mListView;

        public ResultAdapter(int[] list) {
            mListView = list;
        }
        @Override
        public int getCount() {
            return mListView.length;
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
            ViewHolder viewHolder = null;
            if (convertView == null) {
                convertView = getLayoutInflater().inflate(R.layout.item_result_view, null);
                viewHolder = new ViewHolder();
                viewHolder.textID = (TextView) convertView.findViewById(R.id.id_text);
                viewHolder.textCase = (TextView) convertView.findViewById(R.id.test_case);
                viewHolder.textResult = (TextView) convertView.findViewById(R.id.test_result);
                convertView.setTag(viewHolder);
            } else {
                viewHolder = (ViewHolder) convertView.getTag();
            }
            viewHolder.textID.setText(String.valueOf(position + 1));
            viewHolder.textCase.setText(getString(mListView[position]));
            if (UIDialog.mTestResult[position] == UIDialog.FAIL) {
                viewHolder.textResult.setText(getString(R.string.test_fail));
                viewHolder.textResult.setTextColor(Color.RED);
                mTestResultView.setText(getString(R.string.TestResultTitleStringFail));
                mTestResultView.setTextColor(Color.RED);
            } else if (UIDialog.mTestResult[position] == UIDialog.SUCCESS){
                viewHolder.textResult.setText(getString(R.string.test_pass));
                viewHolder.textResult.setTextColor(Color.GREEN);
            } else {
                viewHolder.textResult.setText(getString(R.string.test_not_result));
                viewHolder.textResult.setTextColor(Color.BLACK);
            }
            return convertView;
        }
    }

    private final class ViewHolder {
        private TextView textID;
        private TextView textCase;
        private TextView textResult;
    }
}
