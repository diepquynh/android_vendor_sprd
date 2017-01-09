
package com.sprd.engineermode.telephony;

import java.util.ArrayList;
import java.util.List;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.content.Intent;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class SimLock {
    final static String TAG = "SimLock";
    final static int TYPE_NULL = 0;
    final static int TYPE_SIM = 1;
    final static int TYPE_NETWORK = 2;
    final static int TYPE_NETWORK_SUBSET = 3;
    final static int TYPE_SERVICE = 4;
    final static int TYPE_CORPORATE = 5;
    private final static String[] SIMLOCK_TYPE_NAME = {
            "00", "PS", "PN", "PU", "PP", "PC"
    };

    private final static int SIMLOCK_STATUS = 0;
    private final static int SIMLOCK_IMSI = 1;

    final static int LOCK_INFO_MAX = 2;

    /* Construct init start */
    private boolean mIsInit = false;
    private int mType = 0;
    private boolean mIsLocked = false;
    private boolean mIsUnlockedPermanently = false;
    private List<String> mLockInfos = new ArrayList<String>();

    /* Construct init end */

    private String parseRsp(String response, int type) {
        switch (type) {
            case SIMLOCK_STATUS:
                if (response.contains(IATUtils.AT_OK)) {
                    String[] str = response.split("\n");
                    String[] str1 = str[0].split(":");
                    return str1[1];
                } else
                    return IATUtils.AT_FAIL;
            default:
                break;
        }

        return null;
    }

    public SimLock() {
    }

    public SimLock(int type) {
        mType = type;

        String rsp = IATUtils.sendATCmd(engconstents.ENG_AT_CLCK + SIMLOCK_TYPE_NAME[mType] + ",2",
                "atchannel0");
        Log.d(TAG, rsp);
        String parseRe = parseRsp(rsp, SIMLOCK_STATUS);
        Log.d(TAG, parseRe);
        if (parseRe.contains("1")) {
            mIsLocked = true;
        } else if (parseRe.contains("0")) {
            mIsLocked = false;
        } else {
            Log.d(TAG, "Get lock status ERROR");
        }

        /* UnlockedPermanently not support */
        mIsUnlockedPermanently = false;
        mIsInit = true;
    }

    public boolean lock(String psw) {
        String rsp = IATUtils.sendATCmd(engconstents.ENG_AT_CLCK + SIMLOCK_TYPE_NAME[mType] + ",1"
                + "," + psw,
                "atchannel0");

        if (rsp.contains(IATUtils.AT_OK)) {
            mIsLocked = true;
            return true;
        }

        return false;
    }

    public boolean unlock(String psw) {
        String rsp = IATUtils.sendATCmd(engconstents.ENG_AT_CLCK + SIMLOCK_TYPE_NAME[mType] + ",1"
                + "," + psw,
                "atchannel0");
        if (rsp.contains(IATUtils.AT_OK)) {
            mIsLocked = false;
            return true;
        }

        return false;
    }

    public boolean add(String imsi, String gid1, String gid2, String psw) {
        if (imsi == null) {
            String imsiRsp = IATUtils.sendATCmd(engconstents.ENG_AT_GETIMSI,
                    "atchannel0");

            if (imsiRsp.contains(IATUtils.AT_OK)) {
                imsi = parseRsp(imsiRsp, SIMLOCK_IMSI);
            } else {
                return false;
            }
        }
        String rsp = IATUtils.sendATCmd(engconstents.ENG_AT_ADDSIM + SIMLOCK_TYPE_NAME[mType] + ","
                + psw + ",2" + "," + imsi,
                "atchannel0");
        if (rsp.contains(IATUtils.AT_OK)) {
            return true;
        }

        return false;
    }

    public boolean remove() {
        return true;
    }

    public boolean unlockPermanently() {
        return false;
    }

    public int getLockRemainRetiresLeft() {
        int ckType = 1;
        return 5;
    }

    public int getUnlockRemainRetiresLeft() {
        int ckType = 2;
        return 5;
    }

    public boolean isInited() {
        return mIsInit;
    }

    public boolean isLocked() {
        return mIsLocked;
    }

    public boolean isSimLockInfoFull() {
        return (mLockInfos.size() == LOCK_INFO_MAX);
    }

    public boolean isSimLockInfoEmpty() {
        return (mLockInfos.size() == 0);
    }

    public boolean isUnlockPermanently() {
        return mIsUnlockedPermanently;
    }
}
