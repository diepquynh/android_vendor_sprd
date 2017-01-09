package com.android.internal.telephony.cat;

public class CallControlResult{
    static public int RESULT_ALLOWED =0;
    static public int RESULT_NOT_ALLOWED = 1;
    static public int RESULT_ALLOWED_MODIFIED = 2;

    public int call_type =0;
    public int result = 0;
    public int is_alpha = 0;
    public int alpha_len = 0;
    public String alpha_data =null;
    public int pre_type;
    public int ton;
    public int npi;
    public int num_len;
    public String number = null;

    @Override
    public String toString() {
        return super.toString() + "STK Call Control Result "
            + " call_type:" + call_type
            + " result: " + result
            + " is_alpha: " + is_alpha
            + " alpha_len: " + alpha_len
            + " alpha_data: " + alpha_data
            + " pre_type: " + pre_type
            + " ton: " + ton
            + " npi: " + npi
            + " num_len: " + num_len
            + " number: " + number;

    }
}
