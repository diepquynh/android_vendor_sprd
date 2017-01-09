package com.android.server.display;

public abstract class AbsDisplayPowerController {

    public void scheduleButtonTimeout(long now) {

    }


    public void updateButtonTimeout(int timeout){

    }

    public int getButtonTimeout(){
        return 0;
    }

    public void initButtonLight(){

    }

}
