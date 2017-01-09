package android.provider;

/**
 * @hide
 *
 */
public final class SettingsEx {

    public static final class SystemEx {

        /** SPRD: add default ringtone uriString to system dataBase @{ */
        public static final String DEFAULT_RINGTONE = "default_ringtone";
        public static final String DEFAULT_NOTIFICATION = "default_notification";
        public static final String DEFAULT_ALARM = "default_alarm";
        /** @} */

    }

    public static final class GlobalEx {

        /**
         * Indicate state of radio busy to limit some operations related to radios.
         */
        public static final String RADIO_BUSY = "radio_busy";
    }
}
