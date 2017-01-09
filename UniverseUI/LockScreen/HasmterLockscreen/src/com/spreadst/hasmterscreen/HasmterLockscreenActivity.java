/** Create by Spreadst */

package com.spreadst.hasmterscreen;

import android.app.Activity;
import android.os.Bundle;
import android.content.pm.PackageManager.NameNotFoundException;

public class HasmterLockscreenActivity extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        try {
            new HasmterLockscreen(this, null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
