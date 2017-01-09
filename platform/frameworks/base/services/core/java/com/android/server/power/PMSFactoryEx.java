package com.android.server.power;

import android.content.Context;
import android.hardware.display.DisplayManagerInternelEx;

import com.android.server.display.AbsDisplayPowerController;
import com.android.server.display.AbsDisplayPowerState;
import com.android.server.display.DisplayManagerServiceEx;
import com.android.server.display.DisplayPowerControllerEx;
import com.android.server.display.DisplayPowerStateEx;

public class PMSFactoryEx extends PMSFactory{

    public void initExtraPowerManagerService(Context context) {
        PowerManagerServiceEx.init(context);
    }

    public AbsPowerManagerServiceUtils createPowerManagerServiceUtils(Context context) {
        return PowerManagerServiceUtils.getInstance(context);
    }

    public AbsDisplayPowerController createExtraDisplayPowerController(Context context){
        return DisplayPowerControllerEx.getInstance(context);
    }

    public AbsDisplayPowerState createExtraDisplayPowerState(Context context){
        return DisplayPowerStateEx.getInstance(context);
    }

    public DisplayManagerInternelEx createExtraDisplayManagerService(Context context){
        return DisplayManagerServiceEx.getInstance(context);
    }

}
