package android.app;

import android.content.Context;

public class PowerGuruFactoryEx extends PowerGuruFactory {

    public PowerGuruFactoryEx(){

    }

    public AbsPowerGuru createExtraPowerGuru(IPowerGuru service, Context context){
        return new PowerGuru(service, context);
    }

}

