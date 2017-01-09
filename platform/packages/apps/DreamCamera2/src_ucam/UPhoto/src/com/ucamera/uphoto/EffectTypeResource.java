/*
 *   Copyright (C) 2010,2012 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.uphoto;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.SparseArray;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import com.ucamera.ucam.compatible.Models;

//import com.ucamera.uphoto.R;

public class EffectTypeResource {
//    private HashMap<Integer, ArrayList<EffectItem>> mHashMap = new HashMap<Integer, ArrayList<EffectItem>>();
    public static final int UGIF_EFFECT_RES = 0;
    public static final int UPHOTO_EFFECT_RES = 1;
    private HashMap<Integer, ArrayList<EffectItem>> mHashMap;
    private ArrayList<EffectItem> currentCategoryList;//default filters
    private int selectedItemsCount;//
//    private static EffectTypeResource mInstance;
    private static final SparseArray<EffectTypeResource> mResArray = new SparseArray<EffectTypeResource>();

    /*
     * BUG FIX: 4751
     * DATE: 2013-08-28
     */
    public static EffectTypeResource getResInstance(int resType) {
//        if(mInstance == null) {
//            mInstance = new EffectTypeResource();
//        }

//        return mInstance;
        EffectTypeResource mInstance = mResArray.get(resType);
        if(null == mInstance) {
            mInstance = new EffectTypeResource();
            mResArray.put(resType, mInstance);
        }
        return mInstance;
    }

    public void initLikedList(SharedPreferences sp) {
        upgradeEffectPreferences(sp);
        if(null == currentCategoryList) {
            currentCategoryList = new ArrayList<EffectItem>();
            int len = readSharedPreferencesLikedCount(sp);
            if(len<=0){
                currentCategoryList.addAll(0, getLikedItems());

                //This is open effects gridview btn
                EffectItem item = new EffectItem(-1, "", "effect_showed_btn_selector");
                currentCategoryList.add(item);
            }else{
                for(int i=0;i<len;i++){
                    EffectItem item = readSharedPreferencesLikedEffecti(sp,i);
                    if(null!=item){
                        currentCategoryList.add(item);
                    }
                }
            }
        }
    }

    public ArrayList<EffectItem> getLikedList() {
        return currentCategoryList;
    }

    public void exchangeLikedItems(int position0, int position1, EffectItem item0, EffectItem item1){
        if(null==currentCategoryList ||
                position0>=currentCategoryList.size() || position1>=currentCategoryList.size()){
            return;
        }
        currentCategoryList.set(position0, item1);
        currentCategoryList.set(position1, item0);
    }

    public void updateLikedList(int index, EffectItem item){
        if(null == currentCategoryList) {
            return;
        }
        currentCategoryList.set(index, item);
    }

    public void updateLikedList(EffectItem[]itemsArr, int len){
        if(null == currentCategoryList) {
            return;
        }
        for(int i=0;i<len;i++){
            currentCategoryList.set(i, itemsArr[i]);
        }
    }

    /*
     * The default effect
     */
    private ArrayList<EffectItem>defaultLikedItems;
    private ArrayList<EffectItem> getLikedItems() {
        if(null==defaultLikedItems){
            int index = 1;
            defaultLikedItems = new ArrayList<EffectItem>();
            defaultLikedItems.add(new EffectItem(14,"text_effect_rainbrown", "effect_rainbrown", index++));//right
            defaultLikedItems.add(new EffectItem(14,"text_effect_hope", "effect_hope",index++));//right
            defaultLikedItems.add(new EffectItem(14,"text_effect_sparkling", "effect_sparkling",index++));//right
            defaultLikedItems.add(new EffectItem(14,"text_effect_fresh", "effect_fresh",index++));//right
            defaultLikedItems.add(new EffectItem(100,"text_effect_film", "effect_film",index++));//right
            defaultLikedItems.add(new EffectItem(160,"text_effect_black_white", "effect_black_white",index++));//right

            defaultLikedItems.add(new EffectItem(187, "text_effect_autumn", "effect_autumn",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_bright", "effect_bright",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_bless", "effect_bless",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_colorful", "effect_colorful",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_1839", "effect_1839",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_bloom", "effect_bloom",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_blue", "effect_blue",index++));
            defaultLikedItems.add(new EffectItem(187, "text_effect_cute", "effect_cute",index++));
            defaultLikedItems.add(new EffectItem(14,"text_effect_floweriness", "effect_floweriness",index++));
        }
        return defaultLikedItems;
    }

    /*
     * The other effect
     */
    private ArrayList<EffectItem> getUnLikedItems() {
        ArrayList<EffectItem> unLikedList = new ArrayList<EffectItem>();
        unLikedList.add(new EffectItem(148,"text_effect_america", "effect_america"));//right
        unLikedList.add(new EffectItem(146,"text_effect_france", "effect_france"));//right
        unLikedList.add(new EffectItem(146, "text_effect_desert", "effect_desert"));//right
        unLikedList.add(new EffectItem(14,"text_effect_reflection", "effect_reflection",-1));//right
        unLikedList.add(new EffectItem(100, "text_edit_symmetry_horizontal_left", "effect_h_symmetry",-1));//right
        unLikedList.add(new EffectItem(102, "text_edit_symmetry_vertical_down", "effect_v_symmetry",-1));//right
        /*
         * FIX BUG:  4782 4832
         * BUG CAUSE: not supported dot effect
         * DATE: 2013-09-09
         */
        /* SPRD: not needed in platform of spreadtrum @{
        if(!dotNotCompitable()) {
            unLikedList.add(new EffectItem(5, "text_effect_dot", "effect_dot",-1));//right
        }
        @} */
        unLikedList.add(new EffectItem(5,"text_effect_cool", "effect_cool",-1));//right
        unLikedList.add(new EffectItem(12,"text_effect_negative",  "effect_negative",-1));//right
        unLikedList.add(new EffectItem(12,"text_effect_thermal", "effect_thermal",-1));//right
        unLikedList.add(new EffectItem(62,"text_effect_sketch", "effect_sketch",-1));//right
        unLikedList.add(new EffectItem(14,"text_effect_fly", "effect_fly"));
        unLikedList.add(new EffectItem(14,"text_effect_gouache", "effect_gouache", -1));
        unLikedList.add(new EffectItem(14,"text_effect_pencil", "effect_pencil"));
        unLikedList.add(new EffectItem(14,"text_effect_happy", "effect_happy"));
        unLikedList.add(new EffectItem(14,"text_effect_lotus", "effect_lotus"));
        unLikedList.add(new EffectItem(122,"text_effect_jiangnan", "effect_jiangnan"));//right
        unLikedList.add(new EffectItem(187, "text_effect_japan", "effect_japan"));//right
        unLikedList.add(new EffectItem(22, "text_effect_korea", "effect_korea"));//right
        unLikedList.add(new EffectItem(100, "text_effect_mosaic","effect_mosaic"));//right

        return unLikedList;
    }

    private ArrayList<EffectItem> showedList;
    public void initEffectShowedList(SharedPreferences sp) {
        if(null == showedList){
            showedList = new ArrayList<EffectItem>();
            ArrayList<EffectItem>likedList=getLikedList();
            int likedSize=likedList.size()-1;//The last is open effects gridview btn
            for(int i=0;i<likedSize;i++){
                showedList.add(likedList.get(i));
            }

            int showedLen=readSharedPreferencesShowedCount(sp);
            /*
             * FIX BUG: 4866
             * DATA 2013-09-30
             */
            int effectsNumber = readSharedPreferencesEffectsNumber(sp);
            if(showedLen<=0){
                if(effectsNumber<=0 || showedList.size() < effectsNumber){
                    showedList.addAll(showedList.size(), getUnLikedItems());
                    saveSharedPreferencesEffectsNumber(sp, showedList.size());
                }
            }else{
                for(int i=0;i<showedLen;i++){
                    EffectItem item = readSharedPreferencesShowedEffecti(sp,i);
                    if(null!=item){
                        showedList.add(item);
                    }
                }
            }
        }
    }

    public ArrayList<EffectItem> getEffectShowedList() {
        return showedList;
    }

    public int getSelectedItemCount(){
        return selectedItemsCount;
    }
    public ArrayList<EffectItem> getEffectItem(int key) {
        return mHashMap.get(key);
    }

    private EffectTypeResource() {
//        initResource();
    }

    private boolean dotNotCompitable() {
        return Arrays.asList(new String[]{
                Models.Meizu_M030,Models.Meizu_M031,Models.Meizu_M032,Models.Meizu_MEIZU_MX,Models.Meizu_MX,
                Models.SN_ISW11M,Models.SN_ISW13F,Models.Samsung_SGH_I777,Models.SPHS_on_Hsdroid,
                Models.Samsung_GT_I9100,Models.Samsung_GT_I9220,Models.Samsung_GT_I9103,Models.Samsung_GT_I9300,
                Models.Lenovo_K860i, Models.Meizu_M040, Models.Lenovo_A288t, Models.SP7710GA, Models.Lenovo_A390t,
                Models.KONKA_K3, Models.YUYI_D902
        }).contains(Models.getModel());
    }

    private void initResource() {
        if(null == mHashMap){
            mHashMap = new HashMap<Integer, ArrayList<EffectItem>>();
        }
        //LOMO
        ArrayList<EffectItem> arrayList = getLomo();
        mHashMap.put(ImageEditConstants.EFFECT_LOMO, arrayList);

        //HDR
        arrayList = getHDR();
        mHashMap.put(ImageEditConstants.EFFECT_HDR, arrayList);

        //SKIN
        arrayList = getSkin();
        mHashMap.put(ImageEditConstants.EFFECT_SKIN, arrayList);

        //VIVID LIGHT
        arrayList = getVividLight();
        mHashMap.put(ImageEditConstants.EFFECT_VIVID_LIGHT, arrayList);

        //SKETCH
        arrayList = getSketch();
        mHashMap.put(ImageEditConstants.EFFECT_SKETCH, arrayList);

        //COLOR FULL
        arrayList = getColorFull();
        mHashMap.put(ImageEditConstants.EFFECT_COLORFULL, arrayList);

        //FUNNY
        arrayList = getFunny();
        mHashMap.put(ImageEditConstants.EFFECT_FUNNY, arrayList);

        //NOSTALGIA
        arrayList = getNostalgia();
        mHashMap.put(ImageEditConstants.EFFECT_NOSTALGIA, arrayList);

        //BLACKWHITE
        arrayList = getBlackWhite();
        mHashMap.put(ImageEditConstants.EFFECT_BLACKWHITE, arrayList);

        //DEFORM
        arrayList = getDeform();
        mHashMap.put(ImageEditConstants.EFFECT_DEFORM, arrayList);
    }

    private ArrayList<EffectItem> getLomo() {
        //0, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {0, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "LOMO" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getHDR() {
        //1, 16, 210, 211, 212, 213, 214
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {1, 16, 210, 211, 212, 213, 214};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "HDR" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getSkin() {
        //200, 201, 202, 203, 204, 205
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {200, 201, 202, 203, 204, 205};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Skin" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getVividLight() {
        //180, 181, 182, 183, 184, 185, 186, 187
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {180, 181, 182, 183, 184, 185, 186, 187};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Light" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getSketch() {
        //62, 61, 60, 7, 30, 31, 32, 33, 34, 35, 36, 37
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {62, 61, 60, /* 7 */ 30, 31, 32, 33, 34, 35, 36, 37};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Sketch" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getColorFull() {
        //5, 6, 22, 23, 24, 25, 104, 105, 106, 107, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {5, 6, 22, 23, 24, 25, 104, 105, 106, 107, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Color" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getFunny() {
        //3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Funny" + (index + 1)));
//        }
        return arrayList;
    }

    private ArrayList<EffectItem> getNostalgia() {
        //50, 51, 52, 53, 54, 55, 56, 57
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {50, 51, 52, 53, 54, 55, 56, 57};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Nostalgia" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getBlackWhite() {
        //4, 2, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {4, 2, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "BlackWhite" + (index + 1)));
//        }

        return arrayList;
    }

    private ArrayList<EffectItem> getDeform() {
        //19, 40, 41, 42, 43, 44, 100, 101, 102, 103
        ArrayList<EffectItem> arrayList = new ArrayList<EffectTypeResource.EffectItem>();
//        int[] effects = {19, 40, 41, 42, 43, 44, 100, 101, 102, 103};
//        for(int index = 0; index < effects.length; index++) {
//            arrayList.add(new EffectItem(effects[index], "Deform" + (index + 1)));
//        }

        return arrayList;
    }

    public static class EffectItem {
        public int mTypeValue;// effect icon value: R.drawable.effect_line
        public String mTextValue;//etc "R.string.text_effect_nightmode" just use "text_effect_nightmode" as a string
        public String rIconName;//etc "R.drawable.effect_line" just use "effect_line" as a string
        public String mShaderName;
        public int selectedIndex; // this used to order the effects when displaying
//        public boolean mNeedTexture; // need texture or not

        public EffectItem(int typeValue, String textValue, String rIconName) {
            this(typeValue, textValue, rIconName, -1);
        }

        public EffectItem(int typeValue, String textValue, String rIconName, int selected) {
            this(typeValue, textValue, rIconName, null, selected);
        }

        public EffectItem(int typeValue, String textValue, String rIconName, String shaderName) {
           this(typeValue, textValue, rIconName, shaderName, -1);
        }

        public EffectItem(int typeValue, String textValue, String rIconName, String shaderName, int selected) {
            mTypeValue = typeValue;
            mTextValue = textValue;
            this.rIconName = rIconName;
            this.mShaderName = shaderName;
            if(shaderName == null) {
                this.mShaderName = rIconName + ".fs";
            }
            this.selectedIndex = selected;
//            this.mNeedTexture = needTexture;
        }

        public String toString() {
            StringBuffer buf = new StringBuffer();
            buf.append("[ ");
            buf.append("mTypeValue=");
            buf.append(mTypeValue);
            buf.append(" , mTextValue=");
            buf.append(mTextValue);
            buf.append(" , rIconName=");
            buf.append(rIconName);
            buf.append(" , mShaderName=");
            buf.append(mShaderName);
            buf.append(" , selectedIndex=");
            buf.append(selectedIndex);
            buf.append(" ]");

            return buf.toString();
        }


        public String toJson() throws JSONException {
            JSONObject jobj = new JSONObject();
            jobj.put("mTypeValue", mTypeValue);
            jobj.put("mTextValue", mTextValue);
            jobj.put("rIconName", rIconName);
            jobj.put("mShaderName", mShaderName);
            jobj.put("selectedIndex", selectedIndex);
//            jobj.put("needTexture", mNeedTexture);
            return jobj.toString();
        }
    }

    public static EffectItem jsonToEffectItem(String jsonStr) throws JSONException{
        EffectItem item = null;
        JSONObject jobj = new JSONObject(jsonStr);
        int typeValue = jobj.getInt("mTypeValue");
        String textValue = jobj.getString("mTextValue");
        String rIconName = jobj.getString("rIconName");
        String shaderName = jobj.getString("mShaderName");
        int selected = jobj.getInt("selectedIndex");
//        boolean needTexture = jobj.getBoolean("needTexture");
        item = new EffectItem(typeValue, textValue, rIconName,shaderName, selected);
        return item;
    }

    public static boolean saveSharedPreferences(SharedPreferences sp,String key, String str) {
        Editor editor = sp.edit();
        editor.putString(key, str);
        return editor.commit();
    }

    public static String readSharedPreferences(SharedPreferences sp,String key) {
        return sp.getString(key, null);
    }

    private static EffectItem readEffectItem(SharedPreferences sp, String key){
        EffectItem item = null;
        String jstr = readSharedPreferences(sp,key);
        if(null == jstr){
            return null;
        }
        try{
            item = jsonToEffectItem(jstr);
        }catch(Exception e){
            e.printStackTrace();
            item = null;
        }
        return item;
    }

    private static boolean saveEffectItem(SharedPreferences sp, String key, EffectItem item){
        String jstr = null;
        try{
            jstr = item.toJson();
        }catch(Exception e){
            e.printStackTrace();
            jstr = null;
        }
        if(jstr == null){
            return false;
        }
        return saveSharedPreferences(sp,key,jstr);
    }

    private final static String LIKED="Effects_Liked_";
    private final static String SHOWED="Effects_Showed_";
    private final static String LIKED_COUNT=LIKED+"COUNT";
    private final static String SHOWED_COUNT=SHOWED+"COUNT";
    private final static String EFFECTS_NUMBER="Effects_Number";

    public static boolean saveSharedPreferencesEffectsNumber(SharedPreferences sp, int number){
        try{
            Editor editor = sp.edit();
            editor.putInt(EFFECTS_NUMBER, number);
            return editor.commit();
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }
    }

    public int readSharedPreferencesEffectsNumber(SharedPreferences sp){
        return sp.getInt(EFFECTS_NUMBER, 0);
    }

    public int readSharedPreferencesLikedCount(SharedPreferences sp){
        return sp.getInt(LIKED_COUNT, 0);
    }

    public static boolean saveSharedPreferencesLikedCount(SharedPreferences sp, int count){
        try{
            Editor editor = sp.edit();
            editor.putInt(LIKED_COUNT, count);
            return editor.commit();
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }
    }

    public static EffectItem readSharedPreferencesLikedEffecti(SharedPreferences sp, int index){
        String key = LIKED+index;
        return readEffectItem(sp, key);
    }

    public static boolean saveSharedPreferencesLikedEffecti(SharedPreferences sp, int index, EffectItem item){
        String key = LIKED+index;
        return saveEffectItem(sp, key,item);
    }

    public static boolean removeSharedPreferencesLikedShaderi(SharedPreferences sp, int index){
        String key = LIKED+index;
        Editor editor = sp.edit();
        editor.remove(key);
        return editor.commit();
    }

    public static boolean saveSharedPreferencesLikedEffectsv(SharedPreferences sp, int []indices, EffectItem []items){
        try{
            Editor editor = sp.edit();
            for(int i=0; i < indices.length; i++){
                String key = LIKED + indices[i];
                String value = items[i].toJson();
                editor.putString(key, value);
            }
            return editor.commit();
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }
    }

    public int readSharedPreferencesShowedCount(SharedPreferences sp){
        return sp.getInt(SHOWED_COUNT, 0);
    }

    public static boolean saveSharedPreferencesShowedCount(SharedPreferences sp, int count){
        try{
            Editor editor = sp.edit();
            editor.putInt(SHOWED_COUNT, count);
            return editor.commit();
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }
    }

    public static EffectItem readSharedPreferencesShowedEffecti(SharedPreferences sp, int index) {
        String key = SHOWED+index;
        return readEffectItem(sp, key);
    }

    public static boolean saveSharedPreferencesShowedEffecti(SharedPreferences sp, int index, EffectItem item){
        String key = SHOWED+index;
        return saveEffectItem(sp, key,item);
    }

    public static boolean removeSharedPreferencesShowedEffecti(SharedPreferences sp, int index){
        String key = SHOWED+index;
        Editor editor = sp.edit();
        editor.remove(key);
        return editor.commit();
    }

    public static boolean saveSharedPreferencesShowedEffectsv(SharedPreferences sp, int []indices, EffectItem []items){
        try{
            Editor editor = sp.edit();
            for(int i=0; i < indices.length; i++){
                String key = SHOWED + indices[i];
                String value = items[i].toJson();
                editor.putString(key, value);
            }
            return editor.commit();
        }catch(Exception e){
            e.printStackTrace();
            return false;
        }
    }

    private final static int CURRENT_LOCAL_VERSION = 5;
    private final static String KEY_LOCAL_VERSION = "key_local_version";
    public static void upgradeEffectPreferences(SharedPreferences pref) {
        int version;
        try {
            version = pref.getInt(KEY_LOCAL_VERSION, 0);
        } catch (Exception ex) {
            version = 0;
        }
        if (version == CURRENT_LOCAL_VERSION) return;

        SharedPreferences.Editor editor = pref.edit();
        if (version < CURRENT_LOCAL_VERSION) {
            editor.clear();
        }

        editor.putInt(KEY_LOCAL_VERSION, CURRENT_LOCAL_VERSION);
        editor.commit();
    }
}
