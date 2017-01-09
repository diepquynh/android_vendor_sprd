/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 *  Copyright (C) 2009 The Android Open Source Project
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

package com.ucamera.uphoto.preference;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import com.ucamera.uphoto.R;

public class MyDialogPreference extends Preference  implements
        DialogInterface.OnClickListener, DialogInterface.OnDismissListener,
        PreferenceManager.OnActivityDestroyListener {
    private static final String TAG = "MyDialogPreference";

    private MyAlertDialog mBuilder;

    private CharSequence mDialogTitle;
    private Drawable mDialogIcon;
    private int mDialogLayoutResId;

    protected CharSequence[] mEntries;
    protected CharSequence[] mEntryValues;

    private Resources mResources;

    private int[] mIcons;

    private int[] mSelectedIcons;

    private String mValue;
    protected int mClickedDialogEntryIndex;

    /** The dialog, if it is showing. */
    private Dialog mDialog;

    public MyDialogPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mResources = context.getResources();

        mDialogLayoutResId = R.layout.custom_alert_dialog;
    }

    public MyDialogPreference(Context context) {
        this(context, null);
    }

    /**
     * Sets the title of the dialog. This will be shown on subsequent dialogs.
     *
     * @param dialogTitle The title.
     */
    public void setDialogTitle(CharSequence dialogTitle) {
        mDialogTitle = dialogTitle;
    }

    /**
     * @see #setDialogTitle(CharSequence)
     * @param dialogTitleResId The dialog title as a resource.
     */
    public void setDialogTitle(int dialogTitleResId) {
        setDialogTitle(getContext().getString(dialogTitleResId));
    }

    /**
     * Returns the title to be shown on subsequent dialogs.
     * @return The title.
     */
    public CharSequence getDialogTitle() {
        return mDialogTitle;
    }

    /**
     * Sets the icon of the dialog. This will be shown on subsequent dialogs.
     *
     * @param dialogIcon The icon, as a {@link Drawable}.
     */
    public void setDialogIcon(Drawable dialogIcon) {
        mDialogIcon = dialogIcon;
    }

    /**
     * Sets the icon (resource ID) of the dialog. This will be shown on
     * subsequent dialogs.
     *
     * @param dialogIconRes The icon, as a resource ID.
     */
    public void setDialogIcon(int dialogIconRes) {
        mDialogIcon = getContext().getResources().getDrawable(dialogIconRes);
    }

    /**
     * Returns the icon to be shown on subsequent dialogs.
     * @return The icon, as a {@link Drawable}.
     */
    public Drawable getDialogIcon() {
        return mDialogIcon;
    }

    /**
     * Sets the layout resource that is inflated as the {@link View} to be shown
     * as the content View of subsequent dialogs.
     *
     * @param dialogLayoutResId The layout resource ID to be inflated.
     * @see #setDialogMessage(CharSequence)
     */
    public void setDialogLayoutResource(int dialogLayoutResId) {
        mDialogLayoutResId = dialogLayoutResId;
    }

    /**
     * Returns the layout resource that is used as the content View for
     * subsequent dialogs.
     *
     * @return The layout resource.
     */
    public int getDialogLayoutResource() {
        return mDialogLayoutResId;
    }

    public void setEntries(CharSequence[] entries) {
        mEntries = entries;
    }

    /**
     * @see #setEntries(CharSequence[])
     * @param entriesResId The entries array as a resource.
     */
    public void setEntries(int entriesResId) {
        setEntries(mResources.getTextArray(entriesResId));
    }

    /**
     * The list of entries to be shown in the list in subsequent dialogs.
     *
     * @return The list as an array.
     */
    public CharSequence[] getEntries() {
        return mEntries;
    }

    /**
     *
     * @param icons icons
     */
    public void setIconIds(int[] icons) {
        mIcons = icons;
    }

    /**
     * The list of icons to be shown in the list in subsequent dialogs.
     *
     * @return The list as an array.
     */
    public int[] getIconIds() {
        return mIcons;
    }

    /**
     *
     * @param icons icons
     */
    public void setSelectedIconIds(int[] icons) {
        mSelectedIcons = icons;
    }

    /**
     * The list of selected icons to be shown in the list in subsequent dialogs.
     *
     * @return The list as an array.
     */
    public int[] getSelectedIconIds() {
        return mSelectedIcons;
    }

    /**
     * The array to find the value to save for a preference when an entry from
     * entries is selected. If a user clicks on the second item in entries, the
     * second item in this array will be saved to the preference.
     *
     * @param entryValues The array to be used as values to save for the preference.
     */
    public void setEntryValues(CharSequence[] entryValues) {
        mEntryValues = entryValues;
    }

    /**
     * @see #setEntryValues(CharSequence[])
     * @param entryValuesResId The entry values array as a resource.
     */
    public void setEntryValues(int entryValuesResId) {
        setEntryValues(mResources.getTextArray(entryValuesResId));
    }

    /**
     * Returns the array of values to be saved for the preference.
     *
     * @return The array of values.
     */
    public CharSequence[] getEntryValues() {
        return mEntryValues;
    }

    /**
     * Sets the value of the key. This should be one of the entries in
     * {@link #getEntryValues()}.
     *
     * @param value The value to set for the key.
     */
    public void setValue(String value) {
        mValue = value;

        persistString(value);
    }

    /**
     * Sets the value to the given index from the entry values.
     *
     * @param index The index of the value to set.
     */
    public void setValueIndex(int index) {
        if (mEntryValues != null) {
            setValue(mEntryValues[index].toString());
        }
    }

    /**
     * Returns the value of the key. This should be one of the entries in
     * {@link #getEntryValues()}.
     *
     * @return The value of the key.
     */
    public String getValue() {
        return mValue;
    }

    /**
     * Returns the entry corresponding to the current value.
     *
     * @return The entry corresponding to the current value, or null.
     */
    public CharSequence getEntry() {
        int index = getValueIndex();
        return index >= 0 && mEntries != null ? mEntries[index] : null;
    }

    protected int getValueIndex() {
        return findIndexOfValue(mValue);
    }

    /**
     * Returns the index of the given value (in the entry values array).
     *
     * @param value The value whose index should be returned.
     * @return The index of the value, or -1 if not found.
     */
    public int findIndexOfValue(String value) {
        if (value != null && mEntryValues != null) {
            for (int i = mEntryValues.length - 1; i >= 0; i--) {
                if (mEntryValues[i].equals(value)) {
                    return i;
                }
            }
        }
        return -1;
    }

    /**
     * Prepares the dialog builder to be shown when the preference is clicked.
     * Use this to set custom properties on the dialog.
     * <p>
     * Do not {@link AlertDialog.Builder#create()} or
     * {@link AlertDialog.Builder#show()}.
     */
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
    }

    @Override
    protected void onClick() {
        showDialog(null);
    }

    /**
     * Shows the dialog associated with this Preference. This is normally initiated
     * automatically on clicking on the preference. Call this method if you need to
     * show the dialog on some other event.
     *
     * @param state Optional instance state to restore on the dialog
     */
    protected void showDialog(Bundle state) {
        Context context = getContext();

        mClickedDialogEntryIndex = getValueIndex();

        mBuilder = new MyAlertDialog(context);
        View contentView = onCreateDialogView();
        if (contentView != null) {
            TextView textviewTitle = (TextView) contentView.findViewById(R.id.dialog_title);
            textviewTitle.setText(getTitle());

            onBindDialogView(contentView);
        }

//        getPreferenceManager().registerOnActivityDestroyListener(this);

        // Create the dialog
        final Dialog dialog = mDialog = mBuilder;
        if (state != null) {
            dialog.onRestoreInstanceState(state);
        }
        if (needInputMethod()) {
            requestInputMethod(dialog);
        }
        dialog.setOnDismissListener(this);
        dialog.show();

    }

    /**
     * Creates the content view for the dialog (if a custom content view is
     * required). By default, it inflates the dialog layout resource if it is
     * set.
     *
     * @return The content View for the dialog.
     * @see #setLayoutResource(int)
     */
    protected View onCreateDialogView() {
        View view = mBuilder.getContentView();

        return view;
    }

    /**
     * Binds views in the content View of the dialog to data.
     * <p>
     * Make sure to call through to the superclass implementation.
     *
     * @param view The content View of the dialog, if it is custom.
     */
    protected void onBindDialogView(View view) {
    }

    /**
     * Returns whether the preference needs to display a soft input method when the dialog
     * is displayed. Default is false. Subclasses should override this method if they need
     * the soft input method brought up automatically.
     * @hide
     */
    protected boolean needInputMethod() {
        return false;
    }

    /**
     * Sets the required flags on the dialog window to enable input method window to show up.
     */
    private void requestInputMethod(Dialog dialog) {
        Window window = dialog.getWindow();
        window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE |
                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
    }

    public void onClick(DialogInterface dialog, int which) {
//        mWhichButtonClicked = which;
    }

    public void onDismiss(DialogInterface dialog) {
//        getPreferenceManager().unregisterOnActivityDestroyListener(this);

        mDialog = null;
        mBuilder = null;
        onDialogClosed(true);
    }

    /**
     * Called when the dialog is dismissed and should be used to save data to
     * the {@link SharedPreferences}.
     *
     * @param positiveResult Whether the positive button was clicked (true), or
     *            the negative button was clicked or the dialog was canceled (false).
     */
    protected void onDialogClosed(boolean positiveResult) {
        if (mClickedDialogEntryIndex >= 0 && mEntryValues != null) {
            String value = mEntryValues[mClickedDialogEntryIndex].toString();

            boolean bResult = callChangeListener(value);
            Log.d(TAG, "onDialogClosed: value = " + value + ", callChangeListener(value) = "
                    + bResult);
            if (bResult) {
                setValue(value);
            }
        }
    }

    public MyAlertDialog getAlertDialog() {
        return mBuilder;
    }

    /**
     * Gets the dialog that is shown by this preference.
     *
     * @return The dialog, or null if a dialog is not being shown.
     */
    public Dialog getDialog() {
        return mDialog;
    }

    /**
     * {@inheritDoc}
     */
    public void onActivityDestroy() {

        if (mDialog == null || !mDialog.isShowing()) {
            return;
        }

        mDialog.dismiss();
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();
        if (mDialog == null || !mDialog.isShowing()) {
            return superState;
        }

        final SavedState myState = new SavedState(superState);
        myState.isDialogShowing = true;
        myState.dialogBundle = mDialog.onSaveInstanceState();
        return myState;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (state == null || !state.getClass().equals(SavedState.class)) {
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState(state);
            return;
        }

        SavedState myState = (SavedState) state;
        super.onRestoreInstanceState(myState.getSuperState());
        if (myState.isDialogShowing) {
            showDialog(myState.dialogBundle);
        }
    }

    private static class SavedState extends BaseSavedState {
        boolean isDialogShowing;
        Bundle dialogBundle;

        public SavedState(Parcel source) {
            super(source);
            isDialogShowing = source.readInt() == 1;
            dialogBundle = source.readBundle();
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeInt(isDialogShowing ? 1 : 0);
            dest.writeBundle(dialogBundle);
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }

}
