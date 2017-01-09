package com.dream.camera;

import android.view.View;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.os.Handler;
import android.util.Log;

import com.android.camera.settings.Keys;
import com.dream.camera.settings.DataConfig;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DreamSettingUtil;
import com.android.camera2.R;

public class MakeupController {

    private int parameterLevel;
    public interface MakeupListener {

        public void onBeautyValueChanged(int value);

        public void onBeautyValueReset();

        public void setMakeUpController(MakeupController makeUpController);

        public void updateMakeLevel();
    }

    class MakeUpSeekBarChangedListener implements OnSeekBarChangeListener {
        @Override
        public void onProgressChanged(SeekBar SeekBarId, int progress,
                boolean fromUser) {
            Log.d(TAG, "onProgressChanged " + progress);
            /*
             * SPRD: too many beauty set actions will result
             * "Camera master thread job queue full" error thrown @{
             */
            long currentTime = System.currentTimeMillis();
            long delta = currentTime - mLastTime;
            /*
             * if (delta > 100) { mLastBeautyTime = currentTime; } else {
             * return; }
             */
            /* @} */
            int offset = progress % 25;
            int set = progress / 25;
            parameterLevel = 0;
            mCurMakeupLevel.setText("" + (set + 1));
            if (offset == 0) {
                parameterLevel = progress;
            } else if (offset <= 13) {
                mMakeupSeekBar.setProgress(set * 25);
                parameterLevel = set * 25;
            } else {
                mMakeupSeekBar.setProgress(set * 25 + 25);
                parameterLevel = set * 25 + 25;
            }
//            if (delta > 100) {
//                mLastTime = currentTime;
//                mController.onBeautyValueChanged(parameterLevel);
//                DataModuleManager
//                        .getInstance(mMakeupSeekBar.getContext())
//                        .getCurrentDataModule().set(DataConfig.SettingStoragePosition.positionList[6], mKey, "" + parameterLevel);
//            }
            setValue(parameterLevel);
            mHandler.removeCallbacks(runnAble);
            mHandler.postAtTime(runnAble, 100);
        }

        @Override
        public void onStartTrackingTouch(SeekBar SeekBarId) {
        }

        @Override
        public void onStopTrackingTouch(SeekBar SeekBarId) {
            // SPRD: fix bug 487525 save makeup level for makeup module
            mController.onBeautyValueChanged(getValue());
        }
    }

    private class UpdateRunnable implements Runnable{

        @Override
        public void run() {
            mController.onBeautyValueChanged(getValue());
        }

    }

    private int getValue(){
        return DreamSettingUtil.convertToInt(DataModuleManager
                .getInstance(mMakeupSeekBar.getContext())
                .getCurrentDataModule()
                .getString(DataConfig.SettingStoragePosition.positionList[3],
                        mKey, "" + mDefaultValue));
    }

    private void setValue(int level){
        DataModuleManager
                .getInstance(mMakeupSeekBar.getContext())
                .getCurrentDataModule()
                .set(DataConfig.SettingStoragePosition.positionList[3], mKey,
                        "" + level);
    }

    private static final String TAG = "MakeupController";

    private SeekBar mMakeupSeekBar;
    private LinearLayout mMakeupControllerView;
    private TextView mCurMakeupLevel;
    private MakeupListener mController;
    private String mKey;
    private int mDefaultValue;
    private long mLastTime = 0;
    Handler mHandler;
    UpdateRunnable runnAble;

    public MakeupController(View extendPanelParent, MakeupListener listener,
            String key, int defaultValue) {
        if (extendPanelParent != null) {
            mMakeupControllerView = (LinearLayout) extendPanelParent
                    .findViewById(R.id.dream_make_up_panel);
            mMakeupSeekBar = (SeekBar) mMakeupControllerView
                    .findViewById(R.id.make_up_seekbar);
            mMakeupSeekBar
                    .setOnSeekBarChangeListener(new MakeUpSeekBarChangedListener());
            mCurMakeupLevel = (TextView) mMakeupControllerView
                    .findViewById(R.id.current_make_up_level);
            mController = listener;
            mKey = key;
            mDefaultValue = defaultValue;

            mHandler = new Handler(extendPanelParent.getContext().getMainLooper());
            runnAble = new UpdateRunnable();

            mController.setMakeUpController(this);
            mController.updateMakeLevel();
        }
    }

    private void initMakeupLevel() {
        if (mMakeupSeekBar != null) {
            String smakeupLevel = DataModuleManager
                    .getInstance(mMakeupSeekBar.getContext())
                    .getCurrentDataModule().getString(DataConfig.SettingStoragePosition.positionList[3], mKey, "" + mDefaultValue);

            int makeupLevel = DreamSettingUtil.convertToInt(smakeupLevel);
            mMakeupSeekBar.setProgress(makeupLevel);
            mCurMakeupLevel.setText("" + (makeupLevel / 25 + 1));
            mMakeupSeekBar.invalidate();
            mCurMakeupLevel.invalidate();
            mController.onBeautyValueChanged(makeupLevel);
        }
    }

    public void resumeMakeupControllerView() {
        Log.i(TAG, "resumeMakeupControllerView");
        if (mMakeupSeekBar == null) {
            return;
        }
        initMakeupLevel();

        mMakeupControllerView.setVisibility(View.VISIBLE);
    }

    public void pauseMakeupControllerView() {
        Log.i(TAG, "pasueMakeupControllerView");
        if (mMakeupSeekBar == null) {
            return;
        }
        mController.onBeautyValueReset();

        mMakeupControllerView.setVisibility(View.GONE);
    }

    public void pauseMakeupControllerView(boolean resetValue) {
        Log.i(TAG, "pasueMakeupControllerView resetValue = " + resetValue);
        if (mMakeupSeekBar == null) {
            return;
        }
        if (resetValue) {
            mController.onBeautyValueReset();
        }
        mMakeupControllerView.setVisibility(View.GONE);
    }
}
