package com.sprd.contacts.plugin;

import java.util.ArrayList;

import com.android.contacts.R;
import android.util.Log;
import android.view.Menu;
import android.app.AddonManager;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.RawContactModifier;
import com.android.contacts.common.model.ValuesDelta;
import com.android.contacts.common.model.account.AccountType.EditType;
import com.android.contacts.common.model.dataitem.DataKind;
import com.android.contacts.editor.LabeledEditorView;

import android.content.Context;
import android.widget.Spinner;

public class EFDisplayUtils {
    private static EFDisplayUtils sInstance;

    public static EFDisplayUtils getInstance(){
        if (sInstance != null) {
            return sInstance;
        }
        sInstance = (EFDisplayUtils)AddonManager.getDefault().getAddon(R.string.feature_ef_display, EFDisplayUtils.class);
        Log.d("EFDisplayUtils", "sInstance: " + sInstance.hashCode());
        return sInstance;
    }

    public EFDisplayUtils() {
    }

    public boolean isOrangeSupport() {
    	return false;
    }

    public void setAasValue(Context context, ValuesDelta entry, Spinner label) {
    }

    public void startAasEditActivity(Context context) {
    }

	public void updateValidType(RawContactDelta mState, DataKind mKind, EditType mType) {
	}
	
	public void removeTypeFromValid(final String customText, ValuesDelta mEntry) {
	}

	public ArrayList<EditType> getValidTypes(RawContactDelta mState,
			DataKind mKind, EditType mType) {
		return RawContactModifier.getValidTypes(mState, mKind, mType);
	}

	public String getText(Context context, EditType type, EditType mType, ValuesDelta mEntry) {
        if (type == LabeledEditorView.CUSTOM_SELECTION) {
            return mEntry.getAsString(mType.customColumn);
        } else {
            return context.getString(type.labelRes);
        }
	}

	public void clearValidTypes() {
	}
}
