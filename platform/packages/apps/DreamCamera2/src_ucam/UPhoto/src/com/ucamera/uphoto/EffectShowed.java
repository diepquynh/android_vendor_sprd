/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.widget.AdapterView;
import android.widget.RelativeLayout;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.os.Handler;
import android.os.Message;
import java.util.ArrayList;

import com.ucamera.ucam.compatible.Models;
//import com.ucamera.ucam.utils.UiUtils;bug 414691
import com.ucamera.uphoto.UiUtils;
import com.ucamera.uphoto.EffectTypeResource.EffectItem;
import com.ucamera.uphoto.R;

public class EffectShowed implements OnItemClickListener,EffectShowedAdapter.OnStateChangedListener{
    private final static int DEFALUE_LIKED_SIZE=10;
    private final static float ITEM_PRECENT_SCREEN = 132.0f/480.0f;
    private Activity mActivity;
    private Handler mHandler;
    private SharedPreferences mSp;
    private ViewGroup layout;
    private EffectTypeResource mResource;
    private View closeBtn;
    private View rightBtn;
    private GridView effectGridV;
    private EffectShowedAdapter mAdapter;
    private ArrayList<EffectItem> itemsList;
    private ArrayList<EffectItem> originalItemsList;
    private int operatorType;
    private int mSelctedNum;
    private int deleteCount;
    private int likedCount;
    /*
     * BUG FIX: 4751
     * DATE: 2013-08-28
     */
    public EffectShowed(Activity activity, Handler handler, int resType) {
        mActivity = activity;
        mHandler = handler;
        mResource = EffectTypeResource.getResInstance(resType);
        operatorType=-1;
        itemsList = mResource.getEffectShowedList();
        originalItemsList = new ArrayList<EffectItem>();

        layout = (ViewGroup)activity.findViewById(R.id.effect_showed_layout);
        closeBtn = layout.findViewById(R.id.effect_showed_close_btn);
        closeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                hidShowedEffectsImpl();
            }
        });
        rightBtn = layout.findViewById(R.id.effect_showed_right_btn);
        rightBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if(mSelctedNum > likedCount || deleteCount > 0 || operatorType == ImageEditConstants.EXCHANGE_LIKED_EFFECT){
                    int originalLikedCount = likedCount;

                    updateLikedItems();

                    Message msg = Message.obtain(mHandler,ImageEditConstants.CHANGE_LIKED_EFFECT);
                    msg.sendToTarget();

                    hidShowedEffects();

                    saveItemsInThread(originalLikedCount);// in sub thread , must be called at last time

                    initSelectedItems ();

                    operatorType=-1;
                }else{
                    hidShowedEffectsImpl();
                }
            }
        });
        if(Models.AMAZON_KFTT.equals(Models.getModel())){
            RelativeLayout.LayoutParams rightBtnParams=(RelativeLayout.LayoutParams)rightBtn.getLayoutParams();
            rightBtnParams.rightMargin = 30;
            rightBtn.setLayoutParams(rightBtnParams);
            RelativeLayout.LayoutParams closeBtnParams=(RelativeLayout.LayoutParams)closeBtn.getLayoutParams();
            closeBtnParams.leftMargin = 30;
            closeBtn.setLayoutParams(closeBtnParams);
        }
        initEffectGridView(mActivity, layout);

        initSelectedItems();
    }

    /*
     * FIX BUG: 5080
     * DATE: 2013-11-07
     */
    public void onDestroy() {
        if(mSelctedNum != likedCount || deleteCount > 0 || operatorType == ImageEditConstants.EXCHANGE_LIKED_EFFECT) {
            resetOriginalItemsList();
            itemsList.clear();
            itemsList.addAll(0, originalItemsList);
            operatorType=-1;
            if(null != mAdapter){
                mAdapter.notifyDataSetChanged();
            }
        }
    }

    public void showEffects() {
        showEffects(null);
    }

    public void showEffects(Animation anim) {
        layout.setVisibility(View.VISIBLE);
        if(null != anim) {
            layout.startAnimation(anim);
        }

        addOriginalItemsList();
    }

    public void hidShowedEffects() {
        layout.setVisibility(View.INVISIBLE);
    }

    public boolean isShowed() {
        return layout.isShown();
    }

    public boolean onKeyEvent(int keyCode, KeyEvent event){
        if(keyCode==KeyEvent.KEYCODE_BACK && isShowed()){
            hidShowedEffectsImpl();
            return true;
        }
        return false;
    }

    private void addOriginalItemsList(){
        originalItemsList.clear();
        originalItemsList.addAll(0, itemsList);
        operatorType=-1;
    }

    private void initEffectGridView(Context context, ViewGroup parentLayout) {
        int numColumms = 3;
        effectGridV = (GridView)parentLayout.findViewById(R.id.effect_showed_gridview_id);
        if(Models.AMAZON_KFTT.equals(Models.getModel())){
            numColumms = 4;
            effectGridV.setNumColumns(numColumms);
        }
        /*
         *BUG FIX: 5321
         *FIX COMMENT: horizontal space not same of the effect items
         *DATE: 2013-11-28
         */
        float itemWidth = ITEM_PRECENT_SCREEN * UiUtils.screenWidth();
        float space =(UiUtils.screenWidth() - numColumms * itemWidth) / (numColumms +1);
        mAdapter = new EffectShowedAdapter(context, itemsList);
        mAdapter.setItemSize(itemWidth, itemWidth);
        mAdapter.setItemMargins(space * 0.5f, 0);
        mAdapter.setNumColumns(numColumms);
        mAdapter.setOnStateChangedListener(this);
        effectGridV.setAdapter(mAdapter);
        effectGridV.setOnItemClickListener(this);
    }

    private void initSelectedItems() {
        likedCount = mResource.getLikedList().size()-1;//The last is open effects gridview btn
        mSelctedNum = likedCount;
        deleteCount = 0;
    }

    private void hidShowedEffectsImpl() {
        if(operatorType == ImageEditConstants.EXCHANGE_LIKED_EFFECT){
            resetOriginalItemsList();
            itemsList.clear();
            itemsList.addAll(0, originalItemsList);
        }
        operatorType=-1;

        if(null != mAdapter){
            mAdapter.notifyDataSetChanged();
        }

        initSelectedItems ();

        hidShowedEffects();

        Message msg = Message.obtain(mHandler,ImageEditConstants.EFFECT_SELECTED_BACK);
        msg.sendToTarget();
    }

    private void resetOriginalItemsList() {
        final ArrayList<EffectItem> lList = originalItemsList;
        final int size = lList.size();
        for(int i=0;i<size;i++){
            EffectItem item = lList.get(i);
            if(null == item){
                continue;
            }
            if(i<likedCount){
                item.selectedIndex = i;
            }else{
                item.selectedIndex = -1;
            }
        }
    }

    private void updateLikedItems() {
        ArrayList<EffectItem> likedList=mResource.getLikedList();
        EffectItem lastItem=likedList.get(likedList.size()-1);//The last is open effects gridview btn
        likedList.clear();

        int len=itemsList.size();
        for(int i=0;i<len;i++){
            EffectItem item=itemsList.get(i);
            if(null!=item && item.selectedIndex>=0){
                likedList.add(item);
            }
        }

        likedList.add(lastItem);//The last is open effects gridview btn
    }

    private void addSeletcedItem(int position, EffectItem item){
        if(mSelctedNum >= itemsList.size()){
            return;
        }

        item.selectedIndex = mSelctedNum;

        if(position!=mSelctedNum){//不是点击非选中的第一个
            exchangeSelectedItems(mSelctedNum, position);
            itemsList.set(mSelctedNum, item);
        }

        mSelctedNum++;
    }

    private void deleteSelectedItem(int position, EffectItem item){
        if(mSelctedNum<=DEFALUE_LIKED_SIZE){//不能全部删除，要保留默认数目
            return;
        }
        item.selectedIndex = -1;

        if(mSelctedNum > 0) {
            mSelctedNum--;
        }
        if(position != mSelctedNum){
            itemsList.remove(position);
            itemsList.add(mSelctedNum,item);
        }

        deleteCount++;
    }

    private void exchangeSelectedItems(int from, int to){
        for(int i=to;i>from;i--){
            EffectItem itemI = itemsList.get(i-1);
            itemsList.set(i, itemI);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        EffectItem item = itemsList.get(position);
        if(item.selectedIndex >= 0){
            deleteSelectedItem(position, item);
        }else{
            addSeletcedItem(position, item);
        }

        mAdapter.notifyDataSetChanged();

        operatorType=ImageEditConstants.EXCHANGE_LIKED_EFFECT;
    }

    @Override
    public boolean checkBeforeDragItem(int pos) {
        if(pos < mSelctedNum) {
            return true;
        }
        return false;
    }

    @Override
    public boolean checkExchangeItem(int position0, int position1){
        if(position0<mSelctedNum && position1<mSelctedNum){
            return true;
        }
        return false;
    }

    @Override
    public void onExchangeItem(int position0, int position1, EffectItem item0, EffectItem item1) {

        if(position0 < mSelctedNum && position1 < mSelctedNum){
            operatorType=ImageEditConstants.EXCHANGE_LIKED_EFFECT;
        }
    }

    public void showTipDlg(SharedPreferences sp) {
        mSp = sp;
        showTipDlgImpl(sp);
    }

    private void showTipDlgImpl(SharedPreferences sp) {
        String key = "Effect_Showed_Tip_Dlg";
        boolean tipShowed = sp.getBoolean(key, false);
        if(tipShowed)
            return;
        int width = UiUtils.screenWidth();
        int height = UiUtils.screenHeight();
        EffectShowedTipDlg.createEffectSelectedDlg(mActivity, 0, 0, width, height).show();
        Editor editor = sp.edit();
        editor.putBoolean(key, true);
        editor.commit();
    }

    /*
     * 保存喜欢的Items
     */
    private void saveLikedItems(int likedCount){
        if(null == mSp){
            return;
        }
        for(int i=0;i<likedCount;i++){
            EffectTypeResource.removeSharedPreferencesLikedShaderi(mSp, i);
        }

        final ArrayList<EffectItem>likedItems=mResource.getLikedList();
        final int len = likedItems.size();
        EffectTypeResource.saveSharedPreferencesLikedCount(mSp,len);
        for(int i=0; i<len;i++){
            EffectItem item=likedItems.get(i);
            if(null != item){
                EffectTypeResource.
                saveSharedPreferencesLikedEffecti(mSp, i, item);
            }
        }
    }

    /*
     * 保存不喜欢的Items
     */
    private void saveShowedItems() {
        final ArrayList<EffectItem> selectedArr=itemsList;
        final int len = selectedArr.size();
        int counter = 0;
        for (int i=0;i<len;i++){
            EffectTypeResource.removeSharedPreferencesShowedEffecti(mSp, i);
            EffectItem item = selectedArr.get(i);
            if(null !=item && item.selectedIndex<0){
                EffectTypeResource.
                saveSharedPreferencesShowedEffecti(mSp, counter, item);
                counter++;
            }
        }
        EffectTypeResource.saveSharedPreferencesShowedCount(mSp, counter);
    }

    private void saveItemsInThread(final int likedCount){
        Thread t = new Thread() {
            @Override
            public void run() {
                saveShowedItems();
                saveLikedItems(likedCount);
            }
        };
        t.start();
    }

}
