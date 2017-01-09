/* Create by SPRD */

package com.android.internal.telephony.cat;

import com.android.internal.telephony.cat.AppInterface;
import com.android.internal.telephony.cat.CatResponseMessage;

/**
 * Interface for communication between STK App and CAT Telephony
 *
 * {@hide}
 */
public interface AppInterfaceSprd extends AppInterface {

    /* SPRD: Add here for BIP function @{ */
    public static final int DEFAULT_CHANNELID = 0x01;
    /* @} */

    /* SPRD: Add here for SET UP EVENT function @{ */
    public static final String CAT_IDLE_SCREEN =
                                    "com.sprd.action.stk.idle_screen";
    public static final String CAT_USER_ACTIVITY =
                                    "com.sprd.action.stk.user_activity";
    /* @} */

    /*
     * Callback function from app to telephony to pass a result code and user's
     * input back to the ICC.
     */
    void onCmdResponse(CatResponseMessageSprd resMsg);

    /*
     * Enumeration for representing "Type of Command" of proactive commands.
     * Those are the only commands which are supported by the Telephony. Any app
     * implementation should support those.
     * Refer to ETSI TS 102.223 section 9.4
     */
    public static enum CommandTypeSprd {
        DISPLAY_TEXT(0x21),
        GET_INKEY(0x22),
        GET_INPUT(0x23),
        LAUNCH_BROWSER(0x15),
        PLAY_TONE(0x20),
        REFRESH(0x01),
        SELECT_ITEM(0x24),
        SEND_SS(0x11),
        SEND_USSD(0x12),
        SEND_SMS(0x13),
        SEND_DTMF(0x14),
        SET_UP_EVENT_LIST(0x05),
        SET_UP_IDLE_MODE_TEXT(0x28),
        SET_UP_MENU(0x25),
        SET_UP_CALL(0x10),
        PROVIDE_LOCAL_INFORMATION(0x26),
        /* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
        LANGUAGE_NOTIFACTION(0x35),
        /* @}*/
        OPEN_CHANNEL(0x40),
        CLOSE_CHANNEL(0x41),
        RECEIVE_DATA(0x42),
        SEND_DATA(0x43),
        GET_CHANNEL_STATUS(0x44);

        private int mValue;

        CommandTypeSprd(int value) {
            mValue = value;
        }

        public int value() {
            return mValue;
        }

        /**
         * Create a CommandType object.
         *
         * @param value Integer value to be converted to a CommandType object.
         * @return CommandType object whose "Type of Command" value is {@code
         *         value}. If no CommandType object has that value, null is
         *         returned.
         */
        public static CommandTypeSprd fromInt(int value) {
            for (CommandTypeSprd e : CommandTypeSprd.values()) {
                if (e.mValue == value) {
                    return e;
                }
            }
            return null;
        }
    }
}

