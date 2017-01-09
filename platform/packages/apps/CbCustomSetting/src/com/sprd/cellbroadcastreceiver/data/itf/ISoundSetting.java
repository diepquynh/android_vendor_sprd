package com.sprd.cellbroadcastreceiver.data.itf;

public interface ISoundSetting {

    public void setSoundURI(String szUri);
    public void setVibrate(boolean bVibrate);
    public void setNotification(boolean bNotification);
    
    public String getSoundURI();
    public boolean isVibrate();
    public boolean isNotification();
    
    public boolean isEmpty();
}
