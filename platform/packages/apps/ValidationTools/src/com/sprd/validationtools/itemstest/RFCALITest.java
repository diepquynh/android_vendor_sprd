
package com.sprd.validationtools.itemstest;

import android.telephony.TelephonyManager;
import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.IATUtils;
import com.sprd.validationtools.R;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import android.os.Bundle;
import android.os.Handler;

import android.util.Log;
import android.widget.TextView;
import android.os.SystemProperties;
import com.sprd.validationtools.TelephonyManagerSprd;

public class RFCALITest extends BaseActivity {

    private static final String TAG = "RFCALITest";
    //This is only for 9620
    private static final String ADC_PATH = "/productinfo/adc.bin";

    private String str = "loading...";
    private TextView txtViewlabel01;
    private Handler mUiHandler = new Handler();
    private DataInputStream mInputStream=null;
    private static final int ADCBYTES = 56;
    byte[] buffer = new byte[ADCBYTES];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.rf_cali_test);
        setTitle(R.string.rf_cali_test);
        txtViewlabel01 = (TextView) findViewById(R.id.rfc_id);
        txtViewlabel01.setTextSize(18);
        txtViewlabel01.setText(str);

        initial();
    }

    private void initial() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                int modemType;
                try{
                    modemType = TelephonyManagerSprd.getModemType();
                }catch (Exception e){
                    modemType = 1;
                }
                if (modemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA
                		|| TelephonyManagerSprd.getRadioCapbility() == TelephonyManagerSprd.RadioCapbility.TDD_CSFB 
                		|| TelephonyManagerSprd.getRadioCapbility() == TelephonyManagerSprd.RadioCapbility.CSFB ) {
                    str = "GSM/TD ";
                } else {
                    str = "GSM ";
                }
                //modify 319936 by sprd 
                //read /proc/cmdline, if contains "adc_cal",ADC Calibration pass
                //this is only for 9620
                if (SystemProperties.get("ro.product.board").contains("9620")) {
                    Log.d(TAG,"This product is 9620");
                    String str1 = IATUtils.sendATCmd("AT+SGMR=0,0,3,0", "atchannel0");
                    boolean isAdc = readFile();
                    String[] str3 = str1.split("\n");
                    for (int i = 0;i < str3.length;i++) {
                        if (str3[i].contains("ADC")) {
                            Log.d(TAG,"9620 has calibration bit");
                            String[] str4 = str3[i].split("\\:");
                            if (isAdc) {
                                str3[i] = str4[0]+":ADC calibrated Pass";
                            }
                        }
                        str += str3[i]+"\n";
                    }                   
                } else {
                    Log.d(TAG,"This product is not 9620");
                    str += IATUtils.sendATCmd("AT+SGMR=0,0,3,0", "atchannel0");
                }
               
                if (modemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA 
                		|| TelephonyManagerSprd.getRadioCapbility() == TelephonyManagerSprd.RadioCapbility.FDD_CSFB
                		|| TelephonyManagerSprd.getRadioCapbility() == TelephonyManagerSprd.RadioCapbility.CSFB) {
                    str += "WCDMA ";
                    str += IATUtils.sendATCmd("AT+SGMR=0,0,3,1", "atchannel0");
                }
                if(modemType == TelephonyManagerSprd.MODEM_TYPE_LTE) {
                    str += "LTE ";
                    str += IATUtils.sendATCmd("AT+SGMR=1,0,3,3", "atchannel0");
                }
                mUiHandler.post(new Runnable() {
                    public void run() {
                        txtViewlabel01.setText(str);
                    }
                });
            }
        }).start();
    }

    public boolean readFile() {
        try {
            File adcFile = new File(ADC_PATH);
            int count = 0;
            if (!adcFile.exists()) {
                Log.d(TAG, "adcFile do not exists");
                return false;
            }
            mInputStream = new DataInputStream(new FileInputStream(adcFile));
            if (mInputStream != null) {
                count = mInputStream.read(buffer, 0, ADCBYTES);
            }
            if (buffer == null || buffer.length <= 0) {
                Log.d(TAG, "buffer == null or buffer.length <= 0");
                return false;
            }
            Log.d(TAG, "count = " + count + " size = " + buffer.length);
            int adcBit = buffer.length - 4;
            int adcResult = buffer[adcBit] | 0xFFFFFFFC;
            Log.d(TAG, "adcBit = " + adcBit + " buffer[" + adcBit + "] = 0x"
                    + Integer.toHexString(buffer[adcBit]) + " adcResult = 0x"
                    + Integer.toHexString(adcResult));
            if (adcResult == 0xFFFFFFFF) {
                return true;
            } else {
                return false;
            }
        } catch (Exception e) {
            Log.d(TAG, "Failed get outputStream: " + e);
            e.printStackTrace();
        }
        return false;
    }  
    
    //@Override
    //public void onBackPressed() {
     //   storeRusult(true);
      //  finish();
    //}
    @Override	
    protected void onDestroy() {
        try {
            if (mInputStream != null) {
                mInputStream.close();
            }

        } catch (IOException e) {
            e.printStackTrace();
        }

        super.onDestroy();
    }
}
