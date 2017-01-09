
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.app.ProgressDialog;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.net.Uri;
import android.os.Debug;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.provider.ContactsContract;
import android.provider.Settings;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.Groups;
import android.provider.ContactsContract.RawContacts;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import com.android.internal.telephony.EncodeException;
import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.GsmAlphabetEx;
import com.android.internal.telephony.IIccPhoneBook;
import com.android.internal.telephony.IIccPhoneBookEx;
import com.android.internal.telephony.uicc.AdnRecord;
import com.android.internal.telephony.uicc.IccConstants;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * SPRD:
 * 
 * @{
 */
public class SimUtils {
    private static final String LOG_TAG = "SimUtils";

    private static final boolean DEBUG = true;//Debug.isDebug(); sprdPorting

    private static final String ACCOUNT_TYPE_GOOGLE = "com.google";

    private static final String GOOGLE_MY_CONTACTS_GROUP = "System Group: My Contacts";

    private static final ContentValues sEmptyContentValues = new ContentValues();

    // return -1 on error
    public static int getSimCardLength(int subId) {
        int ret = -1;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(
                    ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                int[] sizes = iccIpb.getAdnRecordsSizeForSubscriber(subId, IccConstants.EF_ADN);
                if (sizes != null) {
                    if (sizes.length == 3) {
                        ret = sizes[2];
                    } else if (sizes.length == 2) {
                        ret = sizes[1] / sizes[0];
                    }
                }
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        return ret;
    }

    // return -1 on error
    // FIXME: return 12....
    public static int getSimContactNameLength(int subId) {
        int ret = -1;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(
                    ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                int[] sizes = iccIpb.getAdnRecordsSizeForSubscriber(subId, IccConstants.EF_ADN);
                int size = -1;
                if (sizes != null && sizes.length > 0) {
                    size = sizes[0] - 14;
                    if (size < 0) {
                        // get length of sim contactor's Name fail
                        return 12;
                    }
                } else {
                    return 12;
                }
                ret = size;
            } else {
                return 12;
            }
        } catch (RemoteException ex) {
            return 12;
        } catch (SecurityException ex) {
            return 12;
        }

        return ret;
    }

    /* sprd bug490245 read sne and aas for orange @{ */
    public static int getSimSneLength(int subId) {
        int ret=-1;
        try {
            IIccPhoneBookEx iccIpb = getIccPhoneBook();
            //IIccPhoneBookSprd iccIpb = getIccPhoneBookSprd();
            if (iccIpb != null) {
                int[] sizes = iccIpb.getSneLength(subId);
                if (sizes == null) {
                    Log.d(LOG_TAG, "sizes is null");
                    return 12;
                }
                Log.d(LOG_TAG, "sizes :" + sizes[0]);
                int size = -1;
                size = sizes[0] - 2; // 2byte(ADN file SFI +  ADN file RecordIdentifier)
                if (size < 0) {
                    //get length of sim contactor's sne fail
                    return 12;
                }
                ret=size;
            } else {
                return 12;
            }
        } catch (RemoteException ex) {
            return 12;
        } catch (SecurityException ex) {
            return 12;
        }
        return ret;
    }
    /* @} */

    private static IIccPhoneBookEx getIccPhoneBook() {
        return IIccPhoneBookEx.Stub.asInterface(
                ServiceManager.getService("simphonebookEx"));
    }

    // return -1 on error, and [0,..] on ok
    public static int getSimContactPhoneLength(int subId) {
        int ret = -1;
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        if (iccIpb != null) {
            try {
                ret = iccIpb.getPhoneNumMaxLen(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        if (ret <= 0) {
            ret = -1;
        }
        return ret;
    }

    // return -1 on error, and [0,..] on ok
    public static int getSimContactEmailLength(int subId) {
        int ret = -1;
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        if (iccIpb != null) {
            try {
                ret = iccIpb.getEmailMaxLen(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        if (ret <= 0) {
            ret = -1;
        }
        return ret;
    }

    // return -1 on error, and [0,..] on ok
    public static int getSimContactEmailCapacity(int subId) {
        int[] sizes = null;
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        if (iccIpb != null) {
            try {
                sizes = iccIpb.getEmailRecordsSize(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        int ret = -1;
        if (sizes != null && sizes.length > 0) {
            ret = sizes[2];
        }
        if (ret < 0) {
            ret = -1;
        }

        return ret;
    }

    public static int getSimContactPhoneTypeOverallMax(int subId) {
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        int ret = 0;
        if (iccIpb != null) {
            try {
                ret = iccIpb.getAnrNum(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        ret++;
        return ret;
    }

    public static int getSimContactEmailTypeOverallMax(int subId) {
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        int ret = 0;
        if (iccIpb != null) {
            try {
                ret = iccIpb.getEmailNum(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        if (DEBUG)
            Log.d(LOG_TAG, "EmailTypeOverallMax = " + ret);
        return ret;
    }

    public static byte[] getSimRecordBytes(String record) {
        byte[] bytes = null;
        if (record == null) {
            record = "";
        }
        try {
            bytes = GsmAlphabetEx.isAsciiStringToGsm8BitUnpackedField(record);
        } catch (EncodeException e) {
            try {
                bytes = record.getBytes("utf-16be");
            } catch (UnsupportedEncodingException e1) {
            }
        }
        return bytes;
    }

    public static int getUsimGroupNameMaxLen(int subId) {
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        int ret = 0;
        if (iccIpb != null) {
            try {
                ret = iccIpb.getUsimGroupNameMaxLen(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        if (DEBUG)
            Log.d(LOG_TAG, "UsimGroupNameMaxLen = " + ret);
        return ret;
    }

    public static int getUsimGroupCapacity(int subId) {
        IIccPhoneBookEx iccIpb = getIccPhoneBook();
        int ret = 0;
        if (iccIpb != null) {
            try {
                ret = iccIpb.getUsimGroupCapacity(subId);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        if (DEBUG)
            Log.d(LOG_TAG, "UsimGroupCapacity = " + ret);
        return ret;
    }
}
/**
 * @}
 */
