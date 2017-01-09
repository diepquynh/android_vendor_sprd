/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package plugins.sprd.Folder;

import com.android.launcher3.Folderplugins.LauncherClingsVolteUtils;
import com.android.launcher3.Launcher;
import com.android.launcher3.R;
import com.android.launcher3.*;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.app.ActivityManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.UserManager;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.accessibility.AccessibilityManager;



import android.app.AddonManager;
import android.content.Context;

import java.lang.Override;

public class LauncherClingsVolteUtilsAddon extends LauncherClingsVolteUtils implements AddonManager.InitialCallback,OnClickListener {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public void LauncherClingsVolteUtils(Launcher launcher) {
        mLauncher = launcher;
        mInflater = LayoutInflater.from(mLauncher);
    }

    public void setLauncher(Launcher launcher) {
        mLauncher = launcher;
        mInflater = LayoutInflater.from(mLauncher);
    }

    @Override
    public void onClick(View v) {
        dismissLongPressCling();
    }

    /**
     * Shows the migration cling.
     *
     * This flow is mutually exclusive with showFirstRunCling, and only runs if this Launcher
     * package was not preinstalled and there exists a db to migrate from.
     */
    @Override
    public void showLongPressCling(boolean showWelcome) {
        ViewGroup root = (ViewGroup) mLauncher.findViewById(R.id.launcher);
        View cling = mInflater.inflate(R.layout.longpress_cling_volte, root, false);

        cling.setOnLongClickListener(new OnLongClickListener() {

            @Override
            public boolean onLongClick(View v) {
                dismissLongPressCling();
                return true;
            }
        });

        final ViewGroup content = (ViewGroup) cling.findViewById(R.id.volte_cling_content);
        mInflater.inflate(showWelcome ? R.layout.longpress_cling_welcome_content_volte
                : R.layout.longpress_cling_content, content);
        content.findViewById(R.id.volte_cling_dismiss_longpress_info).setOnClickListener(this);

        if (TAG_CROP_TOP_AND_SIDES.equals(content.getTag())) {
            content.setBackgroundColor(R.color.cling_long_background);
        }

        root.addView(cling);

        if (showWelcome) {
            // This is the first cling being shown. No need to animate.
            return;
        }

        // Animate
        content.getViewTreeObserver().addOnGlobalLayoutListener(new OnGlobalLayoutListener() {

            @Override
            public void onGlobalLayout() {
                content.getViewTreeObserver().removeOnGlobalLayoutListener(this);

                ObjectAnimator anim;
                if (TAG_CROP_TOP_AND_SIDES.equals(content.getTag())) {
                    content.setTranslationY(-content.getMeasuredHeight());
                    anim = LauncherAnimUtils.ofFloat(content, "translationY", 0);
                } else {
                    content.setScaleX(0);
                    content.setScaleY(0);
                    PropertyValuesHolder scaleX = PropertyValuesHolder.ofFloat("scaleX", 1);
                    PropertyValuesHolder scaleY = PropertyValuesHolder.ofFloat("scaleY", 1);
                    anim = LauncherAnimUtils.ofPropertyValuesHolder(content, scaleX, scaleY);
                }

                anim.setDuration(SHOW_CLING_DURATION);
                anim.setInterpolator(new LogDecelerateInterpolator(100, 0));
                anim.start();
            }
        });
    }

    private void dismissLongPressCling() {
        Runnable dismissCb = new Runnable() {
            public void run() {
                dismissCling(mLauncher.findViewById(R.id.volte_longpress_cling), null,
                        VOLTE_WORKSPACE_CLING_DISMISSED_KEY, DISMISS_CLING_DURATION);
            }
        };
        mLauncher.getWorkspace().post(dismissCb);
    }

    // Hides the specified Cling
    public void dismissCling(final View cling, final Runnable postAnimationCb, final String flag, int duration) {
        // To catch cases where siblings of top-level views are made invisible,
        // just check whether
        // the cling is directly set to GONE before dismissing it.
        if (cling != null && cling.getVisibility() != View.GONE) {
            final Runnable cleanUpClingCb = new Runnable() {
                public void run() {
                    cling.setVisibility(View.GONE);
                    mSharedPrefs = mLauncher.getSharedPreferences(SHARED_PREFERENCES_VOLTE_KEY, Context.MODE_PRIVATE);
                    mSharedPrefs.edit().putBoolean(flag, true).apply();
                    if (postAnimationCb != null) {
                        postAnimationCb.run();
                    }
                }
            };
            if (duration <= 0) {
                cleanUpClingCb.run();
            } else {
                cling.animate().alpha(0).setDuration(duration).withEndAction(cleanUpClingCb);
            }
        }
    }

    // Returns whether the clings are enabled or should be shown
    private boolean areClingsEnabled() {
        if (DISABLE_CLINGS) {
            return false;
        }

        // disable clings when running in a test harness
        if (ActivityManager.isRunningInTestHarness())
            return false;

        // Disable clings for accessibility when explore by touch is enabled
        final AccessibilityManager a11yManager = (AccessibilityManager) mLauncher
                .getSystemService(Launcher.ACCESSIBILITY_SERVICE);
        if (a11yManager.isTouchExplorationEnabled()) {
            return false;
        }

        // Restricted secondary users (child mode) will potentially have very
        // few apps
        // seeded when they start up for the first time. Clings won't work well
        // with that
        boolean supportsLimitedUsers = android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;
        Account[] accounts = AccountManager.get(mLauncher).getAccounts();
        if (supportsLimitedUsers && accounts.length == 0) {
            UserManager um = (UserManager) mLauncher.getSystemService(Context.USER_SERVICE);
            Bundle restrictions = um.getUserRestrictions();
            if (restrictions.getBoolean(UserManager.DISALLOW_MODIFY_ACCOUNTS, false)) {
                return false;
            }
        }
        if (Settings.Secure.getInt(mLauncher.getContentResolver(), SKIP_FIRST_USE_HINTS, 0) == 1) {
            return false;
        }
        return true;
    }

    @Override
    public boolean shouldShowFirstRunOrMigrationClings() {
        mSharedPrefs = mLauncher.getSharedPreferences(SHARED_PREFERENCES_VOLTE_KEY, Context.MODE_PRIVATE);
        return areClingsEnabled() && !mSharedPrefs.getBoolean(VOLTE_WORKSPACE_CLING_DISMISSED_KEY, false)
                && !mSharedPrefs.getBoolean(VOLTE_MIGRATION_CLING_DISMISSED_KEY, false);
    }

    // TO-DO static
    public static void synchonouslyMarkFirstRunClingDismissed(Context ctx) {
        SharedPreferences prefs = ctx.getSharedPreferences(LauncherAppState.getSharedPreferencesKey(),
                Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(VOLTE_WORKSPACE_CLING_DISMISSED_KEY, true);
        editor.commit();
    }
}
