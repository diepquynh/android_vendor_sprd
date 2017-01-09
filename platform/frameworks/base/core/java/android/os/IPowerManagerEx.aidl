package android.os;

interface IPowerManagerEx{

    void shutdownForAlarm(boolean confirm, boolean isPowerOffAlarm);
    void rebootAnimation();
    void scheduleButtonLightTimeout(long now);

}
