
package com.android.fmradio;

public class FmConstantsForBrcm {
    public enum FmBandForBrcm {
        FM_BAND_NA,
        FM_BAND_EU,
        FM_BAND_JP_STD,
        FM_BAND_JP_WIDE,
        FM_BAND_UNKNOWN
    };

    public enum FmAudioModeForBrcm {
        FM_AUDIO_MODE_AUTO,
        FM_AUDIO_MODE_STEREO,
        FM_AUDIO_MODE_MONO,
        FM_AUDIO_MODE_BLEND,
        FM_AUDIO_MODE_UNKNOWN
    };

    public enum FmSearchDirectionForBrcm {
        FM_SEARCH_DOWN,
        FM_SEARCH_UP
    };

    public enum FmStepTypeForBrcm {
        FM_STEP_50KHZ,
        FM_STEP_100KHZ,
        FM_STEP_UNKNOWN
    };

    public enum FmAudioPathForBrcm {
        FM_AUDIO_PATH_NONE,
        FM_AUDIO_PATH_SPEAKER,
        FM_AUDIO_PATH_HEADSET,
        FM_AUDIO_PATH_UNKNOWN
    };

    public enum FmMuteModeForBrcm {
        FM_MUTE_MODE_UNMUTE,
        FM_MUTE_MODE_MUTE,
        FM_MUTE_MODE_UNKNOWN
    };

    public static final int MIN_VOLUME = 0;
    public static final int MAX_VOLUME = 15;

    /* BRCM */
    public static final int MAX_VOLUME_BRCM = 255;
    public final static boolean FM_SOFTMUTE_FEATURE_ENABLED = false;
    public static final boolean COMBO_SEARCH_MULTI_CHANNEL_DEFAULT = false;

    /* RDS */
    /* Noise floor level*/
    /** This sets the Noise Floor Level to LOW. */
    public static final int NFL_LOW = 0;
    /** This sets the Noise Floor Level to MEDIUM. */
    public static final int NFL_MED = 1;
    /** This sets the Noise Floor Level to FINE. */
    public static final int NFL_FINE = 2;

    /* RDS feature values. */
    /** Specifies the Program Service feature. */
    public static final int RDS_FEATURE_PS = 4;
    /** Specifies the Program Type feature. */
    public static final int RDS_FEATURE_PTY = 8;
    /** Specifies the Traffic Program feature. */
    public static final int RDS_FEATURE_TP = 16;
    /** Specifies the Program Type Name feature. */
    public static final int RDS_FEATURE_PTYN = 32;
    /** Specifies the Radio Text feature. */
    public static final int RDS_FEATURE_RT = 64;

    /* List of Program Type(s) displayed. */
    public static String[] PTY_LIST = {
            "",
            "News",
            "Current Affairs",
            "Information",
            "Sport",
            "Education",
            "Drama",
            "Cultures",
            "Science",
            "Varied Speech",
            "Pop Music",
            "Rock Music",
            "Easy Listening",
            "Light Classics M",
            "Serious Classics",
            "Other Music",
            "Weather & Metr",
            "Finance",
            "Children's Progs",
            "Social Affairs",
            "Religion",
            "Phone In",
            "Travel & Touring",
            "Leisure & Hobby",
            "Jazz Music",
            "Country Music",
            "National Music",
            "Oldies Music",
            "Folk Music",
            "Documentary",
            "Alarm Test",
            "Alarm - Alarm !"
    };
}
