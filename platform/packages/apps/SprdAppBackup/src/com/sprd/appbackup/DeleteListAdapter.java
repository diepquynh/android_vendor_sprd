package com.sprd.appbackup;

import java.util.ArrayList;
import java.util.List;
import java.util.Collections;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.TextView;

public class DeleteListAdapter extends BaseExpandableListAdapter{

    private List<AppInfo> mApp;
    private List<Bundle> mData;
    private List<String> mHeader;
    private Context mContext;
    public static final String KEY_NAME = "name";
    public static final String KEY_CHECK_STATUS = "checked";

    public DeleteListAdapter(Context context,List<String> header, List<AppInfo> app, List<String> folder){
        mContext = context;
        mHeader = header;
        mApp = app;
        mData = new ArrayList<Bundle>();
        if (null == mHeader) {
            mHeader = new ArrayList<String>();
        }
        if (null != mApp) {
            /* SPRD: 447966 remove all of null element @{ */
            mApp.removeAll(Collections.singleton(null));
            /* @} */
            for(AppInfo ai:mApp){
                ai.setChecked(false);
            }
        } else {
            mApp = new ArrayList<AppInfo>();
        }
        if (null != folder) {
            /* SPRD: 447966 remove all of null element @{ */
            folder.removeAll(Collections.singleton(null));
            /* @} */
            Bundle bundle;
            for(String f:folder){
                bundle = new Bundle();
                bundle.putString(KEY_NAME, f);
                bundle.putBoolean(KEY_CHECK_STATUS, false);
                mData.add(bundle);
            }
        }
    }

    public void upadteAppList(List<AppInfo> app) {

        if (null != app) {
            /* SPRD: 447966 remove all of null element @{ */
            app.removeAll(Collections.singleton(null));
            /* @} */
            if (null == mApp) {
                mApp = new ArrayList<AppInfo>(app);
                return;
            }
            ArrayList<AppInfo> oldAppList = new ArrayList<AppInfo>(mApp);
            for (AppInfo newApp: app) {
                newApp.setChecked(false);
                for (AppInfo oldApp : oldAppList) {
                    if (newApp.equals(oldApp)) {
                        newApp.setChecked(oldApp.isChecked());
                        break;
                    }
                }

            }
            this.mApp = app;
        }
    }

    public void updateFolderList(List<String> folderList) {
        if (null == folderList) {
            return;
        }

        /* SPRD: 447966 remove all of null element @{ */
        folderList.removeAll(Collections.singleton(null));
        /* @} */
        if (null == mData) {
            mData = new ArrayList<Bundle>();
            Bundle bundle;
            for(String f : folderList){
                bundle = new Bundle();
                bundle.putString(KEY_NAME, f);
                bundle.putBoolean(KEY_CHECK_STATUS, false);
                mData.add(bundle);
            }
            return;
        }
        ArrayList<Bundle> newDataList = new ArrayList<Bundle>();
        Bundle bundle;
        for(String f: folderList){
            bundle = new Bundle();
            bundle.putString(KEY_NAME, f);
            bundle.putBoolean(KEY_CHECK_STATUS, false);
            newDataList.add(bundle);
        }
        ArrayList<Bundle> oldDataList = new ArrayList<Bundle>(mData);
        for (Bundle oldBundle: oldDataList) {
            String oldFolderName = oldBundle.getString(KEY_NAME);
            if (oldFolderName != null) {
                for (Bundle newBundle : newDataList) {
                    if (oldFolderName.equals(newBundle.getString(KEY_NAME))) {
                        newBundle.putBoolean(KEY_CHECK_STATUS, oldBundle.getBoolean(KEY_CHECK_STATUS));
                        break;
                    }
                }
            }
        }
        mData.clear();
        mData = newDataList;
    }

    public void updateHeader(List<String> header) {
        if (null != header) {
            mHeader = header;
        }
    }
    public void checkAll(boolean check){
        for(AppInfo ai:mApp){
            ai.setChecked(check);
        }
        for(Bundle bu:mData){
            bu.putBoolean(KEY_CHECK_STATUS, check);
        }
        notifyDataSetChanged();
    }
    public boolean getStatus(int groupPosition, int childPosition){
        boolean result = false;
        if(groupPosition == 0){
            result = mApp.get(childPosition).isChecked();
        }
        if(groupPosition == 1){
            result = mData.get(childPosition).getBoolean(KEY_CHECK_STATUS);
        }
        return result;
    }

    public int getCheckedSize(){
        int count = 0;
        ArrayList<AppInfo> appList = null;
        ArrayList<Bundle> dataList = null;
        if (mApp != null) {
            appList = new ArrayList<AppInfo>(mApp);
        }
        if (mData != null) {
            dataList = new ArrayList<Bundle>(mData);
        }
        if (appList != null) {
            for(AppInfo ai : appList){
                if(ai.isChecked())
                    count++;
            }
        }
        if (dataList != null) {
            for(Bundle bu : dataList){
                if (bu != null) {
                    if(bu.getBoolean(KEY_CHECK_STATUS))
                        count++;
                }
            }
        }
        return count;
    }
    public void setStatus(int groupPosition, int childPosition, boolean check){
        if(groupPosition == 0){
            mApp.get(childPosition).setChecked(check);
        }
        if(groupPosition == 1){
            mData.get(childPosition).putBoolean(KEY_CHECK_STATUS, check);
        }
        notifyDataSetChanged();
    }
    @Override
    public Object getChild(int groupPosition, int childPosition) {
        if(groupPosition == 0){
            return mApp.get(childPosition);
        }
        if(groupPosition == 1){
            return mData.get(childPosition);
        }
        return null;
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return (groupPosition+1)*(childPosition+1);
    }

    @Override
    public View getChildView(int groupPosition, int childPosition,
            boolean isLastChild, View convertView, ViewGroup parent) {
        final int target = childPosition;
     if(groupPosition == 0){
         convertView =  LayoutInflater.from(mContext).inflate(R.layout.backup_list_child_item_layout, null);
         TextView txName = (TextView) convertView.findViewById(R.id.txt_name);
         TextView txVersion = (TextView) convertView.findViewById(R.id.txt_version);
         ImageView imgIcon = (ImageView) convertView.findViewById(R.id.img_icon);
         CheckBox chkBox = (CheckBox) convertView.findViewById(R.id.chkbx_slected);
         final AppInfo appInfo = mApp.get(childPosition);
         txName.setText(appInfo.getName());
         txVersion.setText(mContext.getText(R.string.version_name).toString() + appInfo.getVersionName());
         txVersion.setVisibility(View.VISIBLE);
         imgIcon.setBackground(appInfo.getIcon());
         chkBox.setChecked(appInfo.isChecked());
         chkBox.setVisibility(View.VISIBLE);
         chkBox.setEnabled(true);

         chkBox.setOnCheckedChangeListener(new OnCheckedChangeListener(){

             @Override
             public void onCheckedChanged(CompoundButton buttonView,
                     boolean isChecked) {
                 if(isChecked){
                     mApp.get(target).setChecked(true);
                 }else{
                     mApp.get(target).setChecked(false);
                 }
                 notifyDataSetChanged();
             }
         });
         return convertView;
      }
      if(groupPosition == 1){
          convertView = LayoutInflater.from(mContext).inflate(R.layout.restore_list_child_item_layout, null);
          TextView tv = (TextView) convertView.findViewById(R.id.txt_folder_name_item);
          CheckBox cb = (CheckBox) convertView.findViewById(R.id.chkbx_delete_slected);
          int size = mData.size();

          if (size <= childPosition) {
             childPosition = size - 1;
          }

          /* SPRD： 447966 check childPosition @{ */
          if (childPosition < 0) {
              return convertView;
          }
          /* @} */

          String timeStamp = mData.get(childPosition).getString(KEY_NAME);
          if (timeStamp.contains("/")) {
              timeStamp = timeStamp.substring(timeStamp.lastIndexOf("/") + 1);
          }
          tv.setText(timeStamp);
          cb.setChecked(mData.get(childPosition).getBoolean(KEY_CHECK_STATUS));
          cb.setVisibility(View.VISIBLE);
          cb.setEnabled(true);
          cb.setOnCheckedChangeListener(new OnCheckedChangeListener(){
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                    boolean isChecked) {
                // TODO Auto-generated method stub
                if(isChecked){
                    mData.get(target).putBoolean(KEY_CHECK_STATUS, true);
                }else{
                    mData.get(target).putBoolean(KEY_CHECK_STATUS, false);
                }
                notifyDataSetChanged();
            }
          });
          return convertView;
      }
        return null;
    }

    @Override
    public int getChildrenCount(int groupPosition) {
        int count = 0;
        if(groupPosition == 0){
            if (mApp != null) {
                /* SPRD: 447966 remove all of null element @{ */
                mApp.removeAll(Collections.singleton(null));
                /* @} */
                count = mApp.size();
            }
        }
        if(groupPosition == 1){
            if (mData != null) {
                /* SPRD: 447966 remove all of null element @{ */
                mData.removeAll(Collections.singleton(null));
                /* @} */
                count = mData.size();
            }
        }
        return count;
    }

    @Override
    public Object getGroup(int groupPosition) {
        return mHeader.get(groupPosition);
    }

    @Override
    public int getGroupCount() {
        return mHeader.size();
    }

    @Override
    public long getGroupId(int groupPosition) {
        return groupPosition;
    }

    @Override
    public View getGroupView(int groupPosition, boolean isExpanded,
            View convertView, ViewGroup parent) {
        if (null == convertView) {
            convertView = LayoutInflater.from(mContext).inflate(R.layout.backup_list_parent_item_layout, null);
        }
        TextView txtTitle = (TextView) convertView.findViewById(R.id.txt_backup_title);
        CheckBox chkBox = (CheckBox) convertView.findViewById(R.id.chkbx_select_all_childitem);
        txtTitle.setText(mHeader.get(groupPosition));
        chkBox.setVisibility(View.INVISIBLE);
        return convertView;
    }

    @Override
    public boolean hasStableIds() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return true;
    }

    public List<AppInfo> getApp() {
        return mApp;
    }

    public List<Bundle> getData() {
        return mData;
    }

}
