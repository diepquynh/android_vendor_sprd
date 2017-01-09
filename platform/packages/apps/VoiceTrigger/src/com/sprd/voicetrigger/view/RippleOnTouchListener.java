
package com.sprd.voicetrigger.view;

import android.view.MotionEvent;
import android.view.View;

public interface RippleOnTouchListener {
    boolean onTouchEvent(View v, MotionEvent event);

    void onActionUP();

    void onStart();

    void onStop();
}
