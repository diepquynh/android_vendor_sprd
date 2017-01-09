package com.android.server.power;

import android.content.ContentResolver;
import android.database.ContentObserver;

public class AbsPowerManagerServiceUtils {

    public void registerButtonLightOffTimeOut(ContentResolver resolver, ContentObserver settingsObserver){

    }

    public void updateButtonLightOffTimeOut(ContentResolver resolver){

    }

    public int getButtonOffTimeoutSetting(){
        return 0;
    }

    public void setBootCompleted(boolean bootCompleted){

    }

    public boolean isThermalEnabled(){
        return false;
    }

    public void thermalEnabled(boolean en){

    }

}
