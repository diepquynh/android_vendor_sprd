package android.os;

import android.content.Context;

public class PMFactoryEx extends PMFactory{

    public PMFactoryEx(){

    }

    public AbsPowerManager createExtraPowerManager(Context context){
        return new PowerManagerEx(context);
    }
}
