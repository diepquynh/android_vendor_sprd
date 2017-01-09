package com.sprd.appbackup;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.TextView;

import com.sprd.appbackup.activities.MainActivity;
import com.sprd.appbackup.service.Account;

public class RestoreAdapter extends BaseExpandableListAdapter{
    
    private static final String TAG = "RestoreAdapter";
    private static final boolean DBG = true;
    private MainActivity mContext;
    private List<AppInfo> mAppInfos;
    private List<String> mGroupName;
    private Map<String, List<DataItem>> mDataDetail;
    private List<String> mDataFolderName;
    private final static int FLAG_APP = 0;
    private final static int FLAG_DATA = 1;
    private List<DataItem> mCheckedData;
    private String mCurrentStamp;
    private int mAppTotalCount;
    private int mAppCheckedCount;


    public RestoreAdapter(MainActivity context, List<String> groupName, List<AppInfo> appData, Map<String, List<DataItem>> dataDetail){
        mContext = context;
        mGroupName = groupName;
        mAppInfos = appData;
        mDataDetail = new HashMap<String, List<DataItem>>();
        for(String time: dataDetail.keySet()){
            if(dataDetail.get(time) != null){
                mDataDetail.put(time, dataDetail.get(time));
            }
        }
        mDataFolderName = new ArrayList<String>();
        for(String foldername : mDataDetail.keySet()){
            mDataFolderName.add(foldername);
        }
        sortFolder();
        mAppTotalCount = appData.size();
        /*for new cmcc require*/
        mAppCheckedCount = mAppTotalCount;
        if(DBG) Log.d(TAG, "RestoreAdapter mAppCheckedCount = " + mAppCheckedCount + " mAppInfos = " + mAppInfos);
    }
    private void sortFolder(){
        if (mDataFolderName != null && mDataFolderName.size() > 1){
            Collections.sort(mDataFolderName, new Comparator<String>(){

                @Override
                public int compare(String lhs, String rhs) {
                    String str1 = lhs;
                    String str2 = rhs;
                    if (lhs.contains("/")) {
                        str1 = lhs.substring(lhs.lastIndexOf("/") + 1);
                    }
                    if (rhs.contains("/")) {
                        str2 = rhs.substring(rhs.lastIndexOf("/") + 1);
                    }
                    return str2.compareTo(str1);
                }
            });
        }
    }
    public List<DataItem> getCurrentCheckedData(int groupPosition){
        int gp = (groupPosition - 2);
        if(gp >= 0 && gp < mDataFolderName.size()){
            mCurrentStamp = mDataFolderName.get(gp);
            mCheckedData = mDataDetail.get(mCurrentStamp);
        }
        return mCheckedData;
    }
    public List<DataItem> getCheckedData(){
        return mCheckedData;
    }
    public String getTimeStamp(){
        return mCurrentStamp;
    }
    public void clearCheckedData(){
        mCurrentStamp = null;
        mCheckedData = null;
    }
    public void setCheckedData(List<DataItem> checkdata){
        mCheckedData = checkdata;
    }
    public void setCheckedData(int groupPosition, int childPosition, boolean checked){
        /* SPRD: Bug250261,Set old item unChecked before we check a new dataItem. @{ */
        if(DBG) Log.d(TAG, "setCheckedData others  groupPosition=" + groupPosition + " childPosition = " + childPosition + " checked = " + checked);
        if (mDataDetail != null) {
            for (String timeStamp : mDataDetail.keySet()) {
                if (timeStamp != null && !timeStamp.equals(mDataFolderName.get(groupPosition - 2))) {
                    List<DataItem> checkedDataItems = mDataDetail.get(timeStamp);
                    if (checkedDataItems != null) {
                        for (DataItem item : checkedDataItems) {
                            if (item != null) {
                                item.setChecked(false);
                            }
                        }
                    }
                }
            }
        }
        /* @} */
        getCurrentCheckedData(groupPosition);
        if(childPosition < mCheckedData.size()){
            mCheckedData.get(childPosition).setChecked(checked);
        }
        if(groupPosition > 1 && groupPosition <mDataFolderName.size()+2){
            mDataDetail.put(mDataFolderName.get(groupPosition-2), mCheckedData);
        }
        notifyDataSetChanged();
    }

    /*for bug 388383,cmcc new req,need default select all*/
    public void setCheckedData(int groupPosition, boolean checked){
        List<DataItem> checkedData = getCurrentCheckedData(groupPosition);
        int childCount = getChildrenCount(groupPosition);
/*
        if (checkedData.get(0).isDefaultSelect()) {
            if(DBG) Log.d(TAG, "setCheckedData isDefaultSelect = true   groupPosition=" + groupPosition);
            checkedData.get(0).setDefaultSelect(false);
        } else {
            if(DBG) Log.d(TAG, "setCheckedData isDefaultSelect = false   groupPosition=" + groupPosition);
            return;
        }
*/
        /* SPRD: fix bug 411629 @{*/
        if (mDataDetail != null) {
            for (String timeStamp : mDataDetail.keySet()) {
                if (timeStamp != null && !timeStamp.equals(mDataFolderName.get(groupPosition - 2))) {
                    List<DataItem> checkedDataItems = mDataDetail.get(timeStamp);
                    if (checkedDataItems != null) {
                        for (DataItem item : checkedDataItems) {
                            if (item != null) {
                                item.setChecked(false);
                            }
                        }
                    }
                }
            }
        }
        /*@}*/
        if(DBG) Log.d(TAG, "setCheckedData select all  groupPosition=" + groupPosition + " childCount = " + childCount + " checked = " + checked);
        for (int i = 0; i < childCount; i++ ) {
            checkedData.get(i).setChecked(checked);
        }

        if(groupPosition > 1 && groupPosition < mDataFolderName.size() + 2){
            mDataDetail.put(mDataFolderName.get(groupPosition - 2), checkedData);
        }
        notifyDataSetChanged();
    }

    public void selectApk(int position, boolean checked){
        if(position < mAppInfos.size()){
            mAppInfos.get(position).setChecked(checked);
            int count = 0;
            for(AppInfo ai:mAppInfos){
                if(ai.isChecked())count++;
            }
            mAppCheckedCount = count;
            notifyDataSetChanged();
        }
    }

    public boolean isApkChecked(int position){
        if(position < mAppInfos.size()){
            return mAppInfos.get(position).isChecked();
        }
        return false;
    }
    @Override
    public boolean areAllItemsEnabled() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public Object getChild(int groupPosition, int childPosition) {
        if(groupPosition == FLAG_APP){
            return mAppInfos.get(childPosition);
        }
        if(groupPosition == FLAG_DATA){
            return 0;
        }
        if(groupPosition > FLAG_DATA){
            int positoin = groupPosition - 2;
            String time = mDataFolderName.get(positoin);
            List<DataItem> data = mDataDetail.get(time);
            return data.get(childPosition);
        }
        return null;
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return (groupPosition+1)*childPosition;
    }

    @Override
    public View getChildView(int groupPosition, int childPosition,
            boolean isLastChild, View convertView, ViewGroup parent) {
        final int target = childPosition;
        if(groupPosition == FLAG_APP){
            convertView =  LayoutInflater.from(mContext).inflate(R.layout.backup_list_child_item_layout, null);
            TextView txName = (TextView) convertView.findViewById(R.id.txt_name);
            TextView txVersion = (TextView) convertView.findViewById(R.id.txt_version);
            ImageView imgIcon = (ImageView) convertView.findViewById(R.id.img_icon);
            CheckBox chkBox = (CheckBox) convertView.findViewById(R.id.chkbx_slected);
            final AppInfo appInfo = mAppInfos.get(childPosition);
            if(DBG) Log.d(TAG, "Position =" + childPosition + " appInfo = " + appInfo.toString());
            txName.setText(appInfo.getName());
            txVersion.setText(mContext.getText(R.string.version_name).toString() + appInfo.getVersionName());
            txVersion.setVisibility(View.VISIBLE);
            imgIcon.setBackground(appInfo.getIcon());
            chkBox.setChecked(appInfo.isChecked());
            chkBox.setEnabled(true);

            chkBox.setOnCheckedChangeListener(new OnCheckedChangeListener(){

                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                        boolean isChecked) {
                    if(isChecked){
                        mAppInfos.get(target).setChecked(true);
                    }else{
                        mAppInfos.get(target).setChecked(false);
                    }
                    notifyDataSetChanged();
                }
            });
            convertView.setTag(R.id.txt_folder_name_item, groupPosition);
            convertView.setTag(R.id.chkbx_delete_slected, childPosition);
            return convertView;
        }
        if(groupPosition > FLAG_DATA){
            convertView =  LayoutInflater.from(mContext).inflate(R.layout.restore_list_grandson_item_layout, null);
            TextView txName = (TextView) convertView.findViewById(R.id.txt_name_restore);
            ImageView imgIcon = (ImageView) convertView.findViewById(R.id.img_icon_restore);
            CheckBox chkBox = (CheckBox) convertView.findViewById(R.id.chkbx_slected_restore);
            int positoin = groupPosition - 2;
            String time = mDataFolderName.get(positoin);
            List<DataItem> data = mDataDetail.get(time);
            final DataItem dataItem = data.get(childPosition);
            txName.setText(dataItem.getCategoryName());
            imgIcon.setBackground(dataItem.getIcon());
            chkBox.setChecked(dataItem.isChecked());
            chkBox.setEnabled(true);
            chkBox.setOnCheckedChangeListener(new OnCheckedChangeListener(){

                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                        boolean isChecked) {
                    if(isChecked){
                        dataItem.setChecked(true);
                    }else{
                        dataItem.setChecked(false);
                    }
                    notifyDataSetChanged();
                }
            });
            convertView.setTag(R.id.txt_folder_name_item, -1);
            convertView.setTag(R.id.chkbx_delete_slected, childPosition);
            return convertView;
        }
        return null;

    }

    @Override
    public int getChildrenCount(int groupPosition) {
        if(groupPosition == FLAG_APP){
            return mAppInfos.size();
        }
        if(groupPosition == FLAG_DATA){
            return 0;
        }
        if(groupPosition > 1){
            int positoin = groupPosition -2;
            String time = mDataFolderName.get(positoin);
            int count = mDataDetail.get(time).size();
            return count;
        }
        return 0;
    }

    @Override
    public long getCombinedChildId(long groupId, long childId) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public long getCombinedGroupId(long groupId) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public Object getGroup(int groupPosition) {
        return null;
    }

    @Override
    public int getGroupCount() {
        return mDataFolderName.size()+2;
    }

    @Override
    public long getGroupId(int groupPosition) {
        return groupPosition;
    }

    @Override
    public View getGroupView(int groupPosition, boolean isExpanded,
            View convertView, ViewGroup parent) {
        if(groupPosition<2){
            convertView = LayoutInflater.from(mContext).inflate(R.layout.backup_list_parent_item_layout, null);
            TextView txtTitle = (TextView) convertView.findViewById(R.id.txt_backup_title);
            CheckBox chkBox = (CheckBox) convertView.findViewById(R.id.chkbx_select_all_childitem);
            txtTitle.setText(mGroupName.get(groupPosition));
            chkBox.setVisibility(View.VISIBLE);
            if(groupPosition == FLAG_DATA){
                chkBox.setVisibility(View.INVISIBLE);
                mContext.setInvisibleIndicator(FLAG_DATA);
            }
            final boolean isAppGroup = ((groupPosition == FLAG_APP)?true:false);
            if(groupPosition == FLAG_APP){
                /*for bug 558745,never check mAppTotalCount when delete all appdata*/
                mAppTotalCount = mAppInfos.size();
                if(mAppTotalCount == 0){
                    chkBox.setEnabled(false);
                    chkBox.setChecked(false);
                }else{
                    chkBox.setEnabled(true);
                    if(mAppCheckedCount == mAppTotalCount){
                        chkBox.setChecked(true);
                    }else{
                        chkBox.setChecked(false);
                    }
                }
            }
            chkBox.setOnClickListener(new OnClickListener(){

                @Override
                public void onClick(View v) {
                    boolean checked = ((CheckBox)v).isChecked();
                    if(isAppGroup){
                        if(checked){
                            checkAllApp(true);
                            mAppCheckedCount = mAppTotalCount;
                        }else{
                            checkAllApp(false);
                            mAppCheckedCount = 0;
                        }
                        notifyDataSetChanged();
                    }
                }
            });
            convertView.setTag(R.id.txt_folder_name_item, groupPosition);
            convertView.setTag(R.id.chkbx_delete_slected, -1);
        }
        if(groupPosition >1){
            convertView = LayoutInflater.from(mContext).inflate(R.layout.restore_list_child_item_layout, null);
            TextView tv = (TextView) convertView.findViewById(R.id.txt_folder_name_item);
            CheckBox cb = (CheckBox) convertView.findViewById(R.id.chkbx_delete_slected);
            String timeStamp = "";
            String folderName = "";
            if((groupPosition-2) < mDataFolderName.size()){
                timeStamp = folderName = mDataFolderName.get(groupPosition-2);
                if (folderName.contains("/")) {
                    timeStamp = folderName.substring(folderName.lastIndexOf("/") + 1);
                }
            }
            tv.setText(timeStamp);
            cb.setVisibility(View.INVISIBLE);
            convertView.setPadding(30, 0, 0, 0);
            convertView.setTag(R.id.txt_folder_name_item, groupPosition);
            convertView.setTag(R.id.txt_folder_name, folderName);
        }
        return convertView;
    }
    /*for bug 388383,cmcc new req,need default select all*/
    public void checkAllApp(boolean checked){
        if(DBG) Log.d(TAG, "RestoreAdatper checkAllApp././. mAppInfos = " + mAppInfos);
        mAppTotalCount = 0;
        for(AppInfo ai: mAppInfos){
             if(DBG) Log.d(TAG, "RestoreAdatper checkAllApp mAppTotalCount = " + mAppTotalCount);
             ai.setChecked(checked);
             mAppTotalCount++;
        }
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

    @Override
    public boolean isEmpty() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onGroupCollapsed(int groupPosition) {
        notifyDataSetChanged();
    }

    @Override
    public void onGroupExpanded(int groupPosition) {
        notifyDataSetChanged();
    }

    public Map<String, List<DataItem>> getDataDetail() {
        return mDataDetail;
    }

    public void setDataDetail(Map<String, List<DataItem>> dataMap){
        if(mDataDetail != null){
            mDataDetail.clear();
        }else{
            mDataDetail = new HashMap<String, List<DataItem>>();
        }
        for(String time: dataMap.keySet()){
            if(dataMap.get(time) != null){
                mDataDetail.put(time, dataMap.get(time));
            }
        }
        if(mDataFolderName != null){
            mDataFolderName.clear();
        }else{
            mDataFolderName = new ArrayList<String>();
        }
        for(String foldername : mDataDetail.keySet()){
            mDataFolderName.add(foldername);
        }
        sortFolder();
        notifyDataSetChanged();
    }
    public List<AppInfo> getAppInfos() {
        return mAppInfos;
    }

    public void setAppInfos(List<AppInfo> mAppInfos) {
        mAppTotalCount = mAppInfos.size();
        mAppCheckedCount = mAppTotalCount;
        this.mAppInfos = mAppInfos;
        if(DBG) Log.d(TAG, "setAppInfos mAppTotalCount  = " + mAppTotalCount + " mAppInfos = " + mAppInfos);
    }

    public List<String> getDataFolderName() {
        return mDataFolderName;
    }

    public int getRestoreCount() {
        if (mDataFolderName != null && mAppInfos != null) {
            if(DBG) Log.d(TAG, "restore getRestoreCount  = " + (mAppInfos.size() + mDataFolderName.size() + 2));
            return mAppInfos.size() + mDataFolderName.size();
        } else {
            return -1;
        }
    }

}
