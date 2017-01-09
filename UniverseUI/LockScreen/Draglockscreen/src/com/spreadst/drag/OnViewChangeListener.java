/** Create by Spreadst */

package com.spreadst.drag;

public interface OnViewChangeListener {
    public void OnViewChange(int view);

    public void pokeWakelock();

    public void goToUnlockScreen();

    public void goToCall();

    public void goToSMS();
}
