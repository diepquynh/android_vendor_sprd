package com.android.sprdlauncher2;

import java.io.File;

import com.android.sprdlauncher2.Launcher.State;
import com.sprd.sprdlauncher2.OwnerWidgetInfo;
import com.sprd.sprdlauncher2.WidgetUpdate;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.app.ActivityManager;
import android.content.ComponentCallbacks2;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import com.sprd.themesettings.ThemeSettingsPlugin;

public class SprdLauncher extends Launcher {

    /* SPRD: Feature 254255,add menu@{ */
    // private static final int MENU_EDIT = Menu.FIRST + 1;
    private static final int MENU_GROUP_WALLPAPER = 1;
    private static final int MENU_PREVIEW = Menu.FIRST + 1;
    private static final int MENU_MANAGE_APPS = MENU_PREVIEW + 1;
    private static final int MENU_SYSTEM_SETTINGS = MENU_MANAGE_APPS + 1;
    private static final int MENU_HELP = MENU_SYSTEM_SETTINGS + 1;
    private static final int MENU_WORKSPACE = 1;
    private static final int MENU_EDIT = MENU_HELP + 1;
    private static final int MENU_WORKSPACE_STYLE = MENU_EDIT + 1;

    /* SPRD: add for UUI @{*/
    public static final String CUSTOMIZEPREF = "home_customize";
    public static final String HOME_SCREEN_NUM_TAG = "home_screen_num";
    public static final String HOME_DEFAULT_SCREEN_TAG = "home_default_screen";
    /* @} */

    private Configuration mConfiguration;
    /* SPRD: Feature 261947,Update OWNER_WIDGET after onResume. @{ */
    private Runnable mUpdateWidgetDataRunnable = new Runnable() {
        public void run() {
            if (mWorkspace != null) {
                mWorkspace.updateOwnerWidgetData();
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mConfiguration = new Configuration();
        mConfiguration.setToDefaults();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /* SPRD: Feature 261947,Update OWNER_WIDGET after onResume. @{ */
        mWorkspace.postDelayed(mUpdateWidgetDataRunnable, 700);
        /* @} */
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig != null) {
            // Only user setting theme could take effect
            if (newConfig.UserSetTheme != null
                    && (mConfiguration.UserSetTheme == null
                    || !mConfiguration.UserSetTheme
                            .equals(newConfig.UserSetTheme))) {
                /* SPRD: Fix bug300202, cleanup widgetPreview images after theme changed. @{ */
                WidgetPreviewLoader.onConfigurationChanged();
                // Force reload all icons.
                Log.d("theme", "configuration has been changed, reload icon and delete thumbnail");
                // Clear thumbnail loaded from framework
                new File(getFilesDir(), "default_thumb.jpg").delete();
                // It is only take effect when theme changed ?
                Utilities.onConfigurationChanged();
                ThemeSettingsPlugin.aquireInstance(this).retainThemeSettingsDialog();
                mIconCache.flush();
                mModel.resetLoadedState(true, true);
                mModel.startLoaderFromBackground();
                /* SPRD: fix bug312856, when the widget icon is not updated after switching theme. @{ */
                if (mAppsCustomizeContent != null && mAppsCustomizeContent.getPageCount() != 0) {
                    mAppsCustomizeContent.onPackagesUpdated(
                        LauncherModel.getSortedWidgetsAndShortcuts(this));
                }
                /* @} */
            }
            // Update local configurations
            mConfiguration.updateFrom(newConfig);
        }
    }

    /**
     * Restores the previous state, if it exists.
     *
     * @param savedState
     *            The previous state.
     */
    protected void restoreState(Bundle savedState) {
        super.restoreState(savedState);
        if (savedState == null) {
            return;
        }
        /* SPRD: Feature 254280,show widget list in edit mode@{ */
        if (mAppsCustomizePagedViewHost != null) {
            mAppsCustomizeContent.loadAssociatedPages(mAppsCustomizeContent
                    .getCurrentPage());
            int currentIndex = savedState.getInt("apps_customize_currentIndex");
            mAppsCustomizeContent.restorePageForIndex(currentIndex);
        }
        /* @} */
    }

    /**
     * Finds all the views we need and configure them properly.
     */
    protected void setupViews() {
        super.setupViews();
        /* SPRD: Feature 251656, Remove the application drawer view @{ */
        mUninstaller = (UninstallBar) findViewById(R.id.uninstall_bar);
        mUninstaller.setLauncher(this);
        /* @} */
        final DragController dragController = mDragController;
        /* SPRD: add for UUI previews @{ */
        mWorkspacePreviews = (Previews) mDragLayer.findViewById(R.id.workspace_preview);
        mWorkspacePreviews.setLauncher(this);
        mWorkspacePreviews.setWorkspace(mWorkspace);
        mWorkspacePreviews.setDragController(dragController);
        /* @} */
        /* SPRD: Feature 254280,show widget list in edit mode@{ */
        mAppsCustomizePagedViewHost = (AppsCustomizePagedViewHost) findViewById(R.id.apps_customize_pane);
        mAppsCustomizeContent = (AppsCustomizePagedView) mAppsCustomizePagedViewHost
                .findViewById(R.id.apps_customize_pane_content);
        mAppsCustomizeContent.setup(this, dragController);
        /* @} */
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        /* SPRD: Feature 254280,show widget list in edit mode@{ */
        if (mAppsCustomizePagedViewHost != null) {
            int currentIndex = mAppsCustomizeContent.getSaveInstanceStateIndex();
            outState.putInt("apps_customize_currentIndex", currentIndex);
        }
        /* @} */
    }

    @Override
    public void onDestroy() {
        /* SPRD: Feature 261947,for contacts widget. @{ */
        ownerWidgetOnDestory();
        /* @} */
        super.onDestroy();
    }

    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        if (level >= ComponentCallbacks2.TRIM_MEMORY_MODERATE) {
            mAppsCustomizePagedViewHost.onTrimMemory();
        }
    }

    /* SPRD: Feature 261947,for contacts widget. @{ */
    public void ownerWidgetOnDestory() {
        int cellCount = mWorkspace.getChildCount();
        for (int i = 0; i < cellCount; i++) {
            CellLayout mCellLayout = (CellLayout) mWorkspace.getChildAt(i);
            ShortcutAndWidgetContainer mCellChildren = mCellLayout.getShortcutsAndWidgets();
            int count = mCellChildren.getChildCount();
            for (int j = 0; j < count; j++) {
                View view = mCellChildren.getChildAt(j);
                if (view.getTag() instanceof OwnerWidgetInfo){
                    ((WidgetUpdate) view).onDestory();
                }
            }
        }
    }
    /* @} */

    /* SPRD: Feature 254255,add menu@{ */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (isWorkspaceLocked()) {
            return false;
        }
        super.onCreateOptionsMenu(menu);
        Intent manageApps = new Intent(Settings.ACTION_MANAGE_ALL_APPLICATIONS_SETTINGS);
        manageApps.setFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
        Intent settings = new Intent(android.provider.Settings.ACTION_SETTINGS);
        settings.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        String helpUrl = getString(R.string.help_url);
        Intent help = new Intent(Intent.ACTION_VIEW, Uri.parse(helpUrl));
        help.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);

        Intent workspaceSettings = new Intent(this, WallpaperSettings.class);

        menu.add(0, MENU_WORKSPACE, 1, R.string.menu_desktop_settings).setIntent(
                workspaceSettings);

        Intent workspaceStyles = new Intent(this, WorkspaceStyleSettings.class);
        menu.add(0, MENU_WORKSPACE_STYLE, 3, R.string.workspace_anim_style)
                .setIntent(workspaceStyles);
        menu.add(0, MENU_MANAGE_APPS, 4, R.string.menu_manage_apps)
            .setIcon(android.R.drawable.ic_menu_manage)
            .setIntent(manageApps)
            .setAlphabeticShortcut('M');
        menu.add(0, MENU_SYSTEM_SETTINGS, 5, R.string.menu_settings)
            .setIcon(android.R.drawable.ic_menu_preferences)
            .setIntent(settings)
            .setAlphabeticShortcut('P');
        if (!helpUrl.isEmpty()) {
            menu.add(0, MENU_HELP, 0, R.string.menu_help)
                .setIcon(android.R.drawable.ic_menu_help)
                .setIntent(help)
                .setAlphabeticShortcut('H');
        }
        return true;
    }
    /* @} */

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        menu.setGroupVisible(MENU_GROUP_WALLPAPER, true);

        if (mState != State.EDITING && !mWorkspacePreviews.isShown()) {
            if (menu.findItem(MENU_EDIT) == null) {
                menu.add(0, MENU_EDIT, 0, R.string.menu_edit_mode);
            }
            if (menu.findItem(MENU_PREVIEW) == null) {
                menu.add(0, MENU_PREVIEW, 2, R.string.menu_edit_preview);
            }
        } else {
            if (menu.findItem(MENU_EDIT) != null) {
                menu.removeItem(MENU_EDIT);
            }
            if (menu.findItem(MENU_PREVIEW) != null) {
                menu.removeItem(MENU_PREVIEW);
            }
        }
        return true;
        /* @} */
    }

    /* SPRD: Feature 252909,add edit mode menu@{ */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()){
        /* SPRD: Fix bug 276575, handle optionItem first. @{ */
        /* SPRD: Feature 254255,add menu@{ */
            case MENU_PREVIEW:
                //MENU_WALLPAPER_SETTINGS is equal to MENU_PREVIEW
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (mWorkspace.getOpenFolder() != null) {
                            Folder openFolder = mWorkspace.getOpenFolder();
                            if (openFolder != null && openFolder.isEditingName()) {
                                openFolder.dismissEditingName();
                            } else {
                                closeFolder();
                            }
                        }
                        mWorkspacePreviews.showPreviews();
                    }
                }, 100);
            return true;
        /* @} */
            case MENU_EDIT:
                /* SPRD: optimization for lunch time @{ */
                if (mAppsCustomizeContent != null && mAppsCustomizeContent.getPageCount() == 0) {
                    mAppsCustomizeContent.onPackagesUpdated(
                        LauncherModel.getSortedWidgetsAndShortcuts(this));
                }
                /* @} */
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if(mWorkspace.getOpenFolder() != null){
                            Folder openFolder = mWorkspace.getOpenFolder();
                            if (openFolder != null && openFolder.isEditingName()) {
                                openFolder.dismissEditingName();
                            }else{
                                closeFolder();
                            }
                        }
                        enterEditingMode();
                    }
                }, 100);
                return true;
        /* @} */
        }
        return super.onOptionsItemSelected(item);
    }

    /* @} */
    /* SPRD: add for UUI empty folders @{*/
    /**
     * Process a empty folder drop.
     *
     * @param screen The screen where it should be added
     * @param cell The cell it should be added to, optional
     * @param position The location on the screen where it was dropped, optional
     */
    void processEmptyFolderFromDrop(CellLayout cellLayout, long container, long screenId,
            int[] cell, int[] loc) {
        resetAddInfo();
        mPendingAddInfo.container = container;
        mPendingAddInfo.screenId = screenId;
        mPendingAddInfo.dropPos = loc;

        if (cell != null) {
            mPendingAddInfo.cellX = cell[0];
            mPendingAddInfo.cellY = cell[1];
            addFolderFromEditMode(cellLayout, container, screenId, cell[0], cell[1]);
        }

    }
    /* @} */

    /* SPRD: add for UUI empty folders @{*/
    FolderIcon addFolderFromEditMode(CellLayout layout, long container, final long screenId, int cellX,
            int cellY) {
        final FolderInfo folderInfo = new FolderInfo();
        // SPRD: Fix bug 318951, default FolderName support multi language.
        folderInfo.title = FolderIcon.DEFAULT_FOLDER_EMPTY;

        // Update the model
        LauncherModel.addItemToDatabase(SprdLauncher.this, folderInfo, container, screenId, cellX, cellY,
                false);
        sFolders.put(folderInfo.id, folderInfo);

        // Create the view
        FolderIcon newFolder =
            FolderIcon.fromXml(R.layout.folder_icon, this, layout, folderInfo, mIconCache);
        mWorkspace.addInScreen(newFolder, container, screenId, cellX, cellY, 1, 1,
                isWorkspaceLocked());
        return newFolder;
    }
    /* @} */

    /* SPRD: Feature 251656, Remove the application drawer view @{ */
    boolean startApplicationUninstallActivity(ShortcutInfo appInfo, int flags) {
        if ((flags & AppInfo.DOWNLOADED_FLAG) == 0) {
            // System applications cannot be installed. For now, show a toast
            // explaining that.
            // We may give them the option of disabling apps this way.
            /* SPRD: Feature 253522, Remove the application drawer view @{ */
            // int messageId = R.string.uninstall_system_app_text;
            // Toast.makeText(this, messageId, Toast.LENGTH_SHORT).show();
            /* @} */
            return false;
        } else {
            /* SPRD: bug 278428 In Edit Mode ,if add appweight can't keep full screen @{*/
            Window win = this.getWindow();
            WindowManager.LayoutParams winParams = win.getAttributes();
            final int bits = WindowManager.LayoutParams.FLAG_FULLSCREEN;
            winParams.flags |= bits;
            win.setAttributes(winParams);
            /*@}*/
            mUninstaller.showUninstallBar(appInfo);
            return true;
        }
    }
    /* @} */

    /* SPRD: fix bug 274649 the icon missed when long click a second icon @{ */
    private boolean mUninstall = false;
    public boolean isUninstall() {
        return mUninstall;
    }

    public void setUninstall(boolean uninstall) {
        mUninstall = uninstall;
    }
    /* @} */

    @Override
    void showWorkspace(boolean animated, Runnable onCompleteRunnable) {
        if(mState == State.EDITING){
            if(isExitEditing){
                isExitEditing = false;
            }else{
                return;
            }
        }
        changeState(Workspace.State.NORMAL);
        super.showWorkspace(animated, onCompleteRunnable);
    }


    /**
     * SPRD: Feature 259193, porting from Android.4.1 @{
     *
     */
    public static final String UNREADINFO = "mms_call_unread";

    @Override
    public void bindComponentUnreadChanged(final ComponentName component, final int unreadNum){
        // Post to message queue to avoid possible ANR.
        SharedPreferences mUnreadInfoPreference = (SharedPreferences) this
                .getSharedPreferences(UNREADINFO,
                        Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = mUnreadInfoPreference.edit();
        editor.putInt(component.getPackageName(), unreadNum);
        editor.commit();
        mHandler.post(new Runnable() {
                public void run() {
                       if (mWorkspace != null) {
                               mWorkspace.updateComponentUnreadChanged(component, unreadNum);
                         }
                }
           });
    }
    /** @} */


    /* SPRD: add for UUI previews @{*/
    public boolean isWorkspaceVisible(){
        return (mWorkspace != null) ? mWorkspace.getVisibility()==View.VISIBLE : false;
    }

    public void showPreviews(){
        if(!isWorkspacePreviewsVisible()){
            mWorkspacePreviews.showPreviews();
        }
    }

    public void hidePreviews(){
        if(isWorkspacePreviewsVisible()){
            /* SPRD: Bug 262277 @{ */
            if (!mHidePreviewGotoDefaultPage)
                mWorkspacePreviews.dismissPreview(mWorkspace.getCurrentPage(),
                        mWorkspacePreviews.getChildAt(mWorkspace.getCurrentPage()));
            else {
                mWorkspacePreviews.dismissPreview(mWorkspace.mDefaultPage,
                        mWorkspacePreviews.getChildAt(mWorkspace.mDefaultPage));
            }
            /* @} */
        }
    }

    public boolean isWorkspacePreviewsVisible() {
        return (mWorkspacePreviews != null) ? !mWorkspacePreviews.dissmissed() : false;
    }

    public void hidePageIndicator(boolean isHide) {
        findViewById(R.id.page_indicator).setVisibility(
                isHide ? View.GONE : View.VISIBLE);
    }
    /* SPRD: Feature 251656, Remove the application drawer view @{ */
    public boolean isUninstallerVisible() {
        return mUninstaller.getVisibility() == View.VISIBLE;
    }

    public void refreshResults() {
        Log.d(TAG,"===Launcher.java======refreshResults()   mOnResumeCallbacks = "+mOnResumeCallbacks.size());
        if (mOnResumeCallbacks.size() > 0) {
            for (int i = 0; i < mOnResumeCallbacks.size(); i++) {
                mOnResumeCallbacks.get(i).run();
            }
            mOnResumeCallbacks.clear();
        }
    }

    public void insertEmptyScreen(Long screenId, int screenRank) {
        mModel.insertEmptyScreen(this, screenId, screenRank);
    }
    /* @} */

    /* SPRD: Feature 252998,workspace into edit mode@{ */
    void enterEditingMode() {
        if (mState == State.WORKSPACE) {
            /* SPRD: bug 278428 In Edit Mode ,if add appweight can't keep full screen @{*/
            setEditscreen(FULL);
            Window win = this.getWindow();
            WindowManager.LayoutParams winParams = win.getAttributes();
            final int bits = WindowManager.LayoutParams.FLAG_FULLSCREEN;
            winParams.flags |= bits;
            win.setAttributes(winParams);
            /*@}*/
            mDragLayer.clearAllResizeFrames();
            /* SPRD: Fix bug 261340 @{ */
            LauncherAppState.getInstance().getDynamicGrid().getDeviceProfile().layout(this, true);
            /* @} */
            /* SPRD: Feature 254917,add new view in edit mode@{ */
            mWorkspace.onEditModeStart();
            /* @} */
            changeState(Workspace.State.SPRING_LOADED);
            /* SPRD: Feature 255608,show delete area @{ */
            mSearchDropTargetBar.showIndicator();
            /* @} */
            switchWidgets(true);
            mState = State.EDITING;

            /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @{ */
            // this line must be called after set mState to State.EDITING becuase we used
            // isEditingMode() in method updateChildrenDeletable(boolean).
            mWorkspace.updateChildrenDeletable(true /* allow show delete image */);
            /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @} */
        }
    }

    void exitEditingMode() {
        if (mState == State.EDITING) {
            // SPRD: Fix bug281552,long pressed on an item will stretch the wallpaper.
            getWorkspace().setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);

            /* SPRD: bug 278428 In Edit Mode ,if add appweight can't keep full screen @{*/
            setEditscreen(UNFULL);
            Window win = this.getWindow();
            WindowManager.LayoutParams winParams = win.getAttributes();
            final int bits = WindowManager.LayoutParams.FLAG_FULLSCREEN;
            winParams.flags &= ~bits;
            win.setAttributes(winParams);
            /*@}*/
            /* SPRD: Fix bug 261340 @{ */
            LauncherAppState.getInstance().getDynamicGrid().getDeviceProfile().layout(this, false);
            /* @} */
            isExitEditing = true;
            /* SPRD: Feature 254953,remove edit view when exit edit mode@{ */
            /* SPRD: bug332564 2014-07-15 suppress layout trasition animation to avoid influence scroll. @{ */
            mWorkspace.disableLayoutTransitions();
            mWorkspace.onEditModeEnd();
            mWorkspace.enableLayoutTransitions();
            /* SPRD: bug332564 2014-07-15 suppress layout trasition animation to avoid influence scroll. @} */

            /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @{ */
            mWorkspace.updateChildrenDeletable(false /* allow show delete image */);
            /* SPRD: bug258558 2014-01-23 new requirement to allow delete in edit mode @} */

            /* @} */
            /* SPRD: Feature 255608,show delete area @{ */
            mSearchDropTargetBar.hideIndicator();
            /* @} */
            showWorkspace();
            switchWidgets(false);
        }
    }

    void changeState(Workspace.State state) {
        Animator workspaceAnim = null;
        workspaceAnim = mWorkspace.getChangeStateAnimation(state, false);
        mStateAnimation = LauncherAnimUtils.createAnimatorSet();
        if (workspaceAnim != null) {
            mStateAnimation.play(workspaceAnim);
        }
    }
    /* @} */

    /* SPRD: Feature 254280,show widget list in edit mode@{ */
    void switchWidgets(boolean showWidgets) {
        final View showView;
        final View hideView;
        if (showWidgets) {
            showView = mAppsCustomizePagedViewHost;
            hideView = mHotseat;
        } else {
            showView = mHotseat;
            hideView = mAppsCustomizePagedViewHost;
        }
        final ValueAnimator showAnim = ValueAnimator
                .ofInt(showView.getHeight());
        showAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        showAnim.setInterpolator(new Workspace.ZoomOutInterpolator());
        showAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                showView.setTranslationY(showView.getHeight()
                        - (Integer) (animation.getAnimatedValue()));
            }
        });
        final ValueAnimator hideAnim = ValueAnimator
                .ofInt(hideView.getHeight());
        hideAnim.setDuration(EXIT_SPRINGLOADED_MODE_LONG_TIMEOUT);
        hideAnim.setInterpolator(new Workspace.ZoomOutInterpolator());
        hideAnim.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                hideView.setTranslationY((Integer) (animation.getAnimatedValue()));
            }
        });
        AnimatorSet as = new AnimatorSet();
        as.addListener(new AnimatorListenerAdapter() {

            @Override
            public void onAnimationEnd(Animator animation) {
                hideView.setVisibility(View.GONE);
                showView.setVisibility(View.VISIBLE);
                /* SPRD: Fix bug 261340 @{ */
                hidePageIndicator(false);
                /* @} */
            }

            @Override
            public void onAnimationStart(Animator animation) {
                showView.setVisibility(View.VISIBLE);
                /* SPRD: Fix bug 261340 @{ */
                hidePageIndicator(true);
                /* @} */
            }
        });
        as.playTogether(showAnim, hideAnim);
        as.start();
    }

    /* @} */

    /* SPRD: Feature 253075,icon can drag in edit mode@{ */
    public boolean isEditingMode() {
        return mState == State.EDITING;
    }

    /* @} */

    /**
     * SPRD: Feature 261947,add launcher private widget. @{
     *
     * @param info
     * @param container
     * @param screenId
     * @param cell
     * @param loc
     */
    void addCustomWidget(PendingAddItemInfo info, long container, long screenId,
            int[] cell, int[] loc) {
        /* SPRD: coverity 72240 @{*/
        if (info == null) {
            Log.d(TAG,"addCustomWidget info == null");
            return;
        }
        /* @} */
        resetAddInfo();
        mPendingAddInfo.container = info.container = container;
        mPendingAddInfo.screenId = info.screenId = screenId;
        mPendingAddInfo.dropPos = loc;
        if (cell != null) {
            mPendingAddInfo.cellX = cell[0];
            mPendingAddInfo.cellY = cell[1];
        }

//        final CellLayout.CellInfo cellInfo = mAddItemCellInfo;
        CellLayout layout = getCellLayout(container, screenId);

        final int[] xy = mTmpAddItemCellCoordinates;
        final int spanX = info.spanX;
        final int spanY = info.spanY;

        if (layout != null && (!layout.findCellForSpan(xy, spanX, spanY))) {
            return;
        }

        final View view = mInflater.inflate(R.layout.widget_contact_add,
                null);
        OwnerWidgetInfo ownerInfo = new OwnerWidgetInfo();
        ownerInfo.id = info.id;
        ownerInfo.screenId = info.screenId;
        ownerInfo.cellX = info.cellX;
        ownerInfo.cellY = info.cellY;
        ownerInfo.spanX = info.spanX;
        ownerInfo.spanY = info.spanY;
        ownerInfo.container = info.container;
        /* SPRD : fix bug256366, make OWNER_WIDGET cannot be resize. @{ */
        ownerInfo.minSpanX = info.minSpanX;
        ownerInfo.minSpanY = info.minSpanY;
        /* @} */
        LauncherModel.addItemToDatabase(this, ownerInfo, container,
                screenId, xy[0], xy[1], false);
        view.setTag(ownerInfo);
        mWorkspace.addInScreen(view, container, screenId, xy[0], xy[1],
                spanX, spanY, isWorkspaceLocked());
        exitSpringLoadedDragModeDelayed(true,true, null);
    }
    /** @} */

}
