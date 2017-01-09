/** Create by Spreadst */

package com.spreadst.lockscreen;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import android.content.res.Resources;
import android.util.Log;

public class ElsModel {

    private static final String TAG = "ElsModel";
    private static ElsModel elsModel = null;

    private ExpandLockscreenInfo[] mElsInfos;

    private ElsModel() {

    }

    public static ElsModel getInstance() {

        if (elsModel == null) {

            elsModel = new ElsModel();
        }

        return elsModel;
    }

    public ExpandLockscreenInfo[] getElsInfos(Resources res,
            boolean isNeedReload) {

        if ((mElsInfos == null || isNeedReload) && res != null) {

            loadPresetLcsInfos(res);
        }

        return mElsInfos;
    }

    private ExpandLockscreenInfo[] loadPresetLcsInfos(Resources res) {

        ArrayList<HashMap> elsList = Tools.parserXml(
                res.getIdentifier(Constants.CURRENT_PACKAGE_NAME + ":xml/"
                        + Constants.EXPAND_LOCKSCREEN_XML, null, null), res);
        if (elsList == null) {
            return null;
        }
        int elsListSize = elsList.size();
        Log.d(TAG, "elsListSize==" + elsListSize);
        mElsInfos = new ExpandLockscreenInfo[elsListSize];
        for (int i = 0; i < elsListSize; i++) {
            HashMap elsMapInfo = (HashMap) elsList.get(i);
            Iterator iter = elsMapInfo.entrySet().iterator();
            mElsInfos[i] = new ExpandLockscreenInfo();
            while (iter.hasNext()) {
                Map.Entry entry = (Map.Entry) iter.next();
                String key = (String) entry.getKey();
                String val = (String) entry.getValue();
                if (Constants.LOCKSCREEN_ID.equals(key)) {
                    mElsInfos[i].setId(Integer.parseInt(val));
                } else if (Constants.LOCKSCREEN_CLASS_NAME.equals(key)) {
                    mElsInfos[i].setClass_name(val);
                } else if (Constants.LOCKSCREEN_APK_NAME.equals(key)) {
                    mElsInfos[i].setApk_name(val);
                } else if (Constants.LOCKSCREEN_PACKAGE_NAME.equals(key)) {
                    mElsInfos[i].setPackage_name(val);
                } else if (Constants.LOCKSCREEN_PREVIEW_ID.equals(key)) {
                    mElsInfos[i].setPreview_id(val);
                } else if (Constants.LOCKSCREEN_NAME_STRING_ID.equals(key)) {
                    mElsInfos[i].setName_string_id(val);
                }
            }
        }
        return mElsInfos;
    }

    public ExpandLockscreenInfo getCurrentElsInfoById(int elsId, Resources res,
            boolean isNeedReload) {

        if ((mElsInfos == null || isNeedReload) && res != null) {
            loadPresetLcsInfos(res);
        }
        ExpandLockscreenInfo currenElsInfo = null;
        if (mElsInfos != null) {
            Log.d(TAG, "mElsInfos" + mElsInfos.length);
            for (ExpandLockscreenInfo elsInfo : mElsInfos) {
                if (elsInfo.getId() == elsId) {
                    currenElsInfo = elsInfo;
                    break;
                }
            }
        }
        return currenElsInfo;
    }
}
