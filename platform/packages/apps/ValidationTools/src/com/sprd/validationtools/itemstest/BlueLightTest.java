
package com.sprd.validationtools.itemstest;

import android.graphics.Color;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.ServiceManager;
import android.util.Log;
import android.view.Gravity;
import android.widget.TextView;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

public class BlueLightTest extends BaseActivity {
    private static final String TAG = "BlueLightTest";
    private TextView mContent;
    private IBinder mBinder;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContent = new TextView(this);
        setBackground();
        setContentView(mContent);
        mContent.setGravity(Gravity.CENTER);
        mContent.setTextSize(35);
        mBinder = ServiceManager.getService("phasechecknative");
        if (mBinder != null)
            Log.e(TAG, "Get The service connect!");
        else
            Log.e(TAG, "connect Error!!");
        writeLedlightSwitch(1);
    }

    private void setBackground() {
        mContent.setBackgroundColor(Color.BLUE);
        mContent.setText(getString(R.string.status_indicator_blue));
    }

    public boolean writeLedlightSwitch(int value) {
        try {
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInt(value);
            mBinder.transact(8, data, reply, 0);
            Log.e(TAG, "writeLedlightSwitch blue light data = " + reply.readString() + " SUCESS!!");
            data.recycle();
            return true;
        } catch (Exception ex) {
            Log.e(TAG, "Exception ", ex);
            return false;
        }
    }

    @Override
    protected void onDestroy() {
        writeLedlightSwitch(0);
        super.onDestroy();
    }

}
