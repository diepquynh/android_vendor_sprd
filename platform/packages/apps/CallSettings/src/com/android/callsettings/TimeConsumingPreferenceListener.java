package com.android.callsettings;

import android.preference.Preference;
import com.android.internal.telephony.CommandException;

public interface  TimeConsumingPreferenceListener {
    public void onStarted(Preference preference, boolean reading);
    public void onFinished(Preference preference, boolean reading);
    public void onError(Preference preference, int error);
    public void onException(Preference preference, CommandException exception);
    public void onUpdate(int reason);
    /* SPRD: add for callforward time @{ */
    public void onEnableStatus(Preference preference, int status);
    public void onUpdateTwinsPref(boolean toggled, int arg1, int arg2, String arg3, String arg4);
    /* @} */
}