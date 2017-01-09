
package com.sprd.engineermode.connectivity.fm;

import android.app.Fragment;

public abstract class AbsFMFragment extends Fragment {

    protected int getRegisterValue(int registerAddress) {
        return 0;
    }

    protected void setRegisterValue(int registerAddress, int registerValue) {

    };

    protected int[] getFMRegisterValue() {
        return null;
    };
}
