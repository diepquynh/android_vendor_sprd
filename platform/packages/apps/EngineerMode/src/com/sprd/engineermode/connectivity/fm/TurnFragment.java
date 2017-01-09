//
//package com.sprd.engineermode.connectivity.fm;
//
//import java.io.File;
//import java.io.FileNotFoundException;
//import java.io.FileOutputStream;
//import java.io.FileReader;
//import java.io.InputStream;
//import java.io.OutputStream;
//import java.io.PrintStream;
//import java.nio.charset.StandardCharsets;
//import java.util.ArrayList;
//import java.util.Arrays;
//import java.util.Collection;
//import java.util.Collections;
//import java.util.HashMap;
//import java.util.List;
//
//import android.net.LocalSocket;
//import android.net.LocalSocketAddress;
//import android.net.LocalSocketAddress.Namespace;
//import android.os.Bundle;
//import android.os.Handler;
//import android.os.HandlerThread;
//import android.os.Message;
//import android.util.Log;
//import android.view.KeyEvent;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.View.OnClickListener;
//import android.view.ViewGroup;
//import android.widget.BaseAdapter;
//import android.widget.Button;
//import android.widget.EditText;
//import android.widget.ListView;
//import android.widget.TextView;
//import android.widget.Toast;
//
//import com.sprd.engineermode.R;
//
//import android.R.integer;
//import android.app.ProgressDialog;
//import android.content.Context;
//import android.content.DialogInterface;
//import android.content.DialogInterface.OnKeyListener;
//import android.hardware.fm.FmManager;
//import android.hardware.fm.FmConsts.*;
////import com.sprd.engineermode.connectivity.FM.FmManager;
////import com.sprd.engineermode.connectivity.FM.FmConstants.*;
//import android.os.SystemProperties;
//
//public class TurnFragment extends AbsFMFragment implements OnClickListener {
//
//    private static final String TAG = "TurnFragment";
//    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
//
//    private Button mTurnChannelButton;
//    private Button seekAllChannelRssiButton;
//    private EditText mTurnChannelEditText;
//    private ListView mSeekAllChannelRssiListView;
//    private SeekAllChannelRssiAdapter mSeekAllChannelRssiAdapter;
//    private ProgressDialog mSeekAllChannelProgressDialog;
//
//    private LocalSocket mSocketClient;
//    private LocalSocketAddress mSocketAddress;
//
//    private FmManager mFmManager;
//
//    private long mOperind;
//    private long mStereoind;
//    private long mRssi;
//    private long mFreqvalus;
//    private long mPwrindicator;
//    private long mFreqoffset;
//    private long mPilotdet;
//    private long mNodaclpf;
//
//    private TextView mOperIndTextview;
//    private TextView mStereIndTextview;
//    private TextView mRssiTextview;
//    private TextView mFreqValueTextview;
//    private TextView mPwrIndicatorTextview;
//    private TextView mFreqOffsetTextview;
//    private TextView mPilotDetTextview;
//    private TextView mNoDacLpfTextview;
//
//    private HashMap<Integer, Integer> mSeekAllChannelRssiHashMap = new HashMap<Integer, Integer>();
//
//    private List<Integer> mTempList;
//
//    private OutputStream mOutputStream;
//    private InputStream mInputStream;
//
//    private HandlerThread mSeekAllChannelRssiWorkHandlerThread;
//    private Handler mSeekAllChannelRssiWorkHandler;
//
//    private Handler mSeekAllChannelRssiHandler = new Handler() {
//        public void handleMessage(android.os.Message msg) {
//            mSeekAllChannelRssiAdapter.notifyDataSetChanged();
//            writeFileForSeekAllChannelRssi("/data/temp/", "Seek_All_Channel_Rssi");
//            mSeekAllChannelProgressDialog.cancel();
//        };
//    };
//
//    public void initSeekAllChannelRssiWorkHandlerAndThread() {
//        mSeekAllChannelRssiWorkHandlerThread = new HandlerThread("work_thread");
//        mSeekAllChannelRssiWorkHandlerThread.start();
//        mSeekAllChannelRssiWorkHandler = new Handler(
//                mSeekAllChannelRssiWorkHandlerThread.getLooper()) {
//            @Override
//            public void handleMessage(Message msg) {
//                seekAllChannelRssi();
//                sortFrequency();
//                mSeekAllChannelRssiHandler.sendEmptyMessage(0);
//                super.handleMessage(msg);
//            }
//        };
//    }
//
//    public boolean writeFileForSeekAllChannelRssi(String path, String name) {
//        Log.d(TAG, "path->" + path);
//        boolean flag = true;
//        FileOutputStream out = null;
//        PrintStream p = null;
//        try {
//            makeRootDirectory(path);
//            out = new FileOutputStream(new File(path + name));
//            p = new PrintStream(out);
//            Integer[] tempArray = mSeekAllChannelRssiHashMap.keySet().toArray(
//                    new Integer[mSeekAllChannelRssiHashMap.keySet().size()]);
//            ArrayList<Integer> tempList = new ArrayList<Integer>(Arrays.asList(tempArray));
//            Collections.sort(tempList);
//            for (int i = 0; i < mSeekAllChannelRssiHashMap.size(); i++) {
//                p.print("Frequency = " + (tempList.get(i).floatValue() / 1000.0)
//                        + "  pwrIndicator = "
//                        + (mSeekAllChannelRssiHashMap.get(tempList.get(i)) - 512) + "\n");
//            }
//        } catch (Exception e) {
//            flag = false;
//            Log.d(TAG, "Write file error!!!");
//            e.printStackTrace();
//        } finally {
//            if (p != null) {
//                try {
//                    p.close();
//                } catch (Exception e2) {
//                    e2.printStackTrace();
//                }
//            }
//            if (out != null) {
//                try {
//                    out.close();
//                } catch (Exception e2) {
//                    e2.printStackTrace();
//                }
//            }
//        }
//        return flag;
//    }
//
//    public static void makeRootDirectory(String filePath) {
//        File file = null;
//        try {
//            file = new File(filePath);
//            if (!file.exists()) {
//                file.mkdir();
//            }
//        } catch (Exception e) {
//
//        }
//    }
//
//    @Override
//    public void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
//        initFmManager();
//        SystemProperties.set("persist.sys.cmdservice.enable", "enable");
//        String status = SystemProperties.get("persist.sys.cmdservice.enable", "");
//        android.util.Log.d(TAG, "status = " + status);
//        try {
//            Thread.sleep(100);
//        } catch (Exception e) {
//            e.printStackTrace();
//        }
//        connectSocket("cmd_skt", LocalSocketAddress.Namespace.ABSTRACT);
//        initSeekAllChannelRssiWorkHandlerAndThread();
//    }
//
//    @Override
//    public void onDestroy() {
//        SystemProperties.set("persist.sys.cmdservice.enable", "disable");
//        mFmManager.powerDown();
//        if (mSeekAllChannelRssiWorkHandlerThread != null) {
//            mSeekAllChannelRssiWorkHandlerThread.quit();
//        }
//        super.onDestroy();
//    }
//
//    @Override
//    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
//        return initView(inflater);
//    }
//
//    public void initFmManager() {
//        mFmManager = new FmManager(getActivity());
//    }
//
//    public View initView(LayoutInflater inflater) {
//        View view = inflater.inflate(R.layout.turn_fragment, null);
//        mTurnChannelButton = (Button) view.findViewById(R.id.turn_channel_button);
//        mTurnChannelButton.setOnClickListener(this);
//        mTurnChannelEditText = (EditText) view.findViewById(R.id.turn_channnel_edittext);
//
//        mOperIndTextview = (TextView) view.findViewById(R.id.nOperInd_textview);
//        mStereIndTextview = (TextView) view.findViewById(R.id.nStereoInd_textview);
//        mRssiTextview = (TextView) view.findViewById(R.id.nRssi_textview);
//        mFreqValueTextview = (TextView) view.findViewById(R.id.nFreqValue_textview);
//        mPwrIndicatorTextview = (TextView) view.findViewById(R.id.nPwrIndicator_textview);
//        mFreqOffsetTextview = (TextView) view.findViewById(R.id.nFreqOffset_textview);
//        mPilotDetTextview = (TextView) view.findViewById(R.id.nPilotDet_textview);
//        mNoDacLpfTextview = (TextView) view.findViewById(R.id.nNoDacLpf_textview);
//
//        seekAllChannelRssiButton = (Button) view.findViewById(R.id.seek_all_channel_rssi_button);
//        seekAllChannelRssiButton.setOnClickListener(this);
//        mSeekAllChannelRssiListView = (ListView) view.findViewById(R.id.seek_all_channel_rssi_list);
//        mSeekAllChannelRssiAdapter = new SeekAllChannelRssiAdapter(getActivity());
//        mSeekAllChannelRssiListView.setAdapter(mSeekAllChannelRssiAdapter);
//
//        mSeekAllChannelProgressDialog = new ProgressDialog(getActivity());
//        mSeekAllChannelProgressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
//        mSeekAllChannelProgressDialog.setMessage(getResources().getString(
//                R.string.seek_all_channel_rssi));
//        mSeekAllChannelProgressDialog.setIndeterminate(false);
//        mSeekAllChannelProgressDialog.setCancelable(false);
//        mSeekAllChannelProgressDialog.setOnKeyListener(new OnKeyListener() {
//            public boolean onKey(DialogInterface dialoge, int keyCode,
//                    KeyEvent event) {
//                if (KeyEvent.KEYCODE_SEARCH == keyCode || KeyEvent.KEYCODE_HOME == keyCode) {
//                    return true;
//                }
//                return false;
//            }
//        });
//        return view;
//    }
//
//    public boolean isTurnChannelEditTextEmpty() {
//        if (mTurnChannelEditText.getText().toString().equals("")) {
//            return true;
//        } else {
//            return false;
//        }
//    }
//
//    @Override
//    public void onClick(View view) {
//        switch (view.getId()) {
//            case R.id.seek_all_channel_rssi_button:
//                if (!mFmManager.isFmOn()) {
//                    mFmManager.powerUp();
//                }
//                mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_HEADSET);
//                mSeekAllChannelProgressDialog.show();
//                mSeekAllChannelRssiWorkHandler.sendEmptyMessage(0);
//                break;
//            case R.id.turn_channel_button:
//                if (!isHeadsetExists()) {
//                    Toast.makeText(getActivity(),
//                            getActivity().getResources().getString(R.string.check_earphone),
//                            Toast.LENGTH_LONG).show();
//                    return;
//                }
//                if (isTurnChannelEditTextEmpty()) {
//                    Toast.makeText(
//                            getActivity(),
//                            getActivity().getResources().getString(R.string.turn_channel_edit_text),
//                            Toast.LENGTH_LONG).show();
//                    return;
//                }
//                TurnChannelInfo();
//                break;
//        }
//    }
//
//    private boolean isHeadsetExists() {
//        char[] buffer = new char[1024];
//        int newState = 0;
//        FileReader file = null;
//        try {
//            file = new FileReader(HEADSET_STATE_PATH);
//            int len = file.read(buffer, 0, 1024);
//            newState = Integer.valueOf((new String(buffer, 0, len)).trim());
//        } catch (FileNotFoundException e) {
//            Log.e("FMTest", "This kernel does not have wired headset support");
//        } catch (Exception e) {
//            Log.e("FMTest", "", e);
//        } finally {
//            if (file != null) {
//                try {
//                    file.close();
//                } catch (Exception e) {
//                    e.printStackTrace();
//                }
//            }
//        }
//        return newState != 0;
//    }
//
//    public void TurnChannelInfo() {
//        turnChannel();
//
//        mOperind = getOperInd("0x402700B0");
//
//        mStereoind = getStereoInd("0x402700B0");
//
//        mRssi = getRssi("0x402700B0");
//
//        mFreqvalus = getFreqValue("0x402700B8");
//
//        mPwrindicator = getPwrIndicator("0x402700C0");
//
//        mFreqoffset = getFreqOffset("0x402700BC");
//
//        mPilotdet = getPilotDet("0x402700B0");
//
//        mNodaclpf = getNoDacLpf("0x402700C8");
//
//        android.util.Log.d(TAG,
//                "  operind = " + mOperind +
//                        "  stereoind = " + mStereoind +
//                        "  rssi = " + mRssi +
//                        "  freqvalus = " + mFreqvalus +
//                        "  pwrindicator = " + (mPwrindicator - 512) +
//                        "  freqoffset = " + mFreqoffset +
//                        "  pilotdet = " + mPilotdet +
//                        "  nodaclpf = " + mNodaclpf);
//
//        setTurnInfoValues();
//    }
//
//    public void setTurnInfoValues() {
//        mOperIndTextview.setText(getResources().getString(R.string.nOperInd, mOperind + ""));
//        mStereIndTextview.setText(getResources().getString(R.string.nStereoInd, mStereoind + ""));
//        mRssiTextview.setText(getResources().getString(R.string.nRssi, mRssi + ""));
//        mFreqValueTextview.setText(getResources().getString(R.string.nFreqValue, mFreqvalus + ""));
//        mPwrIndicatorTextview.setText(getResources().getString(R.string.nPwrIndicator,
//                (mPwrindicator - 512) + ""));
//        mFreqOffsetTextview.setText(getResources()
//                .getString(R.string.nFreqOffset, mFreqoffset + ""));
//        mPilotDetTextview.setText(getResources().getString(R.string.nPilotDet, mPilotdet + ""));
//        mNoDacLpfTextview.setText(getResources().getString(R.string.nNoDacLpf, mNodaclpf + ""));
//    }
//
//    public long getOperInd(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0x10000) >> 16;
//        android.util.Log.d(TAG, "result 0x402700B0 = " + result);
//        return result;
//    }
//
//    public long getStereoInd(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0x20000) >> 17;
//        android.util.Log.d(TAG, "result 0x402700B0 = " + result);
//        return result;
//    }
//
//    public long getRssi(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0xFF);
//        android.util.Log.d(TAG, "result 0x402700B0 = " + result);
//        return result;
//    }
//
//    public long getFreqValue(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0xFFFF);
//        android.util.Log.d(TAG, "result 0x402700B8 = " + result);
//        return result;
//    }
//
//    public long getPwrIndicator(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0x1FF);
//        android.util.Log.d(TAG, "result 0x402700C0 = " + result);
//        return result;
//    }
//
//    public long getFreqOffset(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0xFFFF);
//        android.util.Log.d(TAG, "result 0x402700BC = " + result);
//        return result;
//    }
//
//    public long getPilotDet(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = praseTemp >> 18;
//        android.util.Log.d(TAG, "result 0x402700B0 = " + result);
//        return result;
//    }
//
//    public long getNoDacLpf(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
//        long praseTemp = Long.parseLong(tempStringArray[0].replace("0x", "").trim(), 16);
//        long result = (praseTemp & 0x3);
//        android.util.Log.d(TAG, "result 0x402700C8 = " + result);
//        return result;
//    }
//
//    public void turnChannel() {
//        if (mFmManager.isFmOn()) {
//            mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_HEADSET);
//            mFmManager
//                    .setFreq((int) (Float.parseFloat(mTurnChannelEditText.getText().toString()) * 1000));
//        } else {
//            mFmManager.powerUp();
//            mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_HEADSET);
//            mFmManager
//                    .setFreq((int) (Float.parseFloat(mTurnChannelEditText.getText().toString()) * 1000));
//        }
//    }
//
//    private void connectSocket(String socketName, Namespace namespace) {
//        try {
//            mSocketClient = new LocalSocket();
//            mSocketAddress = new LocalSocketAddress(socketName, namespace);
//            mSocketClient.connect(mSocketAddress);
//            Log.d(TAG, "mSocketClient connect is " + mSocketClient.isConnected());
//        } catch (Exception e) {
//            Log.d(TAG, "mSocketClient connect is false");
//            e.printStackTrace();
//        }
//    }
//
//    private String sendCmdAndResult(String cmd) {
//        byte[] buffer = new byte[1024];
//        String result = null;
//        try {
//            mOutputStream = mSocketClient.getOutputStream();
//            if (mOutputStream != null) {
//                final StringBuilder cmdBuilder = new StringBuilder(cmd).append('\0');
//                final String cmmand = cmdBuilder.toString();
//                mOutputStream.write(cmmand.getBytes(StandardCharsets.UTF_8));
//                mOutputStream.flush();
//            }
//            mInputStream = mSocketClient.getInputStream();
//            mInputStream.read(buffer, 0, 1024);
//            result = new String(buffer, "utf-8");
//            Log.d(TAG, "result is " + result);
//        } catch (Exception e) {
//            Log.d(TAG, "Failed get outputStream: " + e);
//            e.printStackTrace();
//        }
//        return result;
//    }
//
//    public void seekAllChannelRssi() {
//        float endChannel = 108000;
//        int rssi = 0;
//        for (int startChannel = 87500; startChannel <= endChannel; startChannel = startChannel + 100) {
//            mFmManager.setFreq(startChannel);
//            try {
//                Thread.sleep(50);
//            } catch (Exception e) {
//                e.printStackTrace();
//            }
//            rssi = mFmManager.getRssi();
//            android.util.Log.d(TAG, "pwrindicator = " + rssi);
//            mSeekAllChannelRssiHashMap.put(new Integer(startChannel), new Integer(rssi));
//        }
//    }
//
//    public void sortFrequency() {
//        Integer[] keyset = mSeekAllChannelRssiHashMap.keySet().toArray(
//                new Integer[mSeekAllChannelRssiHashMap.keySet().size()]);
//
//        mTempList = new ArrayList<Integer>(Arrays.asList(keyset));
//
//        Collections.sort(mTempList);
//    }
//
//    class SeekAllChannelRssiAdapter extends BaseAdapter {
//
//        private final Context mContext;
//        private final LayoutInflater mInflater;
//
//        public SeekAllChannelRssiAdapter(Context context) {
//            mContext = context;
//            mInflater = LayoutInflater.from(mContext);
//        }
//
//        @Override
//        public int getCount() {
//            return mSeekAllChannelRssiHashMap.size();
//        }
//
//        @Override
//        public Object getItem(int position) {
//            return null;
//        }
//
//        @Override
//        public long getItemId(int position) {
//            return position;
//        }
//
//        @Override
//        public View getView(int position, View convertView, ViewGroup parent) {
//            ViewHolder viewHolder;
//            if (convertView == null) {
//                convertView = mInflater.inflate(
//                        getResources().getLayout(R.layout.seek_all_channel_rssi_adapter), null);
//                viewHolder = new ViewHolder();
//                viewHolder.mShowChannelAndRssiTextView = (TextView) convertView
//                        .findViewById(R.id.frequency_and_rssi);
//                convertView.setTag(viewHolder);
//            } else {
//                viewHolder = (ViewHolder) convertView.getTag();
//            }
//
//            String frequencyAndCounts = mContext.getResources().getString(
//                    R.string.seel_all_channel_frequency_adapter_frequency_and_rssi,
//                    (mTempList.get(position).floatValue() / 1000.0) + "",
//                    (mSeekAllChannelRssiHashMap.get(mTempList.get(position)) - 512) + "");
//            android.util.Log.d(TAG, "Freq = " + (mTempList.get(position).floatValue() / 1000.0)
//                    + "  inputpower = "
//                    + (mSeekAllChannelRssiHashMap.get(mTempList.get(position)) - 512));
//            viewHolder.mShowChannelAndRssiTextView.setText(frequencyAndCounts);
//
//            return convertView;
//        }
//
//        public class ViewHolder {
//            TextView mShowChannelAndRssiTextView;
//        }
//    }
//}
