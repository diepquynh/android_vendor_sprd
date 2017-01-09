/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.uphoto.preference;

import com.ucamera.uphoto.R;

import android.app.Dialog;
import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;

public class MyListPreference extends MyDialogPreference {
    private static final String TAG = "MyListPreference";
    private Context mContext = null;

    public MyListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        TypedArray a = context.obtainStyledAttributes(attrs,
                com.android.internal.R.styleable.ListPreference, 0, 0);
        mEntries = a.getTextArray(com.android.internal.R.styleable.ListPreference_entries);
        mEntryValues = a.getTextArray(com.android.internal.R.styleable.ListPreference_entryValues);
        a.recycle();
    }

    public MyListPreference(Context context) {
        this(context, null);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        CharSequence[] entries  = getEntries();
        mClickedDialogEntryIndex = getValueIndex();
        mClickedDialogEntryIndex = mClickedDialogEntryIndex < 0 ? 0 : mClickedDialogEntryIndex;

        ListView listView = (ListView) view.findViewById(R.id.preference_settings_listview);
        listView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        Log.d(TAG, "onBindDialogView(): mClickedDialogEntryIndex = " + mClickedDialogEntryIndex);
        listView.setAdapter(new ArrayAdapter<CharSequence>(mContext, R.layout.simple_list_item_single_choice, entries));
        listView.setItemChecked(mClickedDialogEntryIndex, true);
        listView.setOnItemClickListener(new OnItemClickListener() {

            public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
                mClickedDialogEntryIndex = position;
                Dialog dialog = getDialog();
                if (dialog != null && dialog.isShowing()){
                    dialog.dismiss();
                }
            }
        });
    }

    @Override
    protected View onCreateDialogView() {
        super.onCreateDialogView();

        final LayoutInflater layoutInflater =
            (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        View view = getAlertDialog().getContentView();
        final ViewGroup dialogContentFrame = (ViewGroup)view.findViewById(R.id.custom_dialog_content);

        layoutInflater.inflate(R.layout.custom_alert_dialog_listview_content, dialogContentFrame);

        return view;
    }
}
