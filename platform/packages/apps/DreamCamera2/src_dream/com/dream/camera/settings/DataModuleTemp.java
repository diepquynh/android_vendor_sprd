package com.dream.camera.settings;

import java.util.Set;

import android.content.Context;
import android.util.ArraySet;
import android.util.Log;
import java.util.List;

public class DataModuleTemp extends DataModuleInterfacePV {

    public DataModuleTemp(Context context) {
        super(context);
    }

    @Override
    public void changeCurrentModule(DataStructSetting dataSetting) {
        super.changeCurrentModule(dataSetting);

        // Change the handle of sharepreference
        changSPB(dataSetting);
    }

    @Override
    protected void fillEntriesAndSummaries() {
        // TODO Auto-generated method stub
    }
    @Override
    protected void setMutex(String key, Object newValue, Set<String> keyList) {
        // TODO Auto-generated method stub

    }
}
