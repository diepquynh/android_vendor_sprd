/** Copyright 2009 - 2013 Broadcom Corporation
 **
 ** This program is the proprietary software of Broadcom Corporation and/or its
 ** licensors, and may only be used, duplicated, modified or distributed
 ** pursuant to the terms and conditions of a separate, written license
 ** agreement executed between you and Broadcom (an "Authorized License").
 ** Except as set forth in an Authorized License, Broadcom grants no license
 ** (express or implied), right to use, or waiver of any kind with respect to
 ** the Software, and Broadcom expressly reserves all rights in and to the
 ** Software and all intellectual property rights therein.
 ** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 ** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 ** ALL USE OF THE SOFTWARE.
 **
 **
 ** Except as expressly set forth in the Authorized License,
 **
 ** 1.     This program, including its structure, sequence and organization,
 **        constitutes the valuable trade secrets of Broadcom, and you shall
 **        use all reasonable efforts to protect the confidentiality thereof,
 **        and to use this information only in connection with your use of
 **        Broadcom integrated circuit products.
 **
 ** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 **        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 **        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 **        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 **        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 **        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 **        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 **        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 **        OF USE OR PERFORMANCE OF THE SOFTWARE.
 **
 ** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 **        ITS LICENSORS BE LIABLE FOR
 **        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 **              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 **              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 **              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 **        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 **              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 **              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 **              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 */

package com.broadcom.bt.app.fm.rx;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.broadcom.bt.app.fm.FmConstants;
import com.broadcom.bt.app.fm.R;

// TODO: this class should really extend a view, rather than include one.

public class FmReceiveView extends FrameLayout implements View.OnClickListener,
        FmRadioFrequencySliderEventListener {

    /*
     * Signal strength boundaries for selecting GUI display. Note that the lower
     * the number, the better the signal.
     */
    static int HIGH_SIGNAL_STRENGTH = 83;
    static int MEDIUM_SIGNAL_STRENGTH = 91;
    static int LOW_SIGNAL_STRENGTH = 99;

    private final static int[] digitDrawables = {
                    R.drawable.digit_0, R.drawable.digit_1, R.drawable.digit_2, R.drawable.digit_3,
                    R.drawable.digit_4, R.drawable.digit_5, R.drawable.digit_6, R.drawable.digit_7,
                    R.drawable.digit_8, R.drawable.digit_9,
    };

    private static final String TAG = "FmReceiveView";
    private static final boolean DEBUG = true;

    /* Registered touch screen event handler. */
    private IRadioViewRxTouchEventHandler mTouchEventhandler;
    private Context mContext;

    /* GUI components */
    private String rdsPs; // program service name
    private String rdsRt; // radio text
    private String rdsPty; // program type (e.g. 4: "Sport")
    private String rdsPtyn; // program type name (e.g. "Football")

    private TextView rdsPsView, rdsRtView, rdsPtyView, rdsPtynView;

    private ImageView rssiStateView, rdsStateView, afStateView;

    private ImageView digit1, digit2, digit3, digit4, digit5;

    private FmRadioFrequencySlider slider;

    private boolean mIsMute = false;
    private boolean mInCall = false;

    private int frequency, lastFrequencyBeforeSliderDown, frequencyStep;

    /**
     * Main constructor. This function registers the supplied eventHandler
     * instance as the receive of all touch screen events.
     * 
     * @param context
     *            the context to operate in.
     * @param eventHandler
     *            the event handler to receive touch screen events.
     */

    public FmReceiveView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    public void init(IRadioViewRxTouchEventHandler eventHandler) {

        if (DEBUG)
            Log.d(TAG, "FmReceiveView(ctx, eventHandler)");

        mTouchEventhandler = eventHandler;

        rdsPsView = (TextView) findViewById(R.id.rds_ps);
        rdsRtView = (TextView) findViewById(R.id.rds_rt);
        rdsPtyView = (TextView) findViewById(R.id.rds_pty);
        rdsPtynView = (TextView) findViewById(R.id.rds_ptyn);

        rssiStateView = (ImageView) findViewById(R.id.rssi_state);
        rdsStateView = (ImageView) findViewById(R.id.rds_state);
        afStateView = (ImageView) findViewById(R.id.af_state);

        rdsStateView.setVisibility(View.INVISIBLE);
        afStateView.setVisibility(View.INVISIBLE);

        digit1 = (ImageView) findViewById(R.id.freq_digit1);
        digit2 = (ImageView) findViewById(R.id.freq_digit2);
        digit3 = (ImageView) findViewById(R.id.freq_digit3);
        digit4 = (ImageView) findViewById(R.id.freq_digit4);
        digit5 = (ImageView) findViewById(R.id.freq_digit5);

        slider = (FmRadioFrequencySlider) findViewById(R.id.freq_slider);
        final int[][] buttonTranslationTable = {
                        {
                                        R.id.mute_button, FmConstants.BUTTON_MUTE
                        }, {
                                        R.id.tune_up_button, FmConstants.BUTTON_TUNE_UP
                        }, {
                                        R.id.tune_down_button, FmConstants.BUTTON_TUNE_DOWN
                        }, {
                                        R.id.seek_up_button, FmConstants.BUTTON_SEEK_UP
                        }, {
                                        R.id.seek_down_button, FmConstants.BUTTON_SEEK_DOWN
                        }, {
                                        R.id.menu_button, FmConstants.BUTTON_SETTINGS
                        }, {
                                        R.id.power_off_button, FmConstants.BUTTON_POWER_OFF
                        }, {
                                        R.id.ch_1, FmConstants.BUTTON_CH_1
                        }, {
                                        R.id.ch_2, FmConstants.BUTTON_CH_2
                        }, {
                                        R.id.ch_3, FmConstants.BUTTON_CH_3
                        }, {
                                        R.id.ch_4, FmConstants.BUTTON_CH_4
                        }, {
                                        R.id.ch_5, FmConstants.BUTTON_CH_5
                        }, {
                                        R.id.ch_6, FmConstants.BUTTON_CH_6
                        }, {
                                        R.id.ch_7, FmConstants.BUTTON_CH_7
                        }, {
                                        R.id.ch_8, FmConstants.BUTTON_CH_8
                        }, {
                                        R.id.ch_9, FmConstants.BUTTON_CH_9
                        }, {
                                        R.id.ch_10, FmConstants.BUTTON_CH_10
                        },
        };

        for (int i = 0; i < buttonTranslationTable.length; i++) {
            int from = buttonTranslationTable[i][0];
            int to = buttonTranslationTable[i][1];
            View v = findViewById(from);
            v.setId(to);
            v.setOnClickListener(this);
        }
        slider.setListener(this);

        updateChannelButtons();
    }

    // @Override
    public void onClick(View v) {
        int buttonId = v.getId();
        if (DEBUG)
            Log.d(TAG, "onClick(): " + buttonId);
        if (buttonId >= FmConstants.BUTTON_CH_1 && buttonId <= FmConstants.BUTTON_CH_10) {
            int position = buttonId - FmConstants.BUTTON_CH_1; // 0 - 9
            // if there is a frequency select it, otherwise set it
            if (mTouchEventhandler.getChannels()[position] != 0) {
                mTouchEventhandler.selectChannel(position);
            } else {
                mTouchEventhandler.setChannel(position);
                updateChannelButtons();
            }
        } else {
            // BUTTON_MUTE is physically one button,
            // but logically both BUTTON_MUTE_ON and BUTTON_MUTE_OFF
            if (buttonId == FmConstants.BUTTON_MUTE) {
                if (mIsMute) {
                    // unmute
                    buttonId = FmConstants.BUTTON_MUTE_OFF;
                    setMutedState(false, mInCall);
                } else {
                    // mute
                    buttonId = FmConstants.BUTTON_MUTE_ON;
                    setMutedState(true, mInCall);
                }
            }

            mTouchEventhandler.handleButtonEvent(buttonId, FmConstants.BUTTON_EVENT_UP);
        }

    }

    void updateChannelButtons() {
        Button b;

        int[] channels = mTouchEventhandler.getChannels();
        for (int i = FmConstants.BUTTON_CH_1; i <= FmConstants.BUTTON_CH_10; i++) {
            b = (Button) findViewById(i);
            int channel = i - FmConstants.BUTTON_CH_1 + 1; // 1 - 10
            int freq = channels[channel - 1];
            if (freq == 0) {
                b.setText(mContext.getResources().getString(R.string.ch_button, channel));
            } else {
                b.setText("" + (freq / 100) + '.' + (freq % 100 / 10));
            }
        }
        updatePressedButton();
    }

    private void updatePressedButton() {
        int[] channels = mTouchEventhandler.getChannels();
        for (int i = FmConstants.BUTTON_CH_1; i <= FmConstants.BUTTON_CH_10; i++) {
            Button b = (Button) findViewById(i);
            int channel = i - FmConstants.BUTTON_CH_1 + 1; // 1 - 10
            int freq = channels[channel - 1];
            if (freq != 0 && freq == this.frequency) {
                b.setBackgroundResource(R.drawable.btn_default_small_pressed);
            } else {
                b.setBackgroundResource(android.R.drawable.btn_default);
            }
        }
    }

    /**
     * Internal function to set the visible frequency digits to the correct
     * display state for a given frequency.
     * <p>
     * NOTE: Always invalidate() the view after calling this function to ensure
     * refresh of GUI.
     * 
     * @param freq
     *            the frequency multiplied by 100
     */

    protected void setFrequencyGraphics(int freq) {
        this.frequency = freq;
        slider.setFreq(freq);
        setFrequencyDigits(freq);
        updatePressedButton();
    }

    protected void setFrequencyDigits(int freq) {
        digit5.setImageResource(digitDrawables[freq % 10]);
        freq /= 10;
        digit4.setImageResource(digitDrawables[freq % 10]);
        freq /= 10;
        digit3.setImageResource(digitDrawables[freq % 10]);
        freq /= 10;
        int d2 = freq % 10;
        digit2.setImageResource(digitDrawables[d2]);
        freq /= 10;
        int d1 = freq % 10;
        digit1.setImageResource(digitDrawables[d1]);

        /* Check that the leading two digits are not zero. */
        if (d1 == 0) {
            digit1.setVisibility(View.INVISIBLE);
            digit2.setVisibility(d2 == 0 ? View.INVISIBLE : View.VISIBLE);
        } else {
            digit1.setVisibility(View.VISIBLE);
            digit1.setVisibility(View.VISIBLE);
        }
        digit5.setVisibility(frequencyStep >= 10 ? View.GONE : View.VISIBLE);

    }

    /**
     * Sets the signal strength indicator state.
     * 
     * @param strength
     *            SIGNAL_STRENGTH_NONE, SIGNAL_STRENGTH_LOW
     *            SIGNAL_STRENGTH_MEDIUM or SIGNAL_STRENGTH_HIGH.
     */
    public synchronized void setSignalStrength(int rssi) {
        int resId;
        if (rssi > LOW_SIGNAL_STRENGTH) {
            resId = R.drawable.status_signal_none;
        } else if (rssi > MEDIUM_SIGNAL_STRENGTH) {
            resId = R.drawable.status_signal_low;
        } else if (rssi > HIGH_SIGNAL_STRENGTH) {
            resId = R.drawable.status_signal_med;
        } else {
            resId = R.drawable.status_signal_high;
        }
        rssiStateView.setImageResource(resId);
    }

    /**
     * Sets the RDS indicator state.
     * 
     * @param rdsState
     *            RDS_STATE_RDS_OFF, RDS_STATE_RDS_ON or RDS_STATE_RBDS_ON
     */
    public synchronized void setRdsState(int rdsState) {
        /* Display new RDS icon. */
        if (rdsState == FmConstants.RDS_STATE_RDS_OFF) {
            rdsStateView.setVisibility(View.INVISIBLE);
        } else if (rdsState == FmConstants.RDS_STATE_RDS_ON) {
            rdsStateView.setVisibility(View.VISIBLE);
            rdsStateView.setImageResource(R.drawable.rds_on);
        } else if (rdsState == FmConstants.RDS_STATE_RBDS_ON) {
            rdsStateView.setVisibility(View.VISIBLE);
            rdsStateView.setImageResource(R.drawable.rbds_on);
        } else {
            // invalid value
            return;
        }
    }

    /**
     * Sets the AF indicator state.
     * 
     * @param afState
     *            AF_STATE_OFF or AF_STATE_ON
     */
    public synchronized void setAfState(int afState) {
        if (afState == FmConstants.AF_STATE_OFF) {
            afStateView.setVisibility(View.INVISIBLE);
        } else if (afState == FmConstants.AF_STATE_ON) {
            afStateView.setVisibility(View.VISIBLE);
        } else {
            // invalid value
            return;
        }

    }

    /* Display muted/unmuted icon. */
    public synchronized void setMutedState(boolean isMute, boolean inCall) {
        mIsMute = isMute;
        mInCall = inCall;

        ImageButton ib = (ImageButton) findViewById(FmConstants.BUTTON_MUTE);
        if (isMute || inCall) {
            ib.setImageResource(android.R.drawable.ic_lock_silent_mode_off);
        } else {
            ib.setImageResource(android.R.drawable.ic_lock_silent_mode);
        }
        ib.setEnabled(!inCall);
    }

    public synchronized void resetRdsText() {
        rdsPsView.setText(null);
        rdsRtView.setText(null);
        rdsPtyView.setText(null);
        rdsPtynView.setText(null);
    }

    /**
     * Will update the display strings for the supported types of RDS data.
     * 
     * @param rdsDataType
     *            the type of RDS data received (RT, PTY, PTYN etc).
     * @param rdsIndex
     *            the value of the RDS data if in integer form.
     * @param rdsText
     *            the text of RDS data if available.
     */
    public synchronized void setRdsText(int rdsDataType, int rdsIndex, String rdsText) {
        /* Only process GUI supported messages. */
        switch (rdsDataType) {
        case FmConstants.RDS_ID_PTY_EVT:
            rdsText = FmConstants.PTY_LIST[rdsIndex];
            rdsPtyView.setText(rdsText);
            break;
        case FmConstants.RDS_ID_PS_EVT:
            rdsPsView.setText(rdsText);
            break;
        case FmConstants.RDS_ID_PTYN_EVT:
            rdsPtynView.setText(rdsText);
            break;
        case FmConstants.RDS_ID_RT_EVT:
            rdsRtView.setText(rdsText);
            break;
        default:
            break;
        }
        ;
    }

    void setMinMaxFrequencies(int minFreq, int maxFreq) {
        slider.setFrequencyRange(minFreq, maxFreq);
    }

    void setFrequencyStep(int frequencyStep) {
        this.frequencyStep = frequencyStep;
    }

    public synchronized void setSeekStatus(boolean seekInProgress, boolean tuneInProgress) {
        findViewById(R.id.seek_indicator).setVisibility(
                seekInProgress || tuneInProgress ? View.VISIBLE : View.INVISIBLE);
        findViewById(FmConstants.BUTTON_TUNE_UP).setEnabled(!seekInProgress);
        findViewById(FmConstants.BUTTON_TUNE_DOWN).setEnabled(!seekInProgress);
        findViewById(FmConstants.BUTTON_SEEK_UP).setEnabled(!seekInProgress);
        findViewById(FmConstants.BUTTON_SEEK_DOWN).setEnabled(!seekInProgress);
    }

    // slider events
    public void onSliderDown() {
        // store the current frequency, in case of cancel
        lastFrequencyBeforeSliderDown = frequency;

    }

    public void onSliderCancel() {
        setFrequencyGraphics(lastFrequencyBeforeSliderDown);
    }

    public void onSliderDrag(int freq) {
        // only update the digits
        setFrequencyDigits(freq);
    }

    public void onSliderSet(int freq) {
        mTouchEventhandler.setFrequency(freq);
    }
}
