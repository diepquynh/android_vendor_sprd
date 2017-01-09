package com.sprd.cellbroadcastreceiver.data;

import android.database.Cursor;

import com.sprd.cellbroadcastreceiver.data.itf.ISoundSetting;
import com.sprd.cellbroadcastreceiver.data.itf.ModifyImp;
import com.sprd.cellbroadcastreceiver.data.itf.SoundSettingImpl;
import com.sprd.cellbroadcastreceiver.provider.ChannelTableDefine;
 
public class ChannelItemData extends ModifyImp implements ChannelTableDefine {

	public ChannelItemData(Cursor cursor, int nIndexOfArray/*, ISoundSetting  objSoundSetting*/) {
	    mnIndexOfArray = nIndexOfArray;
//	    mobjSoundSetting = objSoundSetting;
		init(cursor);
	}

	public ChannelItemData(int nIndexOfArray) {
	    mnIndexOfArray = nIndexOfArray;
	}

	protected boolean init(Cursor cursor) {
		if (cursor == null) {
			return false;
		}
		try {
			setId(cursor.getInt(INDEX_ID));
		    setChannelId(cursor.getInt(INDEX_CHANNEL_ID))     ;
			setChannelName(cursor.getString(INDEX_CHANNEL_NAME));
			setEnabled(cursor.getInt(INDEX_ENABLE));
			setSubId(cursor.getInt(INDEX_SUB_ID));
			setSave(cursor.getInt(INDEX_SAVE));
			setMcc(cursor.getInt(INDEX_MCC));
			setMnc(cursor.getInt(INDEX_MNC));

	        getSoundSetting().setVibrate(cursor.getInt(INDEX_VIBRATE)!= 0);
	        getSoundSetting().setNotification(cursor.getInt(INDEX_NOTIFICATION)!= 0);
	        getSoundSetting().setSoundURI(cursor.getString(INDEX_SOUND_URI));
			//initNotification(cursor);
			return true;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return false;
	}

	private void initNotification(Cursor cursor)
	{
//	    if( !getSoundSetting().isEmpty()){
//	        return ;
//	    }
	    getSoundSetting().setVibrate(cursor.getInt(INDEX_VIBRATE)!= 0);
	    getSoundSetting().setNotification(cursor.getInt(INDEX_NOTIFICATION)!= 0);
	    getSoundSetting().setSoundURI(cursor.getString(INDEX_SOUND_URI));
	}

	public int getID() {
		return mId;
	}

	public int getChannelId() {
		return mChannel_id;
	}

	public String getChannelName() {
		return mChannel_name;
	}

	public int getEnabled() {
		return mEnable;
	}

	public int getSubId() {
		return mSub_id;
	}

	public int getSave() {
		return mSave;
	}

	public int getMcc() {
		return mMcc;
	}

	public int getMnc() {
		return mMnc;
	}

	public void setId(int _id) {
		mId = _id;
	}

	public void setChannelId(int channel_id) {
		mChannel_id = channel_id;
	}

	public void setChannelName(String channel_name) {
		mChannel_name = channel_name;
	}

	public void setEnabled(int enabled) {
		mEnable = enabled;
	}

	public void setSubId(int sub_id) {
		mSub_id = sub_id;
	}

	public void setSave(int save) {
		mSave = save;
	}

	public void setMcc(int mcc) {
		mMcc = mcc;
	}

	public void setMnc(int mnc) {
		mMnc = mnc;
	}

    public final int getIndexOfArray() {
        return mnIndexOfArray;
    }

    public String toString() {
        return "This itemdata's sub_id is:" + getSubId() + ", channel_id is:"
                + getChannelId() + ", channel_name is:" + getChannelName()
                + ", is Enabled:" + getEnabled() + ", is Saved: " + getSave()
                + " and the IndexOfArray is" + getIndexOfArray();
    }

    public ISoundSetting getSoundSetting()
    {
        if(mobjSoundSetting == null){
            mobjSoundSetting = new SoundSettingImpl();
            return mobjSoundSetting;
        }
//        else if(mobjSoundSetting.isEmpty()){
//            throw new RuntimeException(" Phone Set ,  Sound Setting Is Empty");
//        }
        return mobjSoundSetting; 
    }

    public void setSinglePhoneSound(ISoundSetting obj){
        mobjSoundSetting = obj;
    }

	private int mId = -1;
	private int mChannel_id;
	private String mChannel_name;
	private int mEnable;
	private int mSub_id;
	private int mSave;
	private int mMcc;
	private int mMnc;
	private final int mnIndexOfArray;
	
	private ISoundSetting  mobjSoundSetting;  
	
 
}
