package com.android.ims.internal;

import com.android.ims.internal.IImsServiceListenerEx;
import com.android.ims.internal.IImsRegisterListener;

interface IImsServiceEx {

      /**
     * Used for switch IMS feature.
     * param type:
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
     * return: request id
     */
    int switchImsFeature(int type);

    /**
     * Used for start IMS handover.
     * param targetType:
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
     * return: request id
     */
    int startHandover(int targetType);

    /**
     * Used for notify network unavailable.
     */
    void notifyNetworkUnavailable();

    /**
     * Used for get IMS feature.
     * return:
     * ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN = -1;
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
     */
    int getCurrentImsFeature();

    /**
     * Used for set IMS service listener.
     */
    void setImsServiceListener(IImsServiceListenerEx listener);

    /**
     * Used for get IMS register address.
     */
    String getImsRegAddress();

    /**
     * Used for set release VoWifi Resource.
     */
    void releaseVoWifiResource();

    /**
     * Used for set VoWifi unavailable.
     * param wifiState:
     * wifi_disabled = 0;
     * wifi_enabled = 1;
     * return: request id
      */
    int setVoWifiUnavailable(int wifiState, boolean isOnlySendAT);

    /**
     * Used for cancel current switch or handover request.
     * return: request id
     */
    int cancelCurrentRequest();

     /**
     * Used for register IMS register listener.
     */
    void registerforImsRegisterStateChanged(IImsRegisterListener listener);

    /**
     * Used for unregister IMS register listener.
     */
    void unregisterforImsRegisterStateChanged(IImsRegisterListener listener);
}
