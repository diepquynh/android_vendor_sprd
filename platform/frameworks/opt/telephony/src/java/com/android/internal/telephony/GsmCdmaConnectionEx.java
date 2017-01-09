/* Created by Spreadtrum */

package com.android.internal.telephony;

import android.telecom.VideoProfile;
import android.telephony.Rlog;
import android.os.SystemProperties;

public class GsmCdmaConnectionEx extends GsmCdmaConnection {
    private static final String LOG_TAG = "GsmCdmaConnectionEx";
    protected boolean mIsVideo = false;
    boolean mIsSupportVT = SystemProperties.getBoolean("persist.sys.csvt", false);

    /** This is probably an MT call that we first saw in a CLCC response */
    public GsmCdmaConnectionEx (GsmCdmaPhone phone, DriverCall dc, GsmCdmaCallTracker ct, int index) {
        super(phone, dc, ct, index);
        mIsVideo = !dc.isVoice && mIsSupportVT;
        Rlog.d(LOG_TAG, "[GsmCdmaConnectionEx] mIsVideo = " + mIsVideo);
        if (mIsVideo) {
            setVideoState(VideoProfile.STATE_BIDIRECTIONAL);
        } else {
            setVideoState(VideoProfile.STATE_AUDIO_ONLY);
        }
    }

    /** This is an MO call, created when dialing */
    public GsmCdmaConnectionEx (GsmCdmaPhone phone, String dialString, GsmCdmaCallTracker ct,
                              GsmCdmaCall parent, boolean isVideo) {
        super(phone, dialString, ct, parent);
        mIsVideo = isVideo && mIsSupportVT;
        Rlog.d(LOG_TAG, "[GsmCdmaConnectionEx] mIsVideo = " + mIsVideo);
        if (mIsVideo) {
            setVideoState(VideoProfile.STATE_BIDIRECTIONAL);
        } else {
            setVideoState(VideoProfile.STATE_AUDIO_ONLY);
        }
    }
}
