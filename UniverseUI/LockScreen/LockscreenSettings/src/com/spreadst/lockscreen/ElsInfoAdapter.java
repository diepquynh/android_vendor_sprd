/** Create by Spreadst */

package com.spreadst.lockscreen;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

public class ElsInfoAdapter extends BaseAdapter {

    private Context mContext;

    private ExpandLockscreenInfo[] mElsinfos;

    private int mSelected_id = -1;

    public void setSelected(int selected) {
        this.mSelected_id = selected;
    }

    public ElsInfoAdapter(Context context, ExpandLockscreenInfo[] elsinfos,
            int selected) {

        mContext = context;
        mElsinfos = elsinfos;
        mSelected_id = selected;
    }

    @Override
    public int getCount() {
        return mElsinfos.length;
    }

    @Override
    public Object getItem(int position) {
        return mElsinfos[position];
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ThumbView thumbView;
        if (convertView == null) {
            // if it's not recycled, initialize some
            // attributes
            thumbView = new ThumbView(mContext);
            thumbView.setPadding(0, 0, 0, 0);
        } else {
            thumbView = (ThumbView) convertView;
        }
        int preview_id = mContext.getResources().getIdentifier(
                Constants.CURRENT_PACKAGE_NAME + ":drawable/"
                        + mElsinfos[position].getPreview_id(), null, null);
        int name_string_id = mContext.getResources().getIdentifier(
                Constants.CURRENT_PACKAGE_NAME + ":string/"
                        + mElsinfos[position].getName_string_id(), null, null);

        thumbView.getImageView().setImageResource(preview_id);
        thumbView.getTextView().setText(name_string_id);
        if (mElsinfos[position].getId() == mSelected_id) {
            thumbView.getSelectImageView().setVisibility(View.VISIBLE);
        } else {
            thumbView.getSelectImageView().setVisibility(View.GONE);
        }
        return thumbView;
    }
}
