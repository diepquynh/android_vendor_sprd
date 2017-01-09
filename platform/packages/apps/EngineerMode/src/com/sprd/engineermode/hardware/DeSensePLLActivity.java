
package com.sprd.engineermode.hardware;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.SocketUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.LocalSocketAddress.Namespace;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.os.SystemProperties;

public class DeSensePLLActivity extends Activity {

    private static final String TAG = "DeSensePLLActivity/Activity";
    private static final String CMD_ERROR = "cmd_error";

    private EditText deSensePLLAddress = null;
    private EditText deSensePLLData = null;
    private EditText deSensePLLNumber = null;
    private TextView deSensePLLResult = null;
    private Button mRead = null;
    private Button mWrite = null;
    private int count = 0;

    private LocalSocket mSocketClient = null;
    private LocalSocketAddress mSocketAddress = null;
    private OutputStream mOutputStream = null;
    private InputStream mInputStream = null;
    private boolean isConnected = false;
    private boolean isSuccess = true;
    private ButListener mButListener = new ButListener();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.de_sense_pll);

        //start cmd_services to crate locak socket
        SystemProperties.set("persist.sys.cmdservice.enable", "enable");

        deSensePLLAddress = (EditText) this.findViewById(R.id.de_sense_pll_address);
        deSensePLLData = (EditText) this.findViewById(R.id.de_sense_pll_data);
        deSensePLLNumber = (EditText) this.findViewById(R.id.de_sense_pll_number);
        deSensePLLResult = (TextView) this.findViewById(R.id.de_sense_pll_result);
        mRead = (Button) this.findViewById(R.id.de_sense_pll_read);
        mWrite = (Button) this.findViewById(R.id.de_sense_pll_write);
        mRead.setOnClickListener(mButListener);
        mWrite.setOnClickListener(mButListener);

        String status = SystemProperties.get("persist.sys.cmdservice.enable", "");
        Log.d(TAG, "status:" + status);

        //wait 100ms for cmd_service starting
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        connectSocket("cmd_skt", LocalSocketAddress.Namespace.ABSTRACT);
        new AlertDialog.Builder(this)
        .setTitle("Info")
        .setMessage(this.getString(R.string.desense_pll_tips))
        .setPositiveButton(this.getString(R.string.alertdialog_ok), null)
        .show();
    }

    @Override
    protected void onDestroy() {
        try {
            if (mInputStream != null) {
                mInputStream.close();
            }
            if (mOutputStream != null) {
                mOutputStream.close();
            }
            if (mSocketClient != null) {
                mSocketClient.close();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        SystemProperties.set("persist.sys.cmdservice.enable", "disable");
        String disable = SystemProperties.get("persist.sys.cmdservice.enable", "");
        Log.d(TAG, "disable:" + disable);

        super.onDestroy();
    }

    private class ButListener implements OnClickListener {

        @Override
        public void onClick(View view) {
            if (view.equals(mRead)) {
                if (isConnected) {
                    String result = "";
                    int line = 0;
                    String data = "";
                    String address = deSensePLLAddress.getText().toString();
                    String number = deSensePLLNumber.getText().toString();
                    if (address.length() < 8 ) {
                        Toast.makeText(DeSensePLLActivity.this, "Please input 8 bit Address",
                                Toast.LENGTH_SHORT).show();
                        return;
                    }
                    if (0 == number.length()) {
                        Toast.makeText(DeSensePLLActivity.this, "Please input Number",
                                Toast.LENGTH_SHORT).show();
                        return;
                    }

                    String readCmd = "lookat -l " + number + " 0x" + address;
                    Log.d(TAG, "readCmd->" + readCmd);

                    String readPLL = sendCmdAndResult(readCmd);
                    
                    /**
                     * EXAMPLE
                     * cmd:lookat -l 8 0x82000030
                     * result:
                     *   ADDRESS  |   VALUE
                       -----------+-----------
                       0x82000030 | 0xffb5b49f
                       0x82000034 | 0xffb5b49f
                       0x82000038 | 0xffb5b49f
                       0x8200003c | 0xffb5b49f
                       0x82000040 | 0xffb5b49f
                       0x82000044 | 0xffb5b49f
                       0x82000048 | 0xffb5b49f
                       0x8200004c | 0xffb5b49f
                     */
                    
                    if (readPLL.contains("VALUE")) {
                        // SPRD: Bug 541622 java.lang.ArrayIndexOutOfBoundsException: length=1; index=1
                        result = readPLL;
                        Log.d(TAG, "result->" + result);

                    } else {
                        isSuccess = false;
                        String[] strError = readPLL.split("\n");
                        for (int i = 0; i < strError.length - 2; i++) {
                            result += strError[i] + "\n";
                        }
                        Log.d(TAG, "result->" + result);
                    }
                    if (isSuccess) {
                        deSensePLLData.setText(data);
                        deSensePLLResult.setText(result);
                    } else {
                        deSensePLLData.setText("");
                        deSensePLLResult.setText(result);
                        isSuccess = true;
                        Toast.makeText(DeSensePLLActivity.this, "Read Fail", Toast.LENGTH_SHORT).show();
                    }
                } else {
                    Toast.makeText(DeSensePLLActivity.this,
                            "mSocketClient connect is " + isConnected, Toast.LENGTH_SHORT).show();
                }
            }
            if (view.equals(mWrite)) {
                if (isConnected) {
                    String address = deSensePLLAddress.getText().toString();
                    String data = deSensePLLData.getText().toString();
                    if (address.length() < 8) {
                        Toast.makeText(DeSensePLLActivity.this, "Please input 8 bit Address",
                                Toast.LENGTH_SHORT).show();
                        return;
                    } else {
                        address = "0x" + address;
                    }
                    if (0 == data.length()) {
                        Toast.makeText(DeSensePLLActivity.this, "Please input Data",
                                Toast.LENGTH_SHORT).show();
                        return;
                    } else {
                        data = "0x" + data;
                    }

                    String writeCmd = "lookat -s " + data + " " + address;
                    Log.d(TAG, "writeCmd->" + writeCmd);

                    String result = null;
                    result = sendCmdAndResult(writeCmd);
                    Log.d(TAG, "write result->" + result);
                    // SPRD: Bug 541622 java.lang.ArrayIndexOutOfBoundsException: length=1; index=1
                    if (result != null && result.trim().contains("Result")) {
                        String writeSucc = sendCmdAndResult("lookat " + address);
                        String[] str = writeSucc.split("\n");
                        deSensePLLResult.setText("");
                        Toast.makeText(DeSensePLLActivity.this,
                                "Success,Address:" + address + ",Data:" + str[0],
                                Toast.LENGTH_SHORT).show();
                    } else {
                        String[] str = result.split("\n");
                        deSensePLLResult.setText(str[0]);
                        Toast.makeText(DeSensePLLActivity.this, "Write Fail", Toast.LENGTH_SHORT).show();
                    }
                } else {
                    Toast.makeText(DeSensePLLActivity.this,
                            "mSocketClient connect is " + isConnected, Toast.LENGTH_SHORT).show();
                }
            }
        }

    }

    private void connectSocket(String socketName, Namespace namespace) {
        try {
            mSocketClient = new LocalSocket();
            mSocketAddress = new LocalSocketAddress(socketName, namespace);
            mSocketClient.connect(mSocketAddress);
            isConnected = true;
            Log.d(TAG, "mSocketClient connect is " + mSocketClient.isConnected());
        } catch (Exception e) {
            isConnected = false;
            Log.d(TAG, "mSocketClient connect is false");
            e.printStackTrace();
        }
    }

    private String sendCmdAndResult(String cmd) {
        byte[] buffer = new byte[1024];
        String result = CMD_ERROR;
        try {
            mOutputStream = mSocketClient.getOutputStream();
            if (mOutputStream != null) {
                final StringBuilder cmdBuilder = new StringBuilder(cmd).append('\0');
                final String cmmand = cmdBuilder.toString();
                mOutputStream.write(cmmand.getBytes(StandardCharsets.UTF_8));
                mOutputStream.flush();
            }
            mInputStream = mSocketClient.getInputStream();
            count = mInputStream.read(buffer, 0, 1024);
            result = new String(buffer, "utf-8");
            Log.d(TAG, "count is " + count + ",result is " + result);
        } catch (Exception e) {
            Log.d(TAG, "Failed get outputStream: " + e);
            e.printStackTrace();
        }
        return result;
    }
}
