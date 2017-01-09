package com.sprd.engineermode.debuglog.slogui;

import android.os.Bundle;

/**
 * Applications can control slog configuration using this aidl.
 */
interface ISlogService {

    void setNotification(int which, boolean show);
}
