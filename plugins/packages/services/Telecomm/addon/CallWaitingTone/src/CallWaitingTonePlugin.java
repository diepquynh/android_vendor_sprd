package plugin.sprd.callwaitingtone;

import com.sprd.server.telecom.TelcelCallWaitingToneHelper;
import android.app.AddonManager;
import android.content.Context;
import android.media.SoundPool;
import android.media.AudioManager;

import plugin.sprd.callwaitingtone.R;


public class CallWaitingTonePlugin extends TelcelCallWaitingToneHelper implements AddonManager.InitialCallback {

    private Context mContext;
    private SoundPool mCallWaitingToneSoundPool;

    public void CallWaitingTonePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void stop3rdCallWaitingTone () {
        if(mCallWaitingToneSoundPool != null) {
            mCallWaitingToneSoundPool.stop(1);
            mCallWaitingToneSoundPool.unload(1);
            mCallWaitingToneSoundPool.release();
            mCallWaitingToneSoundPool = null;
        }
    }

    public void play3rdCallWaitingTone () {
        if (mCallWaitingToneSoundPool == null) {
            int stream = AudioManager.STREAM_VOICE_CALL;
            mCallWaitingToneSoundPool = new SoundPool(10, stream, 0);

            mCallWaitingToneSoundPool.load(mContext, R.raw.call_waiting_tone, 1);
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
            }
            mCallWaitingToneSoundPool.play(1, 1, 1, 0, -1, 1);
        }
    }


    public boolean is3rdCallWaitingToneSupport () {
        return true;
    }
}
