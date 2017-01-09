/** Create by Spreadst */

package com.spreadst.lockscreen;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.ILockScreenListener;
import android.widget.TextView;

public class DefualtLockscreen extends Lockscreen {
    int mInitTop = 0;
    int mInitBottom = 0;

    public DefualtLockscreen(Context context, ILockScreenListener listener) {
        super(context, listener);
        LayoutInflater inflater = null;
        Resources resources = null;
        try {
            resources = context.getPackageManager().getResourcesForApplication(
                    Constants.CURRENT_PACKAGE_NAME);
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        inflater = LayoutInflater.from(context);
        inflater.inflate(resources.getLayout(resources.getIdentifier(
                Constants.CURRENT_PACKAGE_NAME + ":layout/defualt_customlockscreen",
                null, null)), this, true);
        DisplayMetrics dm = getResources().getDisplayMetrics();
        final int screenWidth = dm.widthPixels;
        final int screenHeight = dm.heightPixels - 50;
        unlockText = (TextView) findViewById(resources.getIdentifier(
                Constants.CURRENT_PACKAGE_NAME + ":id/unlockText", null, null));
        unlockText.setText(resources.getString(resources.getIdentifier(
                Constants.CURRENT_PACKAGE_NAME + ":string/unlock_text_name_1",
                null, null)));
        int identifier = resources.getIdentifier(Constants.CURRENT_PACKAGE_NAME
                + ":id/startBtn", null, null);
        final Button b = (Button) findViewById(identifier);
        b.setText(resources.getString(resources.getIdentifier(
                Constants.CURRENT_PACKAGE_NAME + ":string/unlock_button_name",
                null, null)));
        b.setOnTouchListener(new OnTouchListener() {
            int lastX, lastY;

            public boolean onTouch(View v, MotionEvent event) {

                int ea = event.getAction();

                Log.i("TAG", "Touch:" + ea);
                switch (ea) {
                    case MotionEvent.ACTION_DOWN:
                        lastX = (int) event.getRawX();
                        lastY = (int) event.getRawY();
                        mInitTop = v.getTop();
                        mInitBottom = v.getBottom();
                        mLockScreenListener.pokeWakelock();
                        break;
                    /**
                     * layout(l,t,r,b) l Left position, relative to parent t Top
                     * position, relative to parent r Right position, relative
                     * to parent b Bottom position, relative to parent
                     */
                    case MotionEvent.ACTION_MOVE:

                        int dx = (int) event.getRawX() - lastX;
                        int dy = (int) event.getRawY() - lastY;
                        int left = v.getLeft() + dx;
                        int top = v.getTop() + dy;
                        int right = v.getRight() + dx;
                        int bottom = v.getBottom() + dy;
                        if (left < 0) {
                            left = 0;
                            right = left + v.getWidth();
                        }
                        if (right > screenWidth) {
                            right = screenWidth;
                            left = right - v.getWidth();
                        }
                        if (top < 0) {
                            top = 0;
                            bottom = top + v.getHeight();
                        }
                        if (bottom > screenHeight) {
                            bottom = screenHeight;
                            top = bottom - v.getHeight();
                        }
                        if (bottom < mInitBottom) {
                            return false;
                        }
                        v.layout(v.getLeft(), top, v.getRight(), bottom);
                        Log.i("",
                                "position" + left + ", " + top + ", " + right
                                        + ", " + bottom + ", " + getHeight() + ", "
                                        + v.getHeight());
                        if (bottom >= (getHeight() - v.getHeight())) {
                            mLockScreenListener.goToUnlockScreen();
                        }

                        lastX = (int) event.getRawX();
                        lastY = (int) event.getRawY();
                        break;
                    case MotionEvent.ACTION_CANCEL:
                    case MotionEvent.ACTION_UP:
                        Log.d("", "layout is " + mInitTop + "  " + mInitBottom);
                        final Message message = Message.obtain();
                        message.obj = v;

                        new Thread() {

                            @Override
                            public void run() {
                                mBackHandler.sendMessage(message);
                            }

                        }.start();
                        break;
                }
                return false;
            }
        });
    }

    final Handler mBackHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            View v = (View) msg.obj;
            v.layout(v.getLeft(), mInitTop, v.getRight(), mInitBottom);
        }

    };

}
