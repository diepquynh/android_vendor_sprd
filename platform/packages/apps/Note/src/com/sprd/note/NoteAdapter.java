
package com.sprd.note;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;
import java.util.Date;
import android.util.Log;
import android.text.format.DateFormat;

import com.sprd.note.data.NoteDataManagerImpl;
import com.sprd.note.data.NoteItem;

public class NoteAdapter extends BaseAdapter {
    private static final String TAG = "NoteAdapter";

    public static final int SHOW_TYPE_NORMAL = 0;
    public static final int SHOW_TYPE_DELETE = 1;
    public static final int SHOW_TYPE_MOVETOFOLDER = 2;
    
    public static final int CHECKED_NONE = 0;
    public static final int CHECKED_SOME = 1;
    public static final int CHECKED_ALL = 2;
    
    private LayoutInflater mListContainer;
    private Context mcontext;
    // char size of display note content
    public int MAX_NUM = 16;

    private int mShowType;

    private List<NoteItem> mItems = null;
    private String mTodayDate;

    public NoteAdapter(Context context, List<NoteItem> items) {
        mListContainer = LayoutInflater.from(context); // create layoutInfalater and context
        this.mcontext = context;
        this.mItems = items;
//        SPRD bug 601287
//        setAllItemChecked(false);
        mShowType = SHOW_TYPE_NORMAL;
        java.text.DateFormat dateFormat = DateFormat.getDateFormat(mcontext);
        mTodayDate = dateFormat.format(new java.util.Date(new Date().getTime()));
    }

    private void setAllItemChecked(boolean isChecked) {
        for (int i = 0; i < getCount(); i++) {
            mItems.get(i).isSelected = isChecked;
        }
    }

    public void setAllItemCheckedAndNotify(boolean isChecked) {
        setAllItemChecked(isChecked);
        notifyDataSetChanged();
    }

    public void toggleChecked(int position) {
        boolean isSelected = !getItem(position).isSelected;
        getItem(position).isSelected = isSelected;
    }

    public void setItemChecked(int position, boolean checked) {
        getItem(position).isSelected = checked;

    }

    public int getSelectedCount() {
        int count = 0;
        for (NoteItem item : mItems) {// NoteItem item : mItems
            if (item.isSelected) {
                count++;
            }
        }
        return count;
    }

    public int getCheckedState() {
        int checkedCount = getSelectedCount();
        if (checkedCount == mItems.size()) {
            return CHECKED_ALL;
        } else if (checkedCount > 0) {
            return CHECKED_SOME;
        } else {
            return CHECKED_NONE;
        }
    }

    public void setShowType(int type) {
        mShowType = type;
        notifyDataSetChanged();
    }

    public int getShowType() {
        return mShowType;
    }

    @Override
    public int getCount() {
        return mItems.size();
    }

    public int getFolderCount() {
        int count = 0;
        for (NoteItem item : mItems) {
            if (item.isFileFolder) {
                count++;
            }
        }
        return count;
    }

    public int getNotesCount() {
        int count = 0;
        for (NoteItem item : mItems) {
            if (!item.isFileFolder) {
                count++;
            }
        }
        return count;
    }

    @Override
    public NoteItem getItem(int position) {
        return mItems.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setListItems(List<NoteItem> items) {
        this.mItems = items;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ListItemView listItemView = null;
        if (convertView == null) {
            listItemView = new ListItemView();
            convertView = mListContainer.inflate(R.layout.listview_item_layout, null);
//            listItemView.mLeftIconView = (ImageView) convertView.findViewById(R.id.icon);
            listItemView.mTitleView = (TextView) convertView.findViewById(R.id.note_title);
            listItemView.mTimeView = (TextView) convertView.findViewById(R.id.note_time);
            listItemView.mCheckView = (CheckBox) convertView.findViewById(R.id.check);
            convertView.setTag(listItemView);
        } else {
            listItemView = (ListItemView) convertView.getTag();
        }
        NoteItem noteItem = mItems.get(position);

        listItemView.mCheckView.setChecked(noteItem.isSelected);
        if (mShowType == SHOW_TYPE_NORMAL) {
            listItemView.mTimeView.setVisibility(View.VISIBLE);
            listItemView.mCheckView.setVisibility(View.GONE);
        } else if (mShowType == SHOW_TYPE_DELETE | mShowType == SHOW_TYPE_MOVETOFOLDER) {
            listItemView.mTimeView.setVisibility(View.INVISIBLE);
            listItemView.mCheckView.setVisibility(View.VISIBLE);
        }
        if (!noteItem.isFileFolder) {
//            listItemView.mLeftIconView.setBackgroundResource(R.drawable.bullet_sprd);
            listItemView.mTitleView.setText(noteItem.getShowTitle());
        } else { // is folder
//            listItemView.mLeftIconView.setBackgroundResource(R.drawable.bullet_group_sprd);
            int count = NoteDataManagerImpl.getNoteDataManger(mcontext)
                    .getNotesFromFolder(noteItem._id).size();
            listItemView.mTitleView.setText(noteItem.getShowTitle() + "(" + count + ")");
        }
        if (noteItem.getDate(mcontext).equals(mTodayDate)) {
            listItemView.mTimeView.setText(noteItem.getTime(mcontext));
        } else {
            listItemView.mTimeView.setText(noteItem.getDate(mcontext));
        }

        return convertView;
    }
}
