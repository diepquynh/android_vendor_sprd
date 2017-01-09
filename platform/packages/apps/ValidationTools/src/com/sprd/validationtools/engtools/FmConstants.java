
package com.sprd.validationtools.engtools;

public class FmConstants {
    public enum Band {
        FM_BAND_NA,
        FM_BAND_EU,
        FM_BAND_JP_STD,
        FM_BAND_JP_WIDE,
        FM_BAND_UNKNOWN
    };

    public enum AudioMode {
        FM_AUDIO_MODE_AUTO,
        FM_AUDIO_MODE_STEREO,
        FM_AUDIO_MODE_MONO,
        FM_AUDIO_MODE_BLEND,
        FM_AUDIO_MODE_UNKNOWN
    };

    public enum SearchDirection {
        FM_SEARCH_DOWN,
        FM_SEARCH_UP
    };

    public enum StepType {
        FM_STEP_50KHZ,
        FM_STEP_100KHZ,
        FM_STEP_UNKNOWN
    };

    public enum AudioPath {
        FM_AUDIO_PATH_NONE,
        FM_AUDIO_PATH_SPEAKER,
        FM_AUDIO_PATH_HEADSET,
        FM_AUDIO_PATH_UNKNOWN
    };

    public enum MuteMode {
        FM_MUTE_MODE_UNMUTE,
        FM_MUTE_MODE_MUTE,
        FM_MUTE_MODE_UNKNOWN
    };

    public static final int MIN_VOLUME = 0;
    public static final int MAX_VOLUME = 15;
}
