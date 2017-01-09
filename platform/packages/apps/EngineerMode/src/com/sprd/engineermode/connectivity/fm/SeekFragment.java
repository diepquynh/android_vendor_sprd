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
//import java.util.Collections;
//import java.util.Comparator;
//import java.util.HashMap;
//import java.util.Iterator;
//import java.util.List;
//import java.util.Map;
//import java.util.zip.Inflater;
//
//import android.R.integer;
//import android.R.string;
//import android.app.ProgressDialog;
//import android.content.Context;
//import android.content.DialogInterface;
//import android.content.DialogInterface.OnKeyListener;
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
//import android.widget.ArrayAdapter;
//import android.widget.BaseAdapter;
//import android.widget.Button;
//import android.widget.EditText;
//import android.widget.ListView;
//import android.widget.TextView;
//import android.widget.Toast;
//
//import com.sprd.engineermode.R;
//import com.sprd.engineermode.connectivity.fm.FmManagerSelect;
//import com.sprd.engineermode.connectivity.fm.FmConstants.*;
//
//import android.os.SystemProperties;
//import android.os.Looper;
//
//public class SeekFragment extends AbsFMFragment implements OnClickListener {
//    private static final int HEADSET_DIALOG = 1;
//    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
//    private static final String TAG = "SeekFragment";
//    private static final String HANDLER_THREAD_NAME = "WriteFile";
//    private static final int FM_SEARCH_TIMEOUT = 30000;
//    private int RADIO_MIN_FREQUENCY = 87500;
//    private int RADIO_MAX_FREQUENCY = 108000;
//    private Button mSeekAllChannelButton;
//    private ProgressDialog mSearchStationDialog = null;
//    private EditText mSeekAllChannelFrequencyEdittext;
//    private ListView mSeekAllChannelFrequencyListview;
//    private ListView mSeekAllChannelRegisterListView;
//
//    private SeekChannelRegister mSeekChannelRegisterAdapter;
//
//    private EditText mMinChannelEditText;
//    private EditText mMaxChannelEditText;
//    private Button mStartSeekbutton;
//
//    private String mMinChannel;
//    private String mMaxChannel;
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
//    private FmManagerSelect mFmManager;
//
//    private LocalSocket mSocketClient;
//    private LocalSocketAddress mSocketAddress;
//
//    private HashMap<Integer, ArrayList<Long>> mChannelRegisterInfo = new HashMap<Integer, ArrayList<Long>>();
//    private List<Integer> mTempChannelRegisterInfoList;
//
//    private boolean mNeedRefreshListView = true;
//    private SeekAllChannelFrequencyAdapter mSeekAllChannelFrequencyAdapter;
//    private List<Integer> mTempList;
//    private HashMap<Integer, Integer> mStatisticsCounts = new HashMap<Integer, Integer>();
//
//    private ArrayList<Integer> mStatisticsCountsArrayList = new ArrayList<Integer>();
//
//    private SearchSeekChannelThread mSearchSeekChannelThread;
//
//    private HandlerThread mWriteFileHandlerThread;
//    private Handler mWriteFileHandler;
//
//    private HandlerThread mSeekThread;
//    private Handler mSeekHandler;
//    private final int MSG_POWER_UP_COMPLETE = 1;
//    private final int MSG_START_POWER_UP = 0;
//
//    private final int MSG_FINSH_SEEK_DIALOG = 1;
//    private final int MSG_START_SEEK_DIALOG = 0;
//
//    private boolean mIsSeek = true;
//    private boolean mNextSeek = true;
//
//    private Handler mSeekAllChannelHandler = new Handler() {
//        public void handleMessage(android.os.Message msg) {
//            switch (msg.what) {
//                case 1:
//                    Integer[] keyset = mChannelRegisterInfo.keySet().toArray(
//                            new Integer[mChannelRegisterInfo.keySet().size()]);
//
//                    mTempChannelRegisterInfoList = new ArrayList<Integer>(Arrays.asList(keyset));
//
//                    Collections.sort(mTempChannelRegisterInfoList);
//                    mSeekChannelRegisterAdapter.notifyDataSetChanged();
//                    break;
//                case 0:
//                    if (!mNeedRefreshListView) {
//                        mStatisticsCountsArrayList.clear();
//                    }
//                    mSeekAllChannelFrequencyAdapter.notifyDataSetChanged();
//                    break;
//            }
//        };
//    };
//
//    public void initWriteFileHandlerThreadAndHandler() {
//        mWriteFileHandlerThread = new HandlerThread(HANDLER_THREAD_NAME);
//        mWriteFileHandlerThread.start();
//        mWriteFileHandler = new Handler(mWriteFileHandlerThread.getLooper()) {
//            @Override
//            public void handleMessage(Message msg) {
//                super.handleMessage(msg);
//                if (msg.what == 0) {
//                    writeFileForSeekChannel("/data/temp/", "Seek_Channel_Register_Info");
//                } else {
//                    writeFile(mStatisticsCountsArrayList, "/data/temp/", "Seek_Frequency_Counts");
//                }
//            }
//        };
//    }
//
//    @Override
//    public void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
//        initFmManager();
//        initWriteFileHandlerThreadAndHandler();
//        SystemProperties.set("persist.sys.cmdservice.enable", "enable");
//        String status = SystemProperties.get("persist.sys.cmdservice.enable", "");
//        android.util.Log.d(TAG, "status = " + status);
//        try {
//            Thread.sleep(100);
//        } catch (Exception e) {
//            e.printStackTrace();
//        }
//        connectSocket("cmd_skt", LocalSocketAddress.Namespace.ABSTRACT);
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
//    @Override
//    public void onDestroy() {
//        if (mWriteFileHandlerThread != null) {
//            mWriteFileHandlerThread.quit();
//        }
//        SystemProperties.set("persist.sys.cmdservice.enable", "disable");
//        destoryHandlerThread();
//        super.onDestroy();
//    }
//
//    private void initFmManager() {
//        mFmManager = new FmManagerSelect(getActivity());
//    }
//
//    @Override
//    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
//        return initView(inflater);
//    }
//
//    public View initView(LayoutInflater inflater) {
//        View view = inflater.inflate(R.layout.seek_fragment_main, null);
//
//        mSeekAllChannelRegisterListView = (ListView) view.findViewById(R.id.seek_channel_list);
//        mSeekChannelRegisterAdapter = new SeekChannelRegister(getActivity());
//        mSeekAllChannelRegisterListView.setAdapter(mSeekChannelRegisterAdapter);
//
//        mMinChannelEditText = (EditText) view.findViewById(R.id.min_channel);
//        mMaxChannelEditText = (EditText) view.findViewById(R.id.max_channel);
//        mStartSeekbutton = (Button) view.findViewById(R.id.seek);
//        mStartSeekbutton.setOnClickListener(this);
//
//        mSeekAllChannelFrequencyListview = (ListView) view
//                .findViewById(R.id.seek_all_channel_frequency_listview);
//        mSeekAllChannelFrequencyAdapter = new SeekAllChannelFrequencyAdapter(getActivity());
//        mSeekAllChannelFrequencyListview.setAdapter(mSeekAllChannelFrequencyAdapter);
//
//        mSeekAllChannelButton = (Button) view.findViewById(R.id.seek_button);
//        mSeekAllChannelButton.setOnClickListener(this);
//
//        mSeekAllChannelFrequencyEdittext = (EditText) view
//                .findViewById(R.id.seek_all_channel_frequency_edittext);
//
//        initProgressDialog();
//
//        return view;
//    }
//
//    public void initProgressDialog() {
//        mSearchStationDialog = new ProgressDialog(getActivity());
//        mSearchStationDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
//        mSearchStationDialog.setIndeterminate(false);
//        mSearchStationDialog.setCancelable(false);
//        mSearchStationDialog.setOnKeyListener(new OnKeyListener() {
//            public boolean onKey(DialogInterface dialoge, int keyCode,
//                    KeyEvent event) {
//                if (KeyEvent.KEYCODE_SEARCH == keyCode || KeyEvent.KEYCODE_HOME == keyCode) {
//                    return true;
//                }
//                return false;
//            }
//        });
//    }
//
//    @Override
//    public void onClick(View view) {
//        switch (view.getId()) {
//            case R.id.seek:
//                if (!decideNeedConditionForSeekChannel()) {
//                    return;
//                }
//                seekChannelInfo();
//                break;
//            case R.id.seek_button:
//                if (!decideNeedConditionForSeekAllChannel()) {
//                    return;
//                }
//                seekAllChannel(Integer.parseInt(mSeekAllChannelFrequencyEdittext.getText()
//                        .toString().trim()));
//                break;
//        }
//    }
//
//    public void seekChannelInfo() {
//        if (!mFmManager.isFmOn()) {
//            mFmManager.powerUp();
//        }
//        mFmManager.setAudioPath(AudioPath.FM_AUDIO_PATH_HEADSET);
//        startSeekChannelStation();
//    }
//
//    private void startSeekChannelStation() {
//        mSearchSeekChannelThread = new SearchSeekChannelThread();
//        if (!mSearchSeekChannelThread.isAlive() && !mSearchSeekChannelThread.isRunning()) {
//            if (!mSearchStationDialog.isShowing()) {
//                mSearchStationDialog
//                        .setMessage(getResources().getString(R.string.seek_channel_tip));
//                mSearchStationDialog.setButton(getResources().getString(R.string.fm_cancel),
//                        new DialogInterface.OnClickListener() {
//
//                            @Override
//                            public void onClick(DialogInterface dialog, int which) {
//                                mFmManager.cancelSearch();
//                                mSearchSeekChannelThread.interrupt();
//                                mFmManager.powerDown();
//                                mSearchStationDialog.cancel();
//                            }
//                        });
//                mSearchStationDialog.show();
//            }
//            mSearchSeekChannelThread.start();
//        }
//    }
//
//    class SearchSeekChannelThread extends Thread {
//        boolean isRunning = false;
//
//        public boolean isRunning() {
//            return isRunning;
//        }
//
//        public void run() {
//            isRunning = true;
//            searchStationForSeekChannel();
//            mSeekAllChannelHandler.sendEmptyMessage(1);
//            mFmManager.powerDown();
//            mSearchStationDialog.cancel();
//            mWriteFileHandler.sendEmptyMessage(0);
//            isRunning = false;
//        };
//    };
//
//    private class SeekChannelRegister extends BaseAdapter {
//
//        public LayoutInflater mInflater;
//
//        public SeekChannelRegister(Context context) {
//            mInflater = LayoutInflater.from(context);
//        }
//
//        @Override
//        public int getCount() {
//            return mChannelRegisterInfo.size();
//        }
//
//        @Override
//        public Object getItem(int position) {
//            return position;
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
//                convertView = mInflater.inflate(R.layout.seek_channel_register_adapter, null);
//                viewHolder = new ViewHolder();
//                viewHolder.channel = (TextView) convertView.findViewById(R.id.channel);
//                viewHolder.nOperInd = (TextView) convertView.findViewById(R.id.operind);
//                viewHolder.nStereoInd = (TextView) convertView.findViewById(R.id.stereoind);
//                viewHolder.nRssi = (TextView) convertView.findViewById(R.id.rssi);
//                viewHolder.nFreqValue = (TextView) convertView.findViewById(R.id.freqvalus);
//                viewHolder.nPwrIndicator = (TextView) convertView.findViewById(R.id.pwrindicator);
//                viewHolder.nFreqOffset = (TextView) convertView.findViewById(R.id.freqoffset);
//                viewHolder.nPilotDet = (TextView) convertView.findViewById(R.id.pilotdet);
//                viewHolder.nNoDacLpf = (TextView) convertView.findViewById(R.id.Nodaclpf);
//                convertView.setTag(viewHolder);
//            } else {
//                viewHolder = (ViewHolder) convertView.getTag();
//            }
//
//            setRegisterText(viewHolder, position);
//
//            return convertView;
//        }
//
//        public void setRegisterText(ViewHolder viewHolder, int position) {
//            viewHolder.channel
//                    .setText((mTempChannelRegisterInfoList.get(position).floatValue() / 1000.0)
//                            + "");
//
//            ArrayList<Long> tempValueSet = mChannelRegisterInfo.get(mTempChannelRegisterInfoList
//                    .get(position));
//
//            viewHolder.nOperInd
//                    .setText(getString(R.string.nOperInd, tempValueSet.get(0).toString()));
//            viewHolder.nStereoInd.setText(getString(R.string.nStereoInd, tempValueSet.get(1)
//                    .toString()));
//            viewHolder.nRssi.setText(getString(R.string.nRssi, tempValueSet.get(2).toString()));
//            viewHolder.nFreqValue.setText(getString(R.string.nFreqValue, tempValueSet.get(3)
//                    .toString()));
//            viewHolder.nPwrIndicator.setText(getString(R.string.nPwrIndicator,
//                    (tempValueSet.get(4) - 512)
//                            + ""));
//            viewHolder.nFreqOffset.setText(getString(R.string.nFreqOffset, tempValueSet.get(5)
//                    .toString()));
//            viewHolder.nPilotDet.setText(getString(R.string.nPilotDet, tempValueSet.get(6)
//                    .toString()));
//            viewHolder.nNoDacLpf.setText(getString(R.string.nNoDacLpf, tempValueSet.get(7)
//                    .toString()));
//        }
//
//        public class ViewHolder {
//            public TextView channel;
//            public TextView nOperInd;
//            public TextView nStereoInd;
//            public TextView nRssi;
//            public TextView nFreqValue;
//            public TextView nPwrIndicator;
//            public TextView nFreqOffset;
//            public TextView nPilotDet;
//            public TextView nNoDacLpf;
//        }
//
//    }
//
//    private void searchStationForSeekChannel() {
//        int value = -1;
//        int freq = (int) (Float.parseFloat(mMinChannel) * 1000);
//        int prev = (int) (Float.parseFloat(mMinChannel) * 1000);
//        while (freq <= (int) (Float.parseFloat(mMaxChannel) * 1000)) {
//            value = mFmManager.searchStation(prev, SearchDirection.FM_SEARCH_UP,
//                    FM_SEARCH_TIMEOUT);
//            if (value == -1) {
//                Log.e(TAG, "current no search frequency");
//                break;
//            }
//            freq = getFreq();
//            if (freq <= prev || freq >= (int) (Float.parseFloat(mMaxChannel) * 1000)) {
//                Log.e(TAG, "current frequency is unnormal value=" + freq + "," + prev);
//                break;
//            }
//            android.util.Log.d(TAG, "freq = " + freq);
//
//            try {
//                Thread.sleep(500);
//            } catch (Exception e) {
//                e.printStackTrace();
//            }
//
//            ArrayList<Long> tempArrayList = getRegisterInfo();
//
//            mChannelRegisterInfo.put(new Integer(freq), tempArrayList);
//
//            Log.d(TAG, "find freq=" + freq);
//            if (freq != prev) {
//                prev = freq;
//                continue;
//            }
//        }
//    }
//
//    public boolean decideNeedConditionForSeekChannel() {
//        if (!isHeadsetExists()) {
//            Toast.makeText(getActivity(),
//                    getActivity().getResources().getString(R.string.check_earphone),
//                    Toast.LENGTH_LONG).show();
//            return false;
//        }
//
//        mMinChannel = mMinChannelEditText.getText().toString().trim();
//        mMaxChannel = mMaxChannelEditText.getText().toString().trim();
//        if (mMinChannel.isEmpty() || mMaxChannel.isEmpty()) {
//            Toast.makeText(getActivity(),
//                    getActivity().getResources().getString(R.string.check_input_frequency),
//                    Toast.LENGTH_LONG).show();
//            return false;
//            /* SPRD: fix bug348882 limit the input number of length @{ */
//        } else if (mMinChannel.length() > 3 || mMaxChannel.length() > 3) {
//            Toast.makeText(getActivity(),
//                    getActivity().getResources().getString(R.string.invalid_frequency),
//                    Toast.LENGTH_LONG).show();
//            return false;
//        }
//        /* @} */
//
//        return true;
//    }
//
//    public boolean decideNeedConditionForSeekAllChannel() {
//        if (!isHeadsetExists()) {
//            Toast.makeText(getActivity(),
//                    getActivity().getResources().getString(R.string.check_earphone),
//                    Toast.LENGTH_LONG).show();
//            return false;
//        }
//
//        String seekCount = mSeekAllChannelFrequencyEdittext.getText().toString().trim();
//        if (seekCount.isEmpty()) {
//            Toast.makeText(getActivity(),
//                    getActivity().getResources().getString(R.string.check_input_count),
//                    Toast.LENGTH_LONG).show();
//            return false;
//            /* SPRD: fix bug348882 limit the input number of length @{ */
//        } else if (seekCount.length() >= 10) {
//            Toast.makeText(getActivity(),
//                    getActivity().getResources().getString(R.string.invalid_count),
//                    Toast.LENGTH_LONG).show();
//            return false;
//        }
//        /* @} */
//
//        return true;
//    }
//
//    public void seekAllChannel(int seekCounts) {
//        if (!mNextSeek) {
//            Log.d(TAG, "last seek do not over");
//            return;
//        }
//        mStatisticsCountsArrayList.clear();
//        mSeekAllChannelHandler.sendEmptyMessage(0);
//
//        mSeekThread = new HandlerThread("SeekFragment");
//        mSeekThread.start();
//        mSeekHandler = new SeekHandler(mSeekThread.getLooper());
//
//        Message startPowerMsg = new Message();
//        startPowerMsg.what = MSG_START_POWER_UP;
//        startPowerMsg.arg1 = seekCounts;
//        mSeekHandler.sendMessage(startPowerMsg);
//    }
//
//    private void startPowerUp(int seekCount) {
//        Log.d(TAG, "startPowerUp");
//        boolean value = mFmManager.powerUp();
//        if (!value) {
//            Toast.makeText(getActivity(), "power up fm fail", Toast.LENGTH_SHORT).show();
//            Log.e(TAG, "powerUp fail ");
//            return;
//        } else {
//            Log.e(TAG, "powerUp sucess ");
//        }
//        mFmManager.setAudioPath(AudioPath.FM_AUDIO_PATH_HEADSET);
//
//        mSeekStationHandler.sendEmptyMessage(MSG_START_SEEK_DIALOG);
//        mSeekHandler.removeMessages(MSG_POWER_UP_COMPLETE);
//        Message powerCompleteMsg = new Message();
//        powerCompleteMsg.what = MSG_POWER_UP_COMPLETE;
//        powerCompleteMsg.arg1 = seekCount;
//        mSeekHandler.sendMessage(powerCompleteMsg);
//    }
//
//    public void powerUpComplete(int count) {
//        Log.d(TAG, "powerUpComplete");
//        for (int seekCount = 0; seekCount < count; seekCount++) {
//            Log.d(TAG, " mIsSeek = " + mIsSeek);
//            if (mIsSeek) {
//                searchStation();
//                mNextSeek = false;
//            } else {
//                break;
//            }
//
//        }
//        mNextSeek = true;
//        mFmManager.powerDown();
//        destoryHandlerThread();
//        mSeekStationHandler.sendEmptyMessage(MSG_FINSH_SEEK_DIALOG);
//    }
//
//    private void startSeekDialog() {
//        Log.d(TAG, "startSeekDialog");
//        mNeedRefreshListView = true;
//        if (!mSearchStationDialog.isShowing()) {
//            mSearchStationDialog.setMessage(getResources().getString(
//                    R.string.seek_channel_counts));
//            mSearchStationDialog.setButton(
//                    getResources().getString(R.string.fm_cancel),
//                    new DialogInterface.OnClickListener() {
//
//                        @Override
//                        public void onClick(DialogInterface dialog, int which) {
//                            Log.d(TAG, "cancle mSearchStationDialog");
//                            mNeedRefreshListView = false;
//                            mIsSeek = false;
//                            mStatisticsCountsArrayList.clear();
//                            mSeekAllChannelHandler.sendEmptyMessage(0);
//                            mFmManager.cancelSearch();
//                            mFmManager.powerDown();
//                            destoryHandlerThread();
//                            mSearchStationDialog.cancel();
//                        }
//                    });
//            mSearchStationDialog.show();
//        }
//    }
//
//    private void finishSeek() {
//        Log.d(TAG, "finishSeek");
//        mIsSeek = true;
//        mSearchStationDialog.cancel();
//        Log.d(TAG, "mNeedRefreshListView = " + mNeedRefreshListView
//                + " mStatisticsCountsArrayList.size() = " + mStatisticsCountsArrayList.size());
//        destoryHandlerThread();
//        mSeekAllChannelHandler.sendEmptyMessage(0);
//        mWriteFileHandler.sendEmptyMessage(1);
//    }
//
//    private class SeekHandler extends Handler {
//        public SeekHandler(Looper looper) {
//            super(looper);
//        }
//
//        public void handleMessage(android.os.Message msg) {
//            int seekCount = (int) msg.arg1;
//            switch (msg.what) {
//                case MSG_START_POWER_UP:
//                    if (!mFmManager.isFmOn()) {
//                        startPowerUp(seekCount);
//                    } else {
//                        Log.e(TAG, "fm has power on ");
//                    }
//                    break;
//                case MSG_POWER_UP_COMPLETE:
//                    powerUpComplete(seekCount);
//                    break;
//            }
//        };
//    };
//
//    private Handler mSeekStationHandler = new Handler() {
//        public void handleMessage(android.os.Message msg) {
//
//            switch (msg.what) {
//                case MSG_START_SEEK_DIALOG:
//                    startSeekDialog();
//                    break;
//                case MSG_FINSH_SEEK_DIALOG:
//                    finishSeek();
//                    break;
//            }
//        };
//    };
//
//    private void destoryHandlerThread() {
//        if (mSeekThread != null) {
//            mSeekThread.quitSafely();
//            mSeekThread = null;
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
//    public void sortFrequency() {
//        Integer[] keyset = mStatisticsCounts.keySet().toArray(
//                new Integer[mStatisticsCounts.keySet().size()]);
//
//        mTempList = new ArrayList<Integer>(Arrays.asList(keyset));
//
//        Collections.sort(mTempList);
//    }
//
//    public void sortFrequencyArrayList() {
//        Collections.sort(mStatisticsCountsArrayList);
//    }
//
//    public int getFreq() {
//        int freq = -1;
//        int iFreq = mFmManager.getFreq();
//        if (iFreq > 0) {
//            freq = iFreq;
//        }
//        return freq;
//    }
//
//    private void searchStation() {
//        int value = -1;
//        int freq = RADIO_MIN_FREQUENCY;
//        int prev = RADIO_MIN_FREQUENCY;
//        while (mIsSeek && freq <= RADIO_MAX_FREQUENCY) {
//            value = mFmManager.searchStation(prev, SearchDirection.FM_SEARCH_UP,
//                    FM_SEARCH_TIMEOUT);
//            if (value == -1) {
//                Log.e(TAG, "current no search frequency");
//                android.util.Log.d(TAG, "current no search frequency");
//                break;
//            }
//            freq = getFreq();
//            if (freq <= prev || freq >= RADIO_MAX_FREQUENCY) {
//                Log.e(TAG, "current frequency is unnormal value=" + freq + "," + prev);
//                break;
//            }
//            Log.d(TAG, "find freq=" + freq);
//            mStatisticsCountsArrayList.add(new Integer(freq));
//            if (freq != prev) {
//                prev = freq;
//                continue;
//            }
//        }
//        mStatisticsCountsArrayList.add(new Integer(0));
//    }
//
//    public void forSearchStation(int freq, int prev) {
//        if (mStatisticsCounts.containsKey(freq)) {
//            mStatisticsCounts.put(new Integer(freq), mStatisticsCounts.get(freq) + 1);
//        } else {
//            mStatisticsCounts.put(new Integer(freq), new Integer(1));
//        }
//    }
//
//    public void outputHashmapForDebugLog(HashMap<Integer, Integer> hashMap) {
//        Iterator iterator = hashMap.keySet().iterator();
//        while (iterator.hasNext()) {
//            Object object = iterator.next();
//            String key = object.toString();
//            Integer values = hashMap.get(new Integer(key));
//            android.util.Log.d(TAG, "outputHashmapForDebugLog()----key = " + key + "  values = "
//                    + values.toString());
//        }
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
//    public boolean writeFileForSeekChannel(String path, String name) {
//        Log.d(TAG, "path->" + path);
//        boolean flag = true;
//        FileOutputStream out = null;
//        PrintStream p = null;
//        try {
//            makeRootDirectory(path);
//            out = new FileOutputStream(new File(path + name));
//            p = new PrintStream(out);
//            for (int i = 0; i < mChannelRegisterInfo.size(); i++) {
//                Integer[] tempArray = mChannelRegisterInfo.keySet().toArray(
//                        new Integer[mChannelRegisterInfo.keySet().size()]);
//
//                ArrayList<Integer> tempList = new ArrayList<Integer>(Arrays.asList(tempArray));
//
//                Collections.sort(tempList);
//
//                p.print((tempList.get(i).floatValue() / 1000.0) + "\n");
//
//                android.util.Log.d(TAG, "tempArray [" + i + "] = " + tempList.get(i));
//
//                ArrayList<Long> tempValueSet = mChannelRegisterInfo.get(tempList.get(i));
//
//                for (int x = 0; x < tempValueSet.size(); x++) {
//                    android.util.Log.d(TAG, "tempValueSet " + x + " = " + tempValueSet.get(x));
//                }
//
//                for (int tempSize = 0; tempSize < tempValueSet.size(); tempSize++) {
//                    switch (tempSize) {
//                        case 0:
//                            p.print("Operind = " + tempValueSet.get(tempSize) + "   ");
//                            break;
//                        case 1:
//                            p.print("Stereoind = " + tempValueSet.get(tempSize) + "   ");
//                            break;
//                        case 2:
//                            p.print("Rssi = " + tempValueSet.get(tempSize) + "   ");
//                            break;
//                        case 3:
//                            p.print("Freqvalus = " + tempValueSet.get(tempSize) + "   ");
//                            break;
//                        case 4:
//                            p.print("Pwrindicator = " + (tempValueSet.get(tempSize) - 512) + "   ");
//                            break;
//                        case 5:
//                            p.print("Freqoffset = " + tempValueSet.get(tempSize) + "   ");
//                            break;
//                        case 6:
//                            p.print("Pilotdet = " + tempValueSet.get(tempSize) + "   ");
//                            break;
//                        case 7:
//                            p.print("Nodaclpf = " + tempValueSet.get(tempSize) + "   " + "\n");
//                            break;
//                    }
//                }
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
//    public boolean writeFile(ArrayList<Integer> statisticsCountsArrayList, String path, String name) {
//        Log.d(TAG, "path->" + path);
//        boolean flag = true;
//        FileOutputStream out = null;
//        PrintStream p = null;
//        try {
//            makeRootDirectory(path);
//            out = new FileOutputStream(new File(path + name));
//            p = new PrintStream(out);
//            for (int i = 0; i < statisticsCountsArrayList.size(); i++) {
//                if (statisticsCountsArrayList.get(i).intValue() == 0) {
//                    p.print("\n");
//                } else {
//                    p.print(statisticsCountsArrayList.get(i) + "  ");
//                }
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
//    class SeekAllChannelFrequencyAdapter extends BaseAdapter {
//        private final Context mContext;
//        private final LayoutInflater mInflater;
//
//        public SeekAllChannelFrequencyAdapter(Context context) {
//            mContext = context;
//            mInflater = LayoutInflater.from(mContext);
//        }
//
//        @Override
//        public int getCount() {
//            // return mStatisticsCounts.size();
//            return mStatisticsCountsArrayList.size();
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
//                        getResources().getLayout(R.layout.seek_all_channel_adapter), null);
//                viewHolder = new ViewHolder();
//                viewHolder.mFrequencyAndCounts = (TextView) convertView
//                        .findViewById(R.id.frequency_and_counts);
//                convertView.setTag(viewHolder);
//            } else {
//                viewHolder = (ViewHolder) convertView.getTag();
//            }
//
//            forGetViewStatisticsArrayList(viewHolder, position);
//            // forGetViewStatisticsCounts(viewHolder,position);
//
//            return convertView;
//        }
//
//        public void forGetViewStatisticsArrayList(ViewHolder viewHolder, int position) {
//            if (mStatisticsCountsArrayList.get(position).intValue() == 0) {
//                viewHolder.mFrequencyAndCounts.setText(R.string.over);
//            } else {
//                viewHolder.mFrequencyAndCounts.setText((mStatisticsCountsArrayList.get(position)
//                        .floatValue() / 1000.0) + "");
//            }
//        }
//
//        public void forGetViewStatisticsCounts(ViewHolder viewHolder, int position) {
//            String frequencyAndCounts = mContext.getResources().getString(
//                    R.string.seel_all_channel_frequency_adapter_frequency_and_counts,
//                    mTempList.get(position) + "",
//                    mStatisticsCounts.get(mTempList.get(position)) + "");
//            viewHolder.mFrequencyAndCounts.setText(frequencyAndCounts);
//        }
//
//        public class ViewHolder {
//            TextView mFrequencyAndCounts;
//        }
//    }
//
//    public ArrayList<Long> getRegisterInfo() {
//        ArrayList<Long> tempArratList = new ArrayList<Long>();
//
//        mOperind = getOperInd("0x402700B0");
//        tempArratList.add(mOperind);
//
//        mStereoind = getStereoInd("0x402700B0");
//        tempArratList.add(mStereoind);
//
//        mRssi = getRssi("0x402700B0");
//        tempArratList.add(mRssi);
//
//        mFreqvalus = getFreqValue("0x402700B8");
//        tempArratList.add(mFreqvalus);
//
//        mPwrindicator = getPwrIndicator("0x402700C0");
//        tempArratList.add(mPwrindicator);
//
//        mFreqoffset = getFreqOffset("0x402700BC");
//        tempArratList.add(mFreqoffset);
//
//        mPilotdet = getPilotDet("0x402700B0");
//        tempArratList.add(mPilotdet);
//
//        mNodaclpf = getNoDacLpf("0x402700C8");
//        tempArratList.add(mNodaclpf);
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
//        return tempArratList;
//    }
//
//    public long getOperInd(String registerAddress) {
//        String tempResult = sendCmdAndResult("lookat " + registerAddress);
//        String[] tempStringArray = tempResult.split("\n");
//        android.util.Log.d(TAG, "tempStringArray[0] = " + tempStringArray[0]);
//        android.util.Log.d(TAG, "tempResult = " + tempStringArray[0]);
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
//    private String sendCmdAndResult(String cmd) {
//        byte[] buffer = new byte[1024];
//        String result = null;
//        OutputStream outputStream = null;
//        InputStream inputStream = null;
//        try {
//            outputStream = mSocketClient.getOutputStream();
//            if (outputStream != null) {
//                final StringBuilder cmdBuilder = new StringBuilder(cmd).append('\0');
//                final String cmmand = cmdBuilder.toString();
//                outputStream.write(cmmand.getBytes(StandardCharsets.UTF_8));
//                outputStream.flush();
//            }
//            inputStream = mSocketClient.getInputStream();
//            inputStream.read(buffer, 0, 1024);
//            result = new String(buffer, "utf-8");
//            Log.d(TAG, "result is " + result);
//        } catch (Exception e) {
//            Log.d(TAG, "Failed get outputStream: " + e);
//            e.printStackTrace();
//        }
//        return result;
//    }
//
//}
