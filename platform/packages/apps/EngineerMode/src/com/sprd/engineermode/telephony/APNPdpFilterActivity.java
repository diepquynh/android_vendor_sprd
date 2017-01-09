
package com.sprd.engineermode.telephony;

import java.lang.reflect.Method;

import android.content.Context;
import android.os.Bundle;

import android.preference.CheckBoxPreference;

import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;

import android.util.Log;
import android.telephony.TelephonyManager;

import android.os.RemoteException;
import android.content.Intent;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class APNPdpFilterActivity extends PreferenceActivity {

    private String LOG_TAG = "APNPdpFilterActivity";
    private boolean mChecked = false;
    private CheckBoxPreference mFilterAll;
    private CheckBoxPreference mFilterDefault;
    private CheckBoxPreference mFilterMms;
    private CheckBoxPreference mFilterSupl;
    private CheckBoxPreference mFilterDun;
    private CheckBoxPreference mFilterHipri;

    private CheckBoxPreference mFilterAll2;
    private CheckBoxPreference mFilterDefault2;
    private CheckBoxPreference mFilterMms2;
    private CheckBoxPreference mFilterSupl2;
    private CheckBoxPreference mFilterDun2;
    private CheckBoxPreference mFilterHipri2;

    private CheckBoxPreference mFilterAll3;
    private CheckBoxPreference mFilterDefault3;
    private CheckBoxPreference mFilterMms3;
    private CheckBoxPreference mFilterSupl3;
    private CheckBoxPreference mFilterDun3;
    private CheckBoxPreference mFilterHipri3;

    static final String PHONE_ID2 = "2";
    static final String PHONE_ID3 = "3";
    static final String APN_TYPE_ALL = "all";
    /** APN type for default data traffic */
    static final String APN_TYPE_DEFAULT = "default";
    /** APN type for MMS traffic */
    static final String APN_TYPE_MMS = "mms";
    /** APN type for SUPL assisted GPS */
    static final String APN_TYPE_SUPL = "supl";
    /** APN type for DUN traffic */
    static final String APN_TYPE_DUN = "dun";
    /** APN type for HiPri traffic */
    static final String APN_TYPE_HIPRI = "hipri";

    static final String APN_TYPE_ALL2 = "all2";
    /** APN type for default data traffic */
    static final String APN_TYPE_DEFAULT2 = "default2";
    /** APN type for MMS traffic */
    static final String APN_TYPE_MMS2 = "mms2";
    /** APN type for SUPL assisted GPS */
    static final String APN_TYPE_SUPL2 = "supl2";
    /** APN type for DUN traffic */
    static final String APN_TYPE_DUN2 = "dun2";
    /** APN type for HiPri traffic */
    static final String APN_TYPE_HIPRI2 = "hipri2";
    private static final String APN_QUERY = "apnquery";

    static final String APN_TYPE_ALL3 = "all3";
    /** APN type for default data traffic */
    static final String APN_TYPE_DEFAULT3 = "default3";
    /** APN type for MMS traffic */
    static final String APN_TYPE_MMS3 = "mms3";
    /** APN type for SUPL assisted GPS */
    static final String APN_TYPE_SUPL3 = "supl3";
    /** APN type for DUN traffic */
    static final String APN_TYPE_DUN3 = "dun3";
    /** APN type for HiPri traffic */
    static final String APN_TYPE_HIPRI3 = "hipri3";
    static final String APN_FILTERSELECT3 = "ApnpdpFilterSelect3";

    static final String ALL_TYPE_APN = "*";
    static final String ALL_TYPE_APN2 = "*";
    private TelephonyManager mTelephonyManager1;
    private TelephonyManager mTelephonyManager2;
    private TelephonyManager mTelephonyManager3;
    //private ITelephony mTelephony1;
//    private ITelephony mTelephony2 = null;
//    private ITelephony mTelephony3 = null;
    boolean mFilterAllStatus;
    boolean mFilterDefaultStatus;
    boolean mFilterMmsStatus;
    boolean mFilterSuplStatus;
    boolean mFilterDunStatus;
    boolean mFilterHipriStatus;

    boolean mFilterAllStatus2;
    boolean mFilterDefaultStatus2;
    boolean mFilterMmsStatus2;
    boolean mFilterSuplStatus2;
    boolean mFilterDunStatus2;
    boolean mFilterHipriStatus2;

    boolean mFilterAllStatus3;
    boolean mFilterDefaultStatus3;
    boolean mFilterMmsStatus3;
    boolean mFilterSuplStatus3;
    boolean mFilterDunStatus3;
    boolean mFilterHipriStatus3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        int simCount = TelephonyManager.from(this).getPhoneCount();
        Log.d(LOG_TAG, "simCount:" + simCount);
        addPreferencesFromResource(R.xml.pref_apn_settings);
        if (simCount == 2) {
            mTelephonyManager1 = (TelephonyManager) TelephonyManager.from(this);
            mTelephonyManager2 = (TelephonyManager) TelephonyManager.from(this);
            mTelephonyManager3 = null;
          //  mTelephony2 = getITelephony(this, mTelephonyManager2);
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_ALL3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_DEFAULT3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_MMS3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_SUPL3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_DUN3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_HIPRI3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_FILTERSELECT3));
        } else if (simCount == 1) {
            mTelephonyManager1 = (TelephonyManager) TelephonyManager.from(this);
            mTelephonyManager2 = null;
            mTelephonyManager3 = null;
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_ALL2));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_DEFAULT2));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_MMS2));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_SUPL2));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_DUN2));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_HIPRI2));
            this.getPreferenceScreen().removePreference(this.findPreference("ApnpdpFilterSelect2"));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_ALL3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_DEFAULT3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_MMS3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_SUPL3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_DUN3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_TYPE_HIPRI3));
            this.getPreferenceScreen().removePreference(this.findPreference(APN_FILTERSELECT3));
        } else if (simCount == 3) {
            mTelephonyManager1 = (TelephonyManager) TelephonyManager.from(this);
            mTelephonyManager2 = (TelephonyManager) TelephonyManager.from(this);
            mTelephonyManager3 = (TelephonyManager) TelephonyManager.from(this);
            //mTelephony2 = getITelephony(this, mTelephonyManager2);
            //mTelephony3 = getITelephony(this, mTelephonyManager3);
        }
       // mTelephony1 = getITelephony(this, mTelephonyManager1);
        mFilterAll = (CheckBoxPreference) findPreference(APN_TYPE_ALL);
        mFilterDefault = (CheckBoxPreference) findPreference(APN_TYPE_DEFAULT);
        mFilterMms = (CheckBoxPreference) findPreference(APN_TYPE_MMS);
        mFilterSupl = (CheckBoxPreference) findPreference(APN_TYPE_SUPL);
        mFilterDun = (CheckBoxPreference) findPreference(APN_TYPE_DUN);
        mFilterHipri = (CheckBoxPreference) findPreference(APN_TYPE_HIPRI);
        if (mTelephonyManager2 != null) {
            mFilterAll2 = (CheckBoxPreference) findPreference(APN_TYPE_ALL2);
            mFilterDefault2 = (CheckBoxPreference) findPreference(APN_TYPE_DEFAULT2);
            mFilterMms2 = (CheckBoxPreference) findPreference(APN_TYPE_MMS2);
            mFilterSupl2 = (CheckBoxPreference) findPreference(APN_TYPE_SUPL2);
            mFilterDun2 = (CheckBoxPreference) findPreference(APN_TYPE_DUN2);
            mFilterHipri2 = (CheckBoxPreference) findPreference(APN_TYPE_HIPRI2);
        }
        if (mTelephonyManager3 != null) {
            mFilterAll3 = (CheckBoxPreference) findPreference(APN_TYPE_ALL3);
            mFilterDefault3 = (CheckBoxPreference) findPreference(APN_TYPE_DEFAULT3);
            mFilterMms3 = (CheckBoxPreference) findPreference(APN_TYPE_MMS3);
            mFilterSupl3 = (CheckBoxPreference) findPreference(APN_TYPE_SUPL3);
            mFilterDun3 = (CheckBoxPreference) findPreference(APN_TYPE_DUN3);
            mFilterHipri3 = (CheckBoxPreference) findPreference(APN_TYPE_HIPRI3);
        }

        //updateApnFilterState();

    }

   /* void updateApnFilterState() {
        Log.v("ApnSetting", "updateApnFilterState() mTelephony1 = " + mTelephony1
                + " , mTelephony2 = " + mTelephony2 + ", mTelephony3 = " + mTelephony3);
        try {
            mFilterAllStatus = mTelephony1.getApnActivePdpFilter(ALL_TYPE_APN);
            mFilterDefaultStatus = mTelephony1.getApnActivePdpFilter(APN_TYPE_DEFAULT);
            mFilterMmsStatus = mTelephony1.getApnActivePdpFilter(APN_TYPE_MMS);
            mFilterSuplStatus = mTelephony1.getApnActivePdpFilter(APN_TYPE_SUPL);
            mFilterDunStatus = mTelephony1.getApnActivePdpFilter(APN_TYPE_DUN);
            mFilterHipriStatus = mTelephony1.getApnActivePdpFilter(APN_TYPE_HIPRI);
            if (mTelephonyManager2 != null) {
                mFilterAllStatus2 = mTelephony2.getApnActivePdpFilter(ALL_TYPE_APN);
                mFilterDefaultStatus2 = mTelephony2.getApnActivePdpFilter(APN_TYPE_DEFAULT);
                mFilterMmsStatus2 = mTelephony2.getApnActivePdpFilter(APN_TYPE_MMS);
                mFilterSuplStatus2 = mTelephony2.getApnActivePdpFilter(APN_TYPE_SUPL);
                mFilterDunStatus2 = mTelephony2.getApnActivePdpFilter(APN_TYPE_DUN);
                mFilterHipriStatus2 = mTelephony2.getApnActivePdpFilter(APN_TYPE_HIPRI);
            }

            if (mTelephonyManager3 != null) {
                mFilterAllStatus3 = mTelephony3.getApnActivePdpFilter(ALL_TYPE_APN);
                mFilterDefaultStatus3 = mTelephony3.getApnActivePdpFilter(APN_TYPE_DEFAULT);
                mFilterMmsStatus3 = mTelephony3.getApnActivePdpFilter(APN_TYPE_MMS);
                mFilterSuplStatus3 = mTelephony3.getApnActivePdpFilter(APN_TYPE_SUPL);
                mFilterDunStatus3 = mTelephony3.getApnActivePdpFilter(APN_TYPE_DUN);
                mFilterHipriStatus3 = mTelephony3.getApnActivePdpFilter(APN_TYPE_HIPRI);
            }

        } catch (RemoteException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        }

        Log.v("ApnActivepdpFilter", "mFilterAllStatus = " + mFilterAllStatus2
                + " , and mFilterMmsStatus = " + mFilterMmsStatus2);
        if (mFilterDefaultStatus) {
            mFilterDefault.setChecked(true);
            mFilterDefault.setSummary("enable filter");
        } else {
            mFilterDefault.setChecked(false);
            mFilterDefault.setSummary("disable filter");
        }

        if (mFilterMmsStatus) {
            mFilterMms.setChecked(true);
            mFilterMms.setSummary("enable filter");
        } else {
            mFilterMms.setChecked(false);
            mFilterMms.setSummary("disable filter");
        }

        if (mFilterSuplStatus) {
            mFilterSupl.setChecked(true);
            mFilterSupl.setSummary("enable filter");
        } else {
            mFilterSupl.setChecked(false);
            mFilterSupl.setSummary("disable filter");
        }

        if (mFilterDunStatus) {
            mFilterDun.setChecked(true);
            mFilterDun.setSummary("enable filter");

        } else {
            mFilterDun.setChecked(false);
            mFilterDun.setSummary("disable filter");
        }

        if (mFilterHipriStatus) {
            mFilterHipri.setChecked(true);
            mFilterHipri.setSummary("enable filter");
        } else {
            mFilterHipri.setChecked(false);
            mFilterHipri.setSummary("disable filter");
        }

        if (mFilterAllStatus) {
            mFilterAll.setChecked(true);
            mFilterAll.setSummary("enable filter");
            mFilterDefault.setEnabled(false);
            mFilterMms.setEnabled(false);
            mFilterSupl.setEnabled(false);
            mFilterDun.setEnabled(false);
            mFilterHipri.setEnabled(false);
        } else {
            mFilterAll.setChecked(false);
            mFilterAll.setSummary("disable filter");
            mFilterDefault.setEnabled(true);
            mFilterMms.setEnabled(true);
            mFilterSupl.setEnabled(true);
            mFilterDun.setEnabled(true);
            mFilterHipri.setEnabled(true);
        }

        if (mTelephonyManager2 != null) {
            if (mFilterDefaultStatus2) {
                mFilterDefault2.setChecked(true);
                mFilterDefault2.setSummary("enable filter");
            } else {
                mFilterDefault2.setChecked(false);
                mFilterDefault2.setSummary("disable filter");
            }

            if (mFilterMmsStatus2) {
                mFilterMms2.setChecked(true);
                mFilterMms2.setSummary("enable filter");
            } else {
                mFilterMms2.setChecked(false);
                mFilterMms2.setSummary("disable filter");
            }

            if (mFilterSuplStatus2) {
                mFilterSupl2.setChecked(true);
                mFilterSupl2.setSummary("enable filter");
            } else {
                mFilterSupl2.setChecked(false);
                mFilterSupl2.setSummary("disable filter");
            }

            if (mFilterDunStatus2) {
                mFilterDun2.setChecked(true);
                mFilterDun2.setSummary("enable filter");

            } else {
                mFilterDun2.setChecked(false);
                mFilterDun2.setSummary("disable filter");
            }

            if (mFilterHipriStatus2) {
                mFilterHipri2.setChecked(true);
                mFilterHipri2.setSummary("enable filter");
            } else {
                mFilterHipri2.setChecked(false);
                mFilterHipri2.setSummary("disable filter");
            }

            if (mFilterAllStatus2) {
                mFilterAll2.setChecked(true);
                mFilterAll2.setSummary("enable filter");
                mFilterDefault2.setEnabled(false);
                mFilterMms2.setEnabled(false);
                mFilterSupl2.setEnabled(false);
                mFilterDun2.setEnabled(false);
                mFilterHipri2.setEnabled(false);
            } else {
                mFilterAll2.setChecked(false);
                mFilterAll2.setSummary("disable filter");
                mFilterDefault2.setEnabled(true);
                mFilterMms2.setEnabled(true);
                mFilterSupl2.setEnabled(true);
                mFilterDun2.setEnabled(true);
                mFilterHipri2.setEnabled(true);
            }
        }
        if (mTelephonyManager3 != null) {
            if (mFilterDefaultStatus3) {
                mFilterDefault3.setChecked(true);
                mFilterDefault3.setSummary("enable filter");
            } else {
                mFilterDefault3.setChecked(false);
                mFilterDefault3.setSummary("disable filter");
            }

            if (mFilterMmsStatus3) {
                mFilterMms3.setChecked(true);
                mFilterMms3.setSummary("enable filter");
            } else {
                mFilterMms3.setChecked(false);
                mFilterMms3.setSummary("disable filter");
            }

            if (mFilterSuplStatus3) {
                mFilterSupl3.setChecked(true);
                mFilterSupl3.setSummary("enable filter");
            } else {
                mFilterSupl3.setChecked(false);
                mFilterSupl3.setSummary("disable filter");
            }

            if (mFilterDunStatus3) {
                mFilterDun3.setChecked(true);
                mFilterDun3.setSummary("enable filter");

            } else {
                mFilterDun3.setChecked(false);
                mFilterDun3.setSummary("disable filter");
            }

            if (mFilterHipriStatus3) {
                mFilterHipri3.setChecked(true);
                mFilterHipri3.setSummary("enable filter");
            } else {
                mFilterHipri3.setChecked(false);
                mFilterHipri3.setSummary("disable filter");
            }

            if (mFilterAllStatus3) {
                mFilterAll3.setChecked(true);
                mFilterAll3.setSummary("enable filter");
                mFilterDefault3.setEnabled(false);
                mFilterMms3.setEnabled(false);
                mFilterSupl3.setEnabled(false);
                mFilterDun3.setEnabled(false);
                mFilterHipri3.setEnabled(false);
            } else {
                mFilterAll3.setChecked(false);
                mFilterAll3.setSummary("disable filter");
                mFilterDefault3.setEnabled(true);
                mFilterMms3.setEnabled(true);
                mFilterSupl3.setEnabled(true);
                mFilterDun3.setEnabled(true);
                mFilterHipri3.setEnabled(true);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateApnFilterState();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        if (preference instanceof CheckBoxPreference) {
            String key = preference.getKey();
            mChecked = ((CheckBoxPreference) preference).isChecked();

            Log.i(LOG_TAG, "onPreferenceChange(), " + key + mChecked);

            Log.v("ApnSetting", "key1 = " + key + "and contains 2 = " + key.contains("2"));
            if (key.contains(PHONE_ID2)) {
                try {
                    if (APN_TYPE_ALL2.equals(key)) {
                        key = ALL_TYPE_APN;
                    } else if (APN_TYPE_DEFAULT2.equals(key)) {
                        key = APN_TYPE_DEFAULT;
                    } else if (APN_TYPE_MMS2.equals(key)) {
                        key = APN_TYPE_MMS;
                    } else if (APN_TYPE_SUPL2.equals(key)) {
                        key = APN_TYPE_SUPL;
                    } else if (APN_TYPE_DUN2.equals(key)) {
                        key = APN_TYPE_DUN;
                    } else {
                        key = APN_TYPE_HIPRI;
                    }
                    mTelephony2.setApnActivePdpFilter(key, mChecked);
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (SecurityException e) {
                    e.printStackTrace();
                }
            } else if (key.contains(PHONE_ID3)) {
                try {
                    if (APN_TYPE_ALL3.equals(key)) {
                        key = ALL_TYPE_APN;
                    } else if (APN_TYPE_DEFAULT3.equals(key)) {
                        key = APN_TYPE_DEFAULT;
                    } else if (APN_TYPE_MMS3.equals(key)) {
                        key = APN_TYPE_MMS;
                    } else if (APN_TYPE_SUPL3.equals(key)) {
                        key = APN_TYPE_SUPL;
                    } else if (APN_TYPE_DUN3.equals(key)) {
                        key = APN_TYPE_DUN;
                    } else {
                        key = APN_TYPE_HIPRI;
                    }
                    mTelephony3.setApnActivePdpFilter(key, mChecked);
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (SecurityException e) {
                    e.printStackTrace();
                }
            } else {
                try {
                    if (APN_TYPE_ALL.equals(key)) {
                        key = ALL_TYPE_APN;
                    }
                    mTelephony1.setApnActivePdpFilter(key, mChecked);
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (SecurityException e) {
                    e.printStackTrace();
                }
            }

            updateApnFilterState();
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private static ITelephony getITelephony(Context context, TelephonyManager mTelephonyManager) {
        Class<TelephonyManager> c = TelephonyManager.class;
        Method getITelephonyMethod = null;
        ITelephony iTelephony = null;
        try {
            getITelephonyMethod = c.getDeclaredMethod("getITelephony", (Class[]) null);
            getITelephonyMethod.setAccessible(true);
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }

        try {
            iTelephony = (ITelephony) getITelephonyMethod
                    .invoke(mTelephonyManager, (Object[]) null);
            return iTelephony;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return iTelephony;
    }*/

}
