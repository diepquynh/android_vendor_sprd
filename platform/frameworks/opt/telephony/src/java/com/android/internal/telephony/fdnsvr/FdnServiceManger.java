package com.android.internal.telephony.fdnsvr;

import java.util.List;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

import com.android.internal.telephony.IFdnService;

/**
 * @hide
 */
public class FdnServiceManger {

    private FdnInterface mFdnInterface;
    private IFdnService mService;
    private Context mContext;
    public  static final String FDNSERVICE_PACKAGENAME = "com.spread.cachefdn";
   
    public FdnServiceManger(Context context) {
        mContext = context;
        mService = FdnServiceUtil.getIFdnService(mContext);
    }

    public interface FdnInterface {

        public boolean getFdnEnable();

        public boolean isNeedFilterMessaging();

    }

    public int process(Context context, int command, long subid,
            List<String> adresslist, byte[] bytes) {
        try {
            if (mFdnInterface == null) {
                Log.i(FdnServiceUtil.TAG,
                        "FdnServiceManger---->process()---->FdnInterface == null, FdnInterface must be imlpemeted");
                return 0;
            }

            if(!getFdnEnable()){
                Log.i(FdnServiceUtil.TAG,
                        "FdnServiceManger----->fdn function is unable");
                return 0;
            }

            if(!isNeedFilterMessaging()){
                Log.i(FdnServiceUtil.TAG,
                        "FdnServiceManger----->no need to filter");
                return 0;
            }

            if (getIFdnService(getContext()) == null) {
                Log.i(FdnServiceUtil.TAG,
                        "FdnServiceManger----->getIFdnService() == null");
                return 0;
            }

            if (getFdnEnable() && isNeedFilterMessaging()) {
                Log.i(FdnServiceUtil.TAG,
                        "FdnServiceManger----->begin to IFdnservice process() ");
                return getIFdnService(getContext()).process(command, subid, adresslist, bytes);
            } 
        } catch (Exception e) {
            return 0;
        }
        return 0;
    }

    private boolean getFdnEnable() {
        if (mFdnInterface == null) {
            Log.i(FdnServiceUtil.TAG,
                    "getFdnEnable----->mFdnInterface == null ");
            return false;
        }
        return mFdnInterface.getFdnEnable();
    }
    
    private Context getContext(){
        return mContext;
    }

    private IFdnService getIFdnService(Context context) {   
        return mService;
    }

    private boolean isNeedFilterMessaging() {
        if (mFdnInterface == null) {
            return false;
        }
        return mFdnInterface.isNeedFilterMessaging();
    }

    public void setFdnListner(final FdnInterface fdnInterface) {
        this.mFdnInterface = fdnInterface;
    }
}
