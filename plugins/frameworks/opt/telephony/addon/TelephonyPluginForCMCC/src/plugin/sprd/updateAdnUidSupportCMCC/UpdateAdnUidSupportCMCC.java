package plugin.sprd.updateAdnUidSupportCMCC;

import android.app.AddonManager;
import android.content.Context;

import com.android.internal.telephony.gsm.UsimPhoneBookManager;
import com.android.internal.telephony.plugin.TelephonyForCmccPluginsUtils;
import com.android.internal.telephony.uicc.AdnRecord;
import com.android.internal.telephony.uicc.IccConstants;
import com.android.internal.telephony.uicc.IccFileHandler;
import android.util.Log;

public class UpdateAdnUidSupportCMCC extends TelephonyForCmccPluginsUtils implements
        AddonManager.InitialCallback {
    private static final String LOGTAG = "UpdateAdnUidSupportCMCC";
    private Context mAddonContext;
    private static final int USIM_EFUID_TAG   = 0xC9;
    public UpdateAdnUidSupportCMCC(){
        Log.d(LOGTAG, "UpdateAdnUidSupportCMCC");
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void updateUidForAdn(UsimPhoneBookManager mUsimPhoneBookManager,int adnEf,int recNum,int adnIndex,AdnRecord adn,IccFileHandler mFh) {
        Log.d(LOGTAG, "UpdateAdnUidSupportCMCC updateUidForAdn");
        if(adn.isEmpty()) {

        }else {
            if (mUsimPhoneBookManager.getEfIdByTag(recNum,USIM_EFUID_TAG) <= 0) {
                Log.d(LOGTAG,"get EfUID failed,EFUID is not exist");
                return;
            }
            mFh.loadEFTransparent(IccConstants.EF_CC, 2,
                    mUsimPhoneBookManager.obtainMessage(mUsimPhoneBookManager.EVENT_EF_CC_LOAD_DONE, 1,1));
        }
    }
}
