package com.sprd.cellbroadcastreceiver.data.itf;
 

public class SoundSettingImpl implements  ISoundSetting {

    public SoundSettingImpl()
    {
        mszSoundURI = null; 
        mbVibrate = false;
        mbNotification = false;
    }

    @Override
    public void setSoundURI(String szUri) {
        
        if( szUri == null || szUri.trim().isEmpty()){
            mszSoundURI = null;
        }
        else{
          mszSoundURI = new String(szUri.trim());
        }
        
    }

    @Override
    public void setVibrate(boolean bVibrate) {
        mbVibrate = bVibrate;
        
    }

    @Override
    public void setNotification(boolean bNotification) {
        mbNotification = bNotification;
        
    }

    @Override
    public String getSoundURI() {
        // TODO Auto-generated method stub
        return mszSoundURI;
    }

    @Override
    public boolean isVibrate() {
        // TODO Auto-generated method stub
        return mbVibrate;
    }

    @Override
    public boolean isNotification() {
        // TODO Auto-generated method stub
        return mbNotification;
    }

    public boolean isEmpty(){
        return (getSoundURI() == null && getSoundURI().isEmpty() && !isVibrate()  && !isNotification());
    }

    private boolean mbVibrate;
    private boolean mbNotification;
    private String mszSoundURI = null;
}
