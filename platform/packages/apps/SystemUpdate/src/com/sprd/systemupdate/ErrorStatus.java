package com.sprd.systemupdate;

import android.content.Context;
import android.content.Intent;

public class ErrorStatus {

    private static final int OTA_GET_TOKEN_NO_VERSION = 1201;
    private static final int OTA_GET_TOKEN_NO_PRODUCT = 1202;
    private static final int OTA_GET_TOKEN_NEED_REGISTER = 1203;
    private static final int OTA_TOKEN_EXPIRED = 1204;
    private static final int OTA_TOKEN_AUTHENTICATE_FAILED = 1205;
    private static final int OTA_REGISTER_SUCCESS = 1207;
    private static final int OTA_REGISTER_UPDATE = 1208;
    private static final int OTA_REGISTER_FAILED = 1209;
    private static final int OTA_QUERY_DELTUM_NOT_FOUND = 1211;
    private static final int OTA_QUERY_DELTUM_SUCCESS = 1212;

    public static final String REGISTER = "register";
    public static final String CHECK = "check";
    public static final String NOTICE = "notice";

    public static int DealStatus(Context context, int status, String flag) {
        switch (status) {
        case OTA_GET_TOKEN_NO_VERSION:
        case OTA_GET_TOKEN_NO_PRODUCT:
            return 1;
        case OTA_GET_TOKEN_NEED_REGISTER:
            if (CHECK.equals(flag)) {
                return CheckupdateService.CHECK_UPDATE_NEED_REGISTER;
            } else {
                Intent intent = new Intent(context, PushService.class);
                if (NOTICE.equals(flag)) {
                    flag = PushService.NEED_REGISTER;
                }
                intent.putExtra("seed", flag);
                context.startService(intent);
            }
            break;
        case OTA_TOKEN_EXPIRED:
        case OTA_TOKEN_AUTHENTICATE_FAILED:
            if (CHECK.equals(flag)) {
                return CheckupdateService.CHECK_UPDATE_SEED_EXPRIED;
            } else if (REGISTER.equals(flag)) {
                return PushService.REGISTER_SEED_EXPRIED;
            }
        case OTA_REGISTER_SUCCESS:
        case OTA_REGISTER_UPDATE:
            return PushService.REGISTER_SUCCEED;
        case OTA_REGISTER_FAILED:
            return PushService.REGISTER_FAILED;
        case OTA_QUERY_DELTUM_NOT_FOUND:
            return CheckupdateService.CHECK_UPDATE_NO_UPDATE;
        case OTA_QUERY_DELTUM_SUCCESS:
            return CheckupdateService.CHECK_UPDATE_HAS_UPDATE;
        }
        return 0;

    }

}
