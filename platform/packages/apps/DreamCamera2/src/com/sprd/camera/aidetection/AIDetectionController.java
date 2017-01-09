/**
 * 
 */

package com.sprd.camera.aidetection;

import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import android.util.Log;

/**
 * @author SPRD
 */
public class AIDetectionController {

    private static final String TAG = "CAM_PhotoModule_AIDetection";

    private static final String VAL_OFF = "off";
    private static final String VAL_FACE = "face";
    private static final String VAL_SMILE = "smile";
    private int sSmileScoreCount = 0;
    private static int SMILE_STEP_INCRMENT = 1;
    private static int SMILE_STEP_DECREASE = -1;
    private static int SMILE_STEP_MAX = 10;
    public static int SMILE_SCORE_X = 7;
    private static String strValue = VAL_FACE;

    public AIDetectionController(SettingsManager sm) {
        getChooseValue(sm);
    }

    public AIDetectionController() {
    }// SPRD: BUG 330578

    private String getChooseValue(SettingsManager sm) {
        if (sm != null) {
            strValue = sm.getString(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_AI_DATECT);
        }
        Log.d(TAG, " getChooseValue strValue=" + strValue);
        return strValue;
    }

    public String getChooseValue(String value) {
        strValue = value;
        Log.d(TAG, " getChooseValue strValue=" + strValue);
        return strValue;
    }

    public boolean isChooseOff() {
        return (VAL_OFF.equals(strValue));
    }

    public boolean isChooseFace() {
        return (VAL_FACE.equals(strValue));
    }

    public boolean isChooseSmile() {
        return (VAL_SMILE.equals(strValue));
    }

    private void setSmileScoreCount(int num) {
        Log.d(TAG, " setSmileScoreCount sSmileScoreCount=" + sSmileScoreCount + "   num=" + num);
        sSmileScoreCount += num;
        if (sSmileScoreCount < 0)
            sSmileScoreCount = 0;
        if (sSmileScoreCount > SMILE_STEP_MAX) {
            sSmileScoreCount = 0;
        }

    }

    public void resetSmileScoreCount(boolean isIncrement) {
        setSmileScoreCount(isIncrement ? SMILE_STEP_INCRMENT : SMILE_STEP_DECREASE);
    }

}
