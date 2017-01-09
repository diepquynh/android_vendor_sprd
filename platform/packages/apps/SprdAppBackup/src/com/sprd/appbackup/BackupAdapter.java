package com.sprd.appbackup;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.DataSetObservable;
import android.database.DataSetObserver;
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

import com.sprd.appbackup.service.Account;

public class BackupAdapter extends BaseExpandableListAdapter{

    private static final String TAG = "BackupAdapter";
    private static final boolean DBG = true;
    private List<AppInfo> mAppsInfo;
    private Context mContext;
    private List<DataItem>  mDataInfo;
    private List<String>  mGroupInfo;
    private String mStrVersion;
    private int mIappCount;
    private int mIdataCount;
    private int mIappCheckedCount;
    private int mIdataCheckedCount;
    private AlertDialog.Builder builder;
    /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed @{ */
    private Dialog dialog;

    public void dimissAlertDialog(){
        if(dialog != null){
            dialog.dismiss();
            dialog = null;
        }
    }
    /* @} */
    public BackupAdapter(Context context, List<String> groupDataInfo, List<AppInfo> appinfo, List<DataItem> dataInfo ){
        if(DBG) Log.d(TAG, "BackupAdapter Constructor()");
        mContext = context;
        mAppsInfo = new ArrayList<AppInfo>(appinfo);
        mDataInfo = new ArrayList<DataItem>();
        for(DataItem da:dataInfo){
            DataItem tmp = da.copy();
            mDataInfo.add(tmp);
        }
        mGroupInfo = groupDataInfo;
        if(DBG) Log.d(TAG, "AppsInfo = "+mAppsInfo.toString());
        if(DBG) Log.d(TAG, "DataInfo = "+mDataInfo.toString());
        mStrVersion = context.getResources().getString(R.string.version_name);
        mIappCount = mAppsInfo.size();

        /*for bug 388383,cmcc new req,need default select all*/
        mIappCheckedCount = mIappCount;
        /*
        //Fix bug 291712, do not checkAllAppData while first onResume.
        for (AppInfo info : mAppsInfo) {
            if (info.isChecked()) {
                mIappCheckedCount++;
            }
        }
        */
        updateIdataCount();
    }
    public void updateIdataCount(){
        mIdataCount = 0;
        mIdataCheckedCount = 0;
        for(DataItem data:mDataInfo){
            if(data.isEnabled()){
                mIdataCount++;
            }
            if(data.isEnabled() && data.isChecked()){
                mIdataCheckedCount ++;
            }
        }
    }

    @Override
    public boolean areAllItemsEnabled() {
        return true;
    }

    @Override
    public Object getChild(int groupPosition, int childPosition) {
        if(groupPosition == 0){
            return mAppsInfo.get(childPosition);
        }
        if(groupPosition == 1){
            return mDataInfo.get(childPosition);
        }
        return null;
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return childPosition;
    }

    @Override
    public View getChildView(int groupPosition, int childPosition,
            boolean isLastChild, View convertView, ViewGroup parent) {
        final boolean appGroup = (groupPosition == 0) ? true : false ;
        if(convertView == null){
            convertView =  LayoutInflater.from(mContext).inflate(R.layout.backup_list_child_item_layout, null);
        }
        TextView txName = (TextView) convertView.findViewById(R.id.txt_name);
        TextView txVersion = (TextView) convertView.findViewById(R.id.txt_version);
        ImageView imgIcon = (ImageView) convertView.findViewById(R.id.img_icon);
        CheckBox chkBox = (CheckBox) convertView.findViewById(R.id.chkbx_slected);
        if(groupPosition == 0){
            final AppInfo appInfo = mAppsInfo.get(childPosition);
            if(DBG) Log.d(TAG, "groupPosition = "+groupPosition+" childPosition ="+childPosition+" appInfo = "+appInfo.toString());
            txName.setText(appInfo.getName());
            txVersion.setText(mStrVersion + appInfo.getVersionName());
            txVersion.setVisibility(View.VISIBLE);
            imgIcon.setBackground(appInfo.getIcon());
            chkBox.setChecked(appInfo.isChecked());
            chkBox.setEnabled(true);
        }
        if(groupPosition == 1){
            final DataItem data = mDataInfo.get(childPosition);
            if(DBG) Log.d(TAG, "groupPosition = "+groupPosition+" childPosition ="+childPosition+" dataItem = "+data.toString());
            txName.setText(data.getCategoryName());
            txVersion.setVisibility(View.GONE);
            imgIcon.setBackground(data.getIcon());
            chkBox.setEnabled(data.isEnabled());
            if (data.isEnabled()) {
                chkBox.setChecked(data.isChecked());
            } else {
                chkBox.setChecked(false);
            }
        }
        final int target = childPosition;
        chkBox.setOnClickListener(new OnClickListener(){

            @Override
            public void onClick(View v) {
                if(appGroup){
                    if(((CheckBox)v).isChecked()){
                        mAppsInfo.get(target).setChecked(true);
                        mIappCheckedCount ++;
                    }else{
                        mAppsInfo.get(target).setChecked(false);
                        mIappCheckedCount --;
                    }
                }else{
                    if(((CheckBox)v).isChecked()){
                        List<Account> accounts = mDataInfo.get(target).getAccounts();
                        if(DBG) {Log.d(TAG, "List<Account> accounts = "+accounts);}
                        if(accounts != null && accounts.size() > 0){
                            final int count = accounts.size();
                            String[] accountNames = new String[count];
                            boolean[] accountChecked = new boolean[count];
                            for(int i = 0; i<count; i++){
                                accountNames[i] = accounts.get(i).getAccountName();
                                accountChecked[i] = accounts.get(i).isChecked();
                                if(DBG){Log.d(TAG, "accountNames" + accountNames[i]);}
                            }
                            /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed @{ */
                            builder = new AlertDialog.Builder(mContext);
                            dialog = builder.setTitle(R.string.select_account)
                            /* @} */
                            .setMultiChoiceItems(accountNames, accountChecked,
                                    new DialogInterface.OnMultiChoiceClickListener() {
                                        public void onClick(DialogInterface dialog, int whichButton,
                                                boolean isChecked) {
                                            /* SPRD: 453451 check mDataInfo and accounts @{ */
                                            if (mDataInfo != null && mDataInfo.size() > target
                                                && mDataInfo.get(target) != null
                                                && mDataInfo.get(target).getAccounts() != null) {

                                                int count = mDataInfo.get(target).getAccounts().size();
                                                if (whichButton < count) {
                                                    if (isChecked) {
                                                        mDataInfo.get(target).getAccounts()
                                                            .get(whichButton).setChecked(true);
                                                    } else {
                                                        mDataInfo.get(target).getAccounts()
                                                            .get(whichButton).setChecked(false);
                                                    }
                                                } else {
                                                    dialog.dismiss();
                                                }
                                            } else {
                                                dialog.dismiss();
                                            }
                                            /* @} */
                                        }
                                    })
                            .setPositiveButton(android.R.string.ok,
                                    new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    /* SPRD:455403 check mDataInfo and accounts @{ */
                                    if (mDataInfo == null || mDataInfo.size() <= target
                                            || mDataInfo.get(target) == null
                                            || mDataInfo.get(target).getAccounts() == null) {
                                        dialog.dismiss();
                                        return;
                                    }
                                    /* @} */

                                    List<Account> accounts = mDataInfo.get(target).getAccounts();
                                    int checkedAccount = 0;
                                    for(Account acc:accounts){
                                        if(acc.isChecked()){
                                            checkedAccount++;
                                        }
                                    }
                                    if(checkedAccount == 0){
                                        mDataInfo.get(target).setChecked(false);
                                        notifyDataSetChanged();
                                    }
                                    if(checkedAccount > 0){
                                        mDataInfo.get(target).setChecked(true);
                                        mIdataCheckedCount ++;
                                        notifyDataSetChanged();
                                    }
                                    dialog.dismiss();
                                    if(DBG) Log.d(TAG, "Accounts = "+mDataInfo.get(target).getAccounts());
                                }
                            /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed @{ */
                            }).create();
                            dialog.show();
                            /* @} */
                        }else{
                            mDataInfo.get(target).setChecked(true);
                            mIdataCheckedCount ++;
                        }
                    }else{
                        mDataInfo.get(target).setChecked(false);
                        mIdataCheckedCount --;
                    }
                }
                notifyDataSetChanged();
            }
        });

        return convertView;
    }

    @Override
    public int getChildrenCount(int groupPosition) {
        int cout = 0;
        if(groupPosition == 0){
            cout = mAppsInfo.size();
        }
        if(groupPosition == 1){
            cout = mDataInfo.size();
        }
        return cout;
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
        return mGroupInfo.get(groupPosition);
    }

    @Override
    public int getGroupCount() {
        return mGroupInfo.size();
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
        txtTitle.setText(mGroupInfo.get(groupPosition));
        if(groupPosition == 0){
            if(mIappCount == 0){
                chkBox.setEnabled(false);
                chkBox.setChecked(false);
            }else{
                chkBox.setEnabled(true);
                if(mIappCount == mIappCheckedCount){
                    chkBox.setChecked(true);
                }else{
                    chkBox.setChecked(false);
                }
            }
        }
        if(groupPosition == 1){
            if(mIdataCount == 0){
                chkBox.setEnabled(false);
                chkBox.setChecked(false);
            }else{
                chkBox.setEnabled(true);
                if(mIdataCount == mIdataCheckedCount){
                    chkBox.setChecked(true);
                }else{
                    chkBox.setChecked(false);
                }
            }
        }
        final int position = groupPosition;
        chkBox.setOnClickListener(new OnClickListener(){

            @Override
            public void onClick(View v) {
                boolean checked = ((CheckBox)v).isChecked();
                switch(position){
                case 0:
                    if(checked){
                        checkAllApp(true);
                        mIappCheckedCount = mIappCount;
                    }else{
                        checkAllApp(false);
                        mIappCheckedCount = 0;
                    }
                    break;
                case 1:
                    if(checked){
                        checkAllData(true);
                        mIdataCheckedCount = mIdataCount;
                    }else{
                        checkAllData(false);
                        mIdataCheckedCount = 0;
                    }
                    break;
                default:
                    break;
                }
               notifyDataSetChanged();
            }

        });
        return convertView;
    }

    @Override
    public boolean hasStableIds() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return false;
    }

    @Override
    public boolean isEmpty() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onGroupCollapsed(int groupPosition) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onGroupExpanded(int groupPosition) {
        // TODO Auto-generated method stub

    }

    public void checkAllData(boolean checked) {
        mIdataCheckedCount = 0;
        for(DataItem data : mDataInfo){
            if(data.isEnabled()){
                data.setChecked(checked);
                mIdataCheckedCount++;
                List<Account> la = data.getAccounts();
                if(la != null){
                    if(checked){
                        for(Account acc:la){
                            if(acc != null){
                                acc.setChecked(true);
                            }
                        }
                    }
                    data.setAccounts(la);
                }
            }
        }
    }

    public void checkAllApp(boolean checked) {
        mIappCheckedCount = 0;
        for(AppInfo data : mAppsInfo){
            data.setChecked(checked);
            mIappCheckedCount++;
        }
    }
    public List<DataItem> getDataInfo(){
        return mDataInfo;

    }

    public void setDataInfo(List<DataItem> dataInfo) {
        if(DBG){Log.d(TAG, "setDataInfo() dataInfo ="+dataInfo);}
        if(mDataInfo != null){
            mDataInfo.clear();
            mIdataCount = 0;
            for (DataItem da : dataInfo) {
                if (da.isEnabled()) {
                    mIdataCount++;
                }
                mDataInfo.add(da.copy());
            }
        }
        notifyDataSetChanged();
    }
    public List<AppInfo> getAppInfo(){
        if(DBG) Log.d(TAG, "getAppInfo =" +mAppsInfo);
        return mAppsInfo;
    }

    public void setAppsInfo(List<AppInfo> mAppsInfo) {
        this.mAppsInfo = mAppsInfo;
        if(mAppsInfo != null){
            mIappCount = mAppsInfo.size();
            mIappCheckedCount = 0;
            for(AppInfo ai:mAppsInfo){
                if(ai.isChecked()){
                    mIappCheckedCount++;
                }
            }
        }
    }

}
