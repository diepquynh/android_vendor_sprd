package plugin.sprd.contactsEFDisplay;

import java.util.ArrayList;

import android.content.Context;
import android.content.Intent;
import android.app.AddonManager;
import android.util.Log;
import android.widget.Spinner;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.RawContactModifier;
import com.android.contacts.common.model.AccountTypeManager.SimAas;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import com.android.contacts.common.model.ValuesDelta;
import com.android.contacts.common.model.account.AccountType.EditType;
import com.android.contacts.common.model.dataitem.DataKind;
import com.android.contacts.editor.LabeledEditorView;
import com.sprd.contacts.plugin.EFDisplayUtils;

public class AddonEFDisplayUtils extends EFDisplayUtils implements
        AddonManager.InitialCallback {

    public static final String TAG = "AddonEFDisplayUtils";
    private Context mAddonContext;
    private ArrayList<EditType> mValidType;
    private EditType removeType = null;

    public AddonEFDisplayUtils() {
        mValidType = new ArrayList<EditType>();
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isOrangeSupport() {
        return true;
    }

    @Override
    public void setAasValue(Context context, ValuesDelta entry, Spinner label) {
        EditType type = (EditType) label.getItemAtPosition(label
                .getSelectedItemPosition());
        String phoneType = entry.getAsString("data2");
        Log.d(TAG, "AddonEFDisplayUtils setAasValue phoneType = " + phoneType);
        if (type != null && type.labelRes < 0) {
            Log.d(TAG, "type.labelRes = " + type.labelRes + ", type.label= "
                    + type.label);
            entry.put(Phone.TYPE, Phone.TYPE_CUSTOM);
            Log.d(TAG, "setAasValue type = " + type);
            String aasIndex = findAasIndex(context, type.label);
            entry.put(Phone.LABEL, Integer.parseInt(aasIndex) > 0 ? type.label
                    : "");
            entry.put(Phone.DATA5,
                    Integer.parseInt(aasIndex) > 0 ? String.valueOf(aasIndex)
                            : "");
        }
    }

    @Override
    public void startAasEditActivity(Context context) {
        context.startActivity(new Intent("android.intent.edit.aas"));
    }

    public void updateValidType(RawContactDelta mState, DataKind mKind,
            EditType mType) {
        mValidType.addAll(RawContactModifier
                .getValidTypes(mState, mKind, mType));
        if (mState.getValues().getAsString("account_type")
                .equals("sprd.com.android.account.usim")
                && (mKind.typeList != null && mValidType != null && (mKind.typeList
                        .size() == mValidType.size()))) {
            EditType addType = null;
            for (EditType type : mValidType) {
                if (type.rawValue == 2 && type.specificMax == 1) {
                    addType = type;
                    break;
                } else {
                    continue;
                }
            }
            Log.d(TAG, "AddonEFDisplayUtils updateValidType2 mValidType: "
                    + mValidType);
            // mValidType.clear();
            if (!mValidType.contains(addType)) {
                mValidType.add(addType);
            }
        }
    }

    public void removeTypeFromValid(final String customText, ValuesDelta mEntry) {
        Log.d(TAG, "AddonEFDisplayUtils removeTypeFromValid mValidType: "
                + mValidType);
        for (EditType valid : mValidType) {
            String custom = mEntry.getAsString(valid.customColumn);
            Log.d("LabeledEditorView", "custom == " + custom
                    + ", customText == " + customText + " valid == " + valid);
            if (valid.labelRes < 0 && valid.label.equals(customText)) {
                removeType = valid;
                Log.d("LabeledEditorView", " ,removeType == " + removeType
                        + ",valid == " + valid + " ,validType.size ==> "
                        + mValidType.size());
            }
        }
    }

    public ArrayList<EditType> getValidTypes(RawContactDelta mState,
            DataKind mKind, EditType mType) {
        return mValidType;
    }

    public void clearValidTypes() {
        mValidType.clear();
    }

    public String getText(Context context, EditType type, EditType mType,
            ValuesDelta mEntry) {
        String text = null;
        if (type == LabeledEditorView.CUSTOM_SELECTION) {
            String aasStr = mEntry.getAsString(mType.customColumn);
            String aasIndex = findAasIndex(context, aasStr);
            text = Integer.parseInt(aasIndex) > 0 ? aasStr : "NONE";
        } else if (type.labelRes < 0) {
            Log.d("LabeledEditorView", "mEntry.type.label == " + type.label);
            text = type.label;
        } else {
            text = context.getString(type.labelRes);
        }
        Log.d(TAG, "AddonEFDisplayUtils getText text: " + text);
        return text;
    }

    private String findAasIndex(Context context, String aasLabel) {
        ArrayList<SimAas> aasList = AccountTypeManager.getInstance(context)
                .getAasList();
        Log.d(TAG, "AddonEFDisplayUtils findAasIndex aasList: " + aasList);
        for (SimAas aas : aasList) {
            if (aas.name.equals(aasLabel)) {
                return aas.index;
            }
        }
        return "-1";
    }
}