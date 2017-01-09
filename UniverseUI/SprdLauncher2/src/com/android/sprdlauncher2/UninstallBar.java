/** Created by Spreadst */
package com.android.sprdlauncher2;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.Message;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class UninstallBar extends LinearLayout implements View.OnClickListener {

    private static final int EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT = 200;
    private int mHeight;
    private Button mCancel;
    private Button mUninstall;
    private Launcher mLauncher;
    private TextView mDisplay;
    private ImageView mIcon;
    private String mPendingUninstallPkg;
    public UninstallBar(Context context) {
        super(context);
        mHeight = (int)context.getResources().getDimension(R.dimen.uninstall_bar_height);
    }

    public UninstallBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mHeight = (int)context.getResources().getDimension(R.dimen.uninstall_bar_height);
    }

    public UninstallBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        mHeight = (int)context.getResources().getDimension(R.dimen.uninstall_bar_height);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mCancel = (Button)findViewById(R.id.cancel);
        mUninstall = (Button)findViewById(R.id.uninstall);
        mCancel.setOnClickListener(this);
        mUninstall.setOnClickListener(this);
        mDisplay = (TextView)findViewById(R.id.display);
        mIcon = (ImageView)findViewById(R.id.icon);
    }

    public void setLauncher(Launcher launcher){
        mLauncher = launcher;
    }
    public void showUninstallBar(ShortcutInfo info){
        mPendingUninstallPkg = info.intent.getComponent().getPackageName();
        mDisplay.setText(String.format(mLauncher.getResources().getString(R.string.uninstall_app), info.title));
        Bitmap icon = info.getIcon(LauncherAppState.getInstance().getIconCache());
        mIcon.setImageBitmap(icon);
        animShow();
    }

    private void animShow(){
        final View showView = this;
        final int deltaX = mHeight;
        /* SPRD:remove for not use @{
         * orgi
        final int count = mLauncher.getDragLayer().getChildCount();
        @}*/

        final ValueAnimator showAnim = ValueAnimator
                .ofInt(deltaX);
//        mLauncher.getWorkspace().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN);
        showAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        showAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                showView.setTranslationY((Integer) (animation
                        .getAnimatedValue()) - deltaX);
            }
        });
        if (showAnim.getDurationScale() < 1.0f) {
            showAnim.setDurationScale(1.0f);
        }
        final ValueAnimator hideAnim = ValueAnimator
                .ofInt(deltaX);
        hideAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        hideAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                int value = (Integer) (animation.getAnimatedValue());
                mLauncher.getDragLayer().setTranslationY(value);
                float alpha = (Integer)(animation.getAnimatedValue()) / (deltaX * 2.0f);
                mLauncher.getDragLayer().setAlpha(1.0f - alpha);
            }
        });
        if (hideAnim.getDurationScale() < 1.0f) {
            hideAnim.setDurationScale(1.0f);
        }
        AnimatorSet as = new AnimatorSet();
        as.addListener(new AnimatorListenerAdapter() {

            @Override
            public void onAnimationEnd(Animator animation) {
                mLauncher.getDragLayer().setAlpha(0.5f);
            }

            @Override
            public void onAnimationStart(Animator animation) {
                showView.setVisibility(View.VISIBLE);
            }
        });
        as.playTogether(showAnim, hideAnim);
        as.start();
    }
    public void hideUninstallBar(){
        final View hideView = this;
        final int deltaX = mHeight;
        /* SPRD:remove for not use @{
         * orgi
        final int count = mLauncher.getDragLayer().getChildCount();
        @}*/

        final ValueAnimator showAnim = ValueAnimator
                .ofInt(deltaX);
//        if(!mLauncher.isEditingMode()){
//            mLauncher.getWorkspace().setSystemUiVisibility(View.VISIBLE);
//        }
        Log.d("Launcher","===UninstallBar.java======hideUninstallBar()   showAnim = "+showAnim);
        showAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        showAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                int value = deltaX - (Integer) (animation.getAnimatedValue());
                mLauncher.getDragLayer().setTranslationY(value);
                float alpha = (Integer)(animation.getAnimatedValue()) / (deltaX * 2.0f);
                mLauncher.getDragLayer().setAlpha(0.5f + alpha);
            }
        });
        final ValueAnimator hideAnim = ValueAnimator
                .ofInt(deltaX);
        hideAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        hideAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                hideView.setTranslationY(-(Integer) (animation.getAnimatedValue()));
            }
        });

        /* SPRD: add for bug 275284@{ */
        float temp = 0;
        final int count1 = mLauncher.getDragLayer().getChildCount();
        for (int i = 0; i < count1; i++) {
            if (!(mLauncher.getDragLayer().getChildAt(i) instanceof SearchDropTargetBar)) {
                temp = mLauncher.getDragLayer().getChildAt(i).getTranslationY();
                break;
            }
        }
        final float deltaX1 = temp;
        final ValueAnimator hideAnim1 = ValueAnimator.ofFloat(deltaX1);
        hideAnim1.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        hideAnim1.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float value = deltaX1 - (Float)(animation.getAnimatedValue());
                for (int i = 0; i < count1; i++) {
                    View vw = mLauncher.getDragLayer().getChildAt(i);
                    if (vw != null && !(vw instanceof SearchDropTargetBar)
                            && !(vw instanceof DragView)
                            /* SPRD: Bug 270719 @{ */
                            &&(vw.getId()!=R.id.folder_bg)) {
                            /* @} */
                        vw.setTranslationY(value);
                    }
                }
            }
        });

        AnimatorSet as = new AnimatorSet();
        as.addListener(new AnimatorListenerAdapter() {

            @Override
            public void onAnimationEnd(Animator animation) {
                hideView.setVisibility(View.GONE);
                mLauncher.getDragLayer().setAlpha(1.0f);
                Log.d("Launcher","===UninstallBar.java======onAnimationEnd()   ");
            }
        });
        as.playTogether(showAnim, hideAnim, hideAnim1);
        as.start();
        /* @} */
        /* SPRD: bug 278428 In Edit Mode ,if add appweight can't keep full screen @{*/
        SprdUtilities.setWindowFullScreen(mLauncher.getWindow(), mLauncher.editscreen == Launcher.FULL);
        /*@}*/
    }

    public void onClick(View v) {
        if (v == mCancel) {
            hideUninstallBar();
            // SPRD: fix bug285091 crash when uninstall application twice
            // SPRD: Fix bug 321525, there is no need to set the visibility of dragSourceView.
            //mLauncher.getWorkspace().setDragSourceViewVisible(true);
        } else if (v == mUninstall) {
            // todo:uninstall at background
            // mPendingUninstall
            // SPRD: Fix bug 321525, there is no need to set the visibility of dragSourceView.
            // mLauncher.getWorkspace().setDragSourceViewVisible(false);
            // SPRD: Fix bug 292421, Icon will be gone after interrupt icon's drag status.
            mLauncher.getWorkspace().mIsUnistalling = true;
            hideUninstallBar();
            new Thread(){
                @Override
                public void run() {
                    mLauncher.getPackageManager().deletePackage(mPendingUninstallPkg, null, 0);
                }
            }.start();
        }
        mLauncher.refreshResults();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // TODO Auto-generated method stub
        return true;
    }
}
