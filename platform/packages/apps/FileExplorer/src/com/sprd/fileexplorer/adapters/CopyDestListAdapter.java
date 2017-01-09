package com.sprd.fileexplorer.adapters;

import java.util.ArrayList;

import android.os.UserHandle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.util.ActivityUtils;

public class CopyDestListAdapter extends BaseAdapter {
    /* SPRD 454659  @{ */
    private ArrayList<Integer> mImagelist = new ArrayList<Integer>();
    private ArrayList<Integer> mTitleList = new ArrayList<Integer>();

    public CopyDestListAdapter(int flag){
        if((flag & ActivityUtils.INTERNAL_STORAGE_AVAILABLE) !=0 ){
            mImagelist.add(R.drawable.main_device);
            mTitleList.add(R.string.title_section_internal);
        }

        // SPRD: bug571788 hide SD/USB dest in New User mode
        if (UserHandle.myUserId() == UserHandle.USER_OWNER) {
            if ((flag & ActivityUtils.EXTERNAL_STORAGE_AVAILABLE) != 0) {
                mImagelist.add(R.drawable.main_sd);
                mTitleList.add(R.string.title_section_external);
            }
            if ((flag & ActivityUtils.USB_STORAGE_AVAILABLE) != 0) {
                mImagelist.add(R.drawable.main_sd);
                mTitleList.add(R.string.title_section_usbdisk);
            }
        }
    }
    /* @} */
    public class ViewHolder {
        ImageView fileImg;
        TextView fileName;
    }
    //SPRD 454659
    //private int[] imgIds = { R.drawable.main_device, R.drawable.main_sd };

    @Override
    public int getCount() {
        /* SPRD 454659  @{ */
        //return imgIds.length;
        return mImagelist.size();
        /* @} */
    }

    @Override
    public Object getItem(int arg0) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public long getItemId(int position) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder vHolder = null;
        if (convertView == null) {
            vHolder = new ViewHolder();
            convertView = LayoutInflater.from(parent.getContext()).inflate(
                    R.layout.copydest_listitem, null);
            vHolder.fileImg = (ImageView) convertView
                    .findViewById(R.id.file_item_list_img);
            vHolder.fileName = (TextView) convertView
                    .findViewById(R.id.file_item_list_name);
            /* SPRD 454659  @{ */
            //vHolder.fileName.setText(parent.getResources().getStringArray(
            //        R.array.select_dest)[position]);
            //vHolder.fileImg.setImageResource(imgIds[position]);
            vHolder.fileName.setText(parent.getResources().getString(mTitleList.get(position)));
            vHolder.fileImg.setImageResource(mImagelist.get(position));
            /* @} */
            convertView.setTag(vHolder);
        } else {
            vHolder = (ViewHolder) convertView.getTag();
        }
        return convertView;
    }
}
