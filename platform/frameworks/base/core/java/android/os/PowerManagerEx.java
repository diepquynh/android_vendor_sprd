package android.os;

import android.content.Context;
import android.util.Log;

public class PowerManagerEx extends AbsPowerManager{

    private static final String TAG = "PowerManagerEx";
    private Context mContext;


    public PowerManagerEx(Context context){

        mContext = context;

    }

    private IPowerManagerEx getPowerManagerEx(){
        return IPowerManagerEx.Stub.asInterface(ServiceManager.getService("power_ex"));
    }

    /**
     * SPRD added for PowerOff Alarm
     * @hide
     */
    public void shutdownForAlarm() {
        try {
            getPowerManagerEx().shutdownForAlarm(false, true);
        } catch (RemoteException e) {
            Log.i(TAG,"shutdownForAlarm could not get remote service");
        }
    }

    /**
     * SPRD added for PowerOff Alarm
     * @hide
     */
    public void rebootAnimation() {
        try {
            getPowerManagerEx().rebootAnimation();
        } catch (RemoteException e) {
            Log.i(TAG,"rebootAnimation could not get remote service");
        }
    }

    @Override
    public void scheduleButtonLightTimeout(long now) {
        try {
            getPowerManagerEx().scheduleButtonLightTimeout(now);
        } catch (RemoteException e) {
        }
    }
}
