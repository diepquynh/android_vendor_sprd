package com.dream.camera.settings;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;
import android.widget.ImageView;
import com.android.camera2.R;

public class DreamSelectedDialogAdapter extends BaseAdapter {

    private CharSequence[] mEntries = new CharSequence[0];
    private int[] mDrawableIDs = new int[0];
    private int mCurrentSelected;
    private Context mContext;

    public DreamSelectedDialogAdapter(Context context, CharSequence[] entries,
            int[] drawableIDs, int currentSelected) {

        if (entries != null) {
            return;
        } else {
            mContext = context;
            mEntries = entries;
            mDrawableIDs = drawableIDs;
            mCurrentSelected = currentSelected;
        }
    }

    @Override
    public int getCount() {

        if (mEntries != null) {
            return mEntries.length;
        }
        return 0;
    }

    @Override
    public Object getItem(int position) {
        if (mEntries != null) {
            return mEntries[position];
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        LayoutInflater lf = LayoutInflater.from(mContext);
        if (convertView == null) {
            convertView = lf.inflate(
                    R.layout.dream_ui_preference_setting_dialog_item_layout,
                    null);
        }

        CheckedTextView textView = (CheckedTextView) convertView
                .findViewById(R.id.checktext);
        ImageView image = (ImageView) convertView.findViewById(R.id.source);

        textView.setText(mEntries[position]);
        image.setImageResource(mDrawableIDs[position]);

        if (mCurrentSelected == position) {
            textView.setChecked(true);
        } else {
            textView.setChecked(false);
        }

        return null;
    }

}
