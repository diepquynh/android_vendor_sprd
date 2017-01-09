/**
 * Created By Spreadst
 * */

package com.sprd.gallery3d.app;

import java.util.List;

import com.android.gallery3d.R;

import android.content.Context;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class DrawerListAdapter extends BaseAdapter {
    private Context mContext;
    LayoutInflater inflater;
    private int selectedPosition = -1;
    private List<String> mPlaneTitleList;
    private String[] planetTitles = new String[3];

    int[] imageSourceId = {
            R.drawable.new_all_videos, R.drawable.new_local_videos, R.drawable.new_filmed_videos,
            R.drawable.history_videos,R.drawable.new_otg_device};
    int[] imageSourceIdHighlight = {
            R.drawable.new_all_videos_highlight, R.drawable.new_local_videos_highlight, R.drawable.new_filmed_videos_highlight,
            R.drawable.new_history_videos_highlight,R.drawable.new_otg_device_highlight};

    public DrawerListAdapter(Context context, List<String>planeTitleList) {
        mContext = context;
        mPlaneTitleList = planeTitleList;
        inflater = LayoutInflater.from(mContext);
    }

    public void setPlaneList(List<String> planelist) {
        mPlaneTitleList = planelist;
    }

    @Override
    public int getCount() {
        // TODO Auto-generated method stub
        return mPlaneTitleList.size();
    }

    @Override
    public Object getItem(int position) {
        // TODO Auto-generated method stub
        return mPlaneTitleList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view;
        if (convertView == null) {
            view = inflater.inflate(R.layout.drawer_list_item, parent, false);
        } else {
            view = convertView;
        }
        ImageView imageView = (ImageView) view.findViewById(R.id.imageView);
        if (position >= 0 && position < 3) {
            imageView.setImageResource(imageSourceId[position]);
        } else if (position == mPlaneTitleList.size()-1) {
            imageView.setImageResource(imageSourceId[3]);
        } else {
            imageView.setImageResource(imageSourceId[4]);
        }
        TextView textView = (TextView) view.findViewById(R.id.textView);
        textView.setText(mPlaneTitleList.get(position));
        textView.setTextColor(R.color.video_list_text_color);
        LinearLayout lineHead = (LinearLayout) view.findViewById(R.id.ll_line_head);
        LinearLayout lineFoot = (LinearLayout) view.findViewById(R.id.ll_line_foot);

        if (position == 0) {
            lineFoot.setVisibility(View.VISIBLE);
            lineHead.setVisibility(View.GONE);
        } else if (mPlaneTitleList != null && position == mPlaneTitleList.size() - 1) {
            lineHead.setVisibility(View.VISIBLE);
            lineFoot.setVisibility(View.GONE);
        } else {
            lineHead.setVisibility(View.GONE);
            lineFoot.setVisibility(View.GONE);
        }
        if (position >= 0 && position < 3) {
            if (selectedPosition != -1 && selectedPosition == position) {
                imageView.setImageResource(imageSourceIdHighlight[position]);
                textView.setTextColor(mContext.getResources().getColor(
                        R.color.sprd_main_theme_color));
            } else {
                imageView.setImageResource(imageSourceId[position]);
                textView.setTextColor(Color.BLACK);
            }
        } else if (position == mPlaneTitleList.size() - 1) {
            if (selectedPosition == position) {
                imageView.setImageResource(imageSourceIdHighlight[3]);
                textView.setTextColor(mContext.getResources().getColor(
                        R.color.sprd_main_theme_color));
            } else {
                imageView.setImageResource(imageSourceId[3]);
                textView.setTextColor(Color.BLACK);
            }
        } else {
            if (selectedPosition == position) {
                imageView.setImageResource(imageSourceIdHighlight[4]);
                textView.setTextColor(mContext.getResources().getColor(
                        R.color.sprd_main_theme_color));
            } else {
                imageView.setImageResource(imageSourceId[4]);
                textView.setTextColor(Color.BLACK);
            }
        }
        return view;
    }

    public void setSelectedPosition(int position) {
        selectedPosition = position;
    }

}
