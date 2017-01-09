
package com.android.ims.internal;

interface IImsServiceListenerEx {
    /**
     * Notifies the result of operation.
     * param id: request id
     * param type: IMS operation type
     * IMS_OPERATION_SWITCH_TO_VOWIFI = 0;
     * IMS_OPERATION_SWITCH_TO_VOLTE = 1;
     * IMS_OPERATION_HANDOVER_TO_VOWIFI = 2;
     * IMS_OPERATION_HANDOVER_TO_VOLTE = 3;
     * IMS_OPERATION_SET_VOWIFI_UNAVAILABLE = 4;
     * IMS_OPERATION_CANCEL_CURRENT_REQUEST = 5;
     * IMS_OPERATION_CP_REJECT_SWITCH_TO_VOWIFI = 6;
     * IMS_OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI = 7;
     */
    void operationSuccessed(int id, int type);

    /**
     * Notifies the result of operation.
     * param id: request id
     * param reason: failed reason
     * param type: IMS operation type
     * IMS_OPERATION_SWITCH_TO_VOWIFI = 0;
     * IMS_OPERATION_SWITCH_TO_VOLTE = 1;
     * IMS_OPERATION_HANDOVER_TO_VOWIFI = 2;
     * IMS_OPERATION_HANDOVER_TO_VOLTE = 3;
     * IMS_OPERATION_SET_VOWIFI_UNAVAILABLE = 4;
     * IMS_OPERATION_CANCEL_CURRENT_REQUEST = 5;
     * IMS_OPERATION_CP_REJECT_SWITCH_TO_VOWIFI = 6;
     * IMS_OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI = 7;
     */
    void operationFailed(int id, String reason, int type);

     /**
     * notify the call end event of IMS.
     * param type: IMS feature type
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
     * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
     */
    void imsCallEnd(int type);
}
