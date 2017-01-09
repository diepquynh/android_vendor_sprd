package com.android.email.activity.setup;

import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;
import com.android.email.R;
import com.android.mail.providers.UIProvider;
import com.android.mail.utils.Utils;

public class QuickResponsesCursorAdapter extends SimpleCursorAdapter{
    private int mLayout;
    private String mFrom;
    private int mTo;

    private final SharedPreferences statePrefs;
    private final SharedPreferences.Editor stateEditor;
    public static final String QUICK_RESPONSE_PREF_STATE = "respond_via_email_prefs_state";

    public QuickResponsesCursorAdapter(Context context, int layout, Cursor c, String[] from,
                                       int[] to, int flags) {
        super(context, layout, c, from, to, flags);
        // TODO Auto-generated constructor stub
        mLayout = layout;
        mFrom = from[0];
        mTo = to[0];
        statePrefs = context.getSharedPreferences(QUICK_RESPONSE_PREF_STATE,
                Context.MODE_PRIVATE);
        stateEditor = statePrefs.edit();
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
        // TODO Auto-generated method stub
        View view = LayoutInflater.from(context).inflate(mLayout, null);
        ViewHolder holder = new ViewHolder();
        holder.text = (TextView) view.findViewById(mTo);
        view.setTag(holder);
        return view;
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
        // TODO Auto-generated method stub
        ViewHolder holder = (ViewHolder) view.getTag();
        String[] defaultQuickResponses = context.getResources().getStringArray(
                R.array.default_quick_responses);
        String responseText = cursor.getString(cursor.getColumnIndex(mFrom));

        String state = statePrefs.getString(responseText,null);
        if (!TextUtils.isEmpty(state) && state.startsWith("unchanged")) {
            int position = Integer.parseInt(state.substring(9,state.length()));
            if (!responseText.equals(defaultQuickResponses[position])) {
                final ContentValues values = new ContentValues(1);
                values.put(mFrom, defaultQuickResponses[position]);
                final Uri uri = Utils.getValidUri(
                        cursor.getString(cursor.getColumnIndex(UIProvider.QuickResponseColumns.URI)));
                context.getContentResolver().update(uri, values, null, null);
                stateEditor.putString(defaultQuickResponses[position], "unchanged"+position);
                stateEditor.commit();
                stateEditor.remove(responseText);
                responseText = defaultQuickResponses[position];
            }
        }
        holder.text.setText(responseText);
    }

    class ViewHolder {
        TextView text;
    }
 }