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
        context.startService(new Intent(context, PreDefineContactsService.class));
    }
}
