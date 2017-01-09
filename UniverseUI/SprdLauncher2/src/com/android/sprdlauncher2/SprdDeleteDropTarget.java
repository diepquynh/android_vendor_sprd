/** Created by spreadst */

package com.android.sprdlauncher2;

import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import com.sprd.sprdlauncher2.OwnerWidgetInfo;

public class SprdDeleteDropTarget extends DeleteDropTarget {

    public SprdDeleteDropTarget(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SprdDeleteDropTarget(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void onDragStart(DragSource source, Object info, int dragAction) {
        // SPRD: Fix bug281552,long pressed on an item will stretch the
        // wallpaper.
        mLauncher.getWorkspace().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN);
        super.onDragStart(source, info, dragAction);
    }

    @Override
    public void onDragEnd() {
        super.onDragEnd();
        /*
         * SPRD: Fix bug281552,long pressed on an item will stretch the
         * wallpaper. @{
         */
        if (!mLauncher.isEditingMode()) {
            mLauncher.getWorkspace().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_VISIBLE);
        }
        /* @} */
    }

    @Override
    public boolean acceptDrop(DragObject d) {
        // SPRD: Feature 251656, Remove the application drawer view
        return true;
    }

    public void onDragEnter(DragObject d) {
        super.onDragEnter(d);

        animWorkspaceDown();
    }

    public void onDragExit(DragObject d) {
        super.onDragExit(d);
        /* SPRD: add for bug 275284@{ */
        if (!mLauncher.getDragController().exitCalledFromDrop) {
            animWorkspaceUp();
        }
        /* @} */
    }

    /* SPRD: UUI : workspace down/up anim @{ */
    private boolean isDown = false;
    private static final int EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT = 200;
    ValueAnimator hideAnim;

    private void animWorkspaceDown() {
        if (mLauncher.isEditingMode()) {
            return;
        }
        final int deltaX = this.getHeight();
        final int count = mLauncher.getDragLayer().getChildCount();
        hideAnim = ValueAnimator.ofInt(deltaX);
        hideAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        hideAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                int value = (Integer) (animation.getAnimatedValue());
                for (int i = 0; i < count; i++) {
                    View vw = mLauncher.getDragLayer().getChildAt(i);
                    if (vw != null && !(vw instanceof SearchDropTargetBar)
                            && !(vw instanceof DragView)
                            /* SPRD: Bug 270719 @{ */
                            && (vw.getId() != R.id.folder_bg)) {
                        /* @} */
                        vw.setTranslationY(value);
                    }
                }
            }
        });
        hideAnim.start();
        isDown = true;
    }

    protected void animWorkspaceUp() {
        if (!isDown) {
            return;
        }
        isDown = false;
        final int deltaX = this.getHeight();
        final int count = mLauncher.getDragLayer().getChildCount();
        final ValueAnimator hideAnim = ValueAnimator.ofInt(deltaX);
        hideAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        hideAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                int value = deltaX - (Integer) (animation.getAnimatedValue());
                for (int i = 0; i < count; i++) {
                    View vw = mLauncher.getDragLayer().getChildAt(i);
                    if (vw != null && !(vw instanceof SearchDropTargetBar)
                            && !(vw instanceof DragView)
                            /* SPRD: Bug 270719 @{ */
                            && (vw.getId() != R.id.folder_bg)) {
                        /* @} */
                        vw.setTranslationY(value);
                    }
                }
            }
        });
        hideAnim.start();
    }

    /* @} */

    /**
     * SPRD: Feature 261947,return if this is OWNER_WIDGET. @{
     * @param d
     * @return boolean
     */
    protected boolean isWorkspaceOrFolderOwnerWidget(DragObject d) {
        return isDragSourceWorkspaceOrFolder(d)
                && (d.dragInfo instanceof OwnerWidgetInfo);
    }
    /** @} */

}