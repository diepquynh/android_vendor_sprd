
package com.android.plat.mms.plugin.Impl;

public class NotifyStatus {
    // define base common value
    public static final int FAILURE = 0XFF000000;
    protected static final int SUCC = 0XEE000000;
    private static final int DETAIL_MSAK = 0X00FFFFFF;

    // define Parameter
    public static final int PARA_NOT_EXIST = 0X00000001;
    public static final int PARA_EXIST = 0X00000001;
    public static final int PARA_ADD = 0X00000002;
    public static final int PARA_REMOVE = 0X00000004;
    public static final int PARA_ERROR = 0X00000008;

    // define process result
    public static final int PROC_PARA_ERROR = 0X00000000;
    public static final int PROC_CONTINUE = 0X00000001;
    public static final int PROC_SKIP = 0X00000002;

    public static int GetDetailReson(int nRet) {
        return (nRet & NotifyStatus.DETAIL_MSAK);
    }

    public static boolean IsSucc(int nRet) {
        return JudgeStatus(nRet, NotifyStatus.SUCC);
    }

    public static boolean IsFailure(int nRet) {
        return JudgeStatus(nRet, NotifyStatus.FAILURE);
    }

    private static boolean JudgeStatus(int nRet, int nRefer) {
        return ((nRet & nRefer) == nRefer);
    }

    public String Debug(int nValue) {
        return "";
    }
}
