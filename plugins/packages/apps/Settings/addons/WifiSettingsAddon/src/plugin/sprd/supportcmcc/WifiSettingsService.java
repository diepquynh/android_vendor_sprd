package plugin.sprd.supportcmcc;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

public class WifiSettingsService extends Service {

    private final IWifiSettings.Stub mBinder = new IWifiSettings.Stub(){
        public void resetTimer() {
            WifiConnectionPolicy.resetTimer();
        }
        public void setManulConnectFlags(boolean enabled) {
            WifiConnectionPolicy.setManulConnectFlags(enabled);
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return mBinder;
    }

}
