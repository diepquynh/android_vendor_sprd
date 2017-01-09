
package com.sprd.validationtools.itemstest;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;
import java.util.ArrayList;

import com.sprd.validationtools.R;
import com.sprd.validationtools.Const;
import com.sprd.validationtools.TestItem;
import com.sprd.validationtools.sqlite.EngSqlite;
import com.sprd.validationtools.testinfo.TestInfoMainActivity;
import java.util.List;

public class ListItemTestActivity extends Activity {
    private ItemListViewAdapter mItemListViewAdapter;
    private Context mContext;
    private ArrayList<TestItem> mItemsListView;
    private ListView mListViewItem;
    private EngSqlite mEngSqlite;
    private int mLastTestItemIndex = 0;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mEngSqlite = EngSqlite.getInstance(this);
        mContext = this;
        setContentView(R.layout.activity_validation_tools_main);
        mListViewItem = (ListView) findViewById(R.id.ValidationToolsList);
        initAdapter();
        mListViewItem.setAdapter(mItemListViewAdapter);
        mListViewItem.setOnItemClickListener(new ListItemClickListener());
    }

    private void initAdapter() {
        TestItem item = null;
        EngSqlite engSqlite = EngSqlite.getInstance(ListItemTestActivity.this);
        if (engSqlite == null) {
            return;
        }
        mItemsListView = Const.getSupportList(true, this);

        for (int i = 0; i < mItemsListView.size(); i++) {
            item = mItemsListView.get(i);
            item.setResult(engSqlite.getTestListItemStatus(mItemsListView.get(i)
                    .getTestname()));
        }
        mItemListViewAdapter = new ItemListViewAdapter(this, mItemsListView);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (resultCode == Const.TEST_ITEM_DONE) {
            int position = mLastTestItemIndex;

            mItemsListView.get(position).setResult(
                    mEngSqlite.getTestListItemStatus(mItemsListView.get(position).getTestname()));
            mItemListViewAdapter.notifyDataSetChanged();
        }
    }

    private class ListItemClickListener implements OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

            mLastTestItemIndex = position;
            Intent intent = new Intent(ListItemTestActivity.this, mItemsListView.get(position)
                    .getTestClass());
            intent.putExtra(Const.INTENT_PARA_TEST_NAME, mItemsListView.get(position).getTestname());

            intent.putExtra(Const.INTENT_PARA_TEST_INDEX, position);
            startActivityForResult(intent, 0);
        }
    }

    private class ItemListViewAdapter extends BaseAdapter {

        private ArrayList<TestItem> mItemList;
        private LayoutInflater mInflater;
        private Context mContext;

        public ItemListViewAdapter(Context c, ArrayList<TestItem> mItemsListView) {
            mItemList = mItemsListView;
            mContext = c;
            mInflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        @Override
        public int getCount() {
            if (mItemList != null) {
                return mItemList.size();
            }
            return 0;
        }

        @Override
        public Object getItem(int position) {
            return position;
        }

        @Override
        public long getItemId(int position) {
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view = null;
            TestItem item = mItemList.get(position);
            if (convertView == null) {
                view = mInflater.inflate(R.layout.listview_item, parent, false);
            } else {
                view = convertView;
            }
            TextView textView = (TextView) view.findViewById(R.id.listitem_text);
            textView.setText(item.getTestTitle());

            if (item.getResult() == Const.SUCCESS) {
                textView.setTextColor(Color.GREEN);
            } else if (item.getResult() == Const.FAIL) {
                textView.setTextColor(Color.RED);
            } else {
                textView.setTextColor(Color.WHITE);
            }
            return view;
        }
    }
}
