package plugin.sprd.contactsdefaultcontact;

import android.util.Log;
import android.app.AppGlobals;
import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.os.RemoteException;
import android.util.Log;

public class PreDefineContactsReceiver extends BroadcastReceiver {

    private static final String TAG = "DefaultContactAddon";

    @Override
    public void onReceive(Context context, Intent intent) {
        boolean isFirstBoot = false;
        try{
            isFirstBoot = AppGlobals.getPackageManager().isFirstBoot();
        } catch(RemoteException e){
            e.printStackTrace();
        }
        Log.d(TAG, "isFirstBoot : " + isFirstBoot);

        if(isFirstBoot){
            context.startService(new Intent(context, PreDefineContactsService.class));
        }
    }
}
