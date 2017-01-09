/** Created by Spreadtrum */
package com.sprd.launcher3.widget.contacts;

import java.util.ArrayList;
import com.android.sprdlauncher1.R;
import com.sprd.launcher3.WidgetUpdate;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.FrameLayout;
import android.widget.ProgressBar;

/* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
import android.view.View.OnAttachStateChangeListener;
/* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */

public class ContactAddLayout extends FrameLayout implements WidgetUpdate,
OnClickListener, OnLongClickListener {
    private static final String TAG = "Launcher.ContactAddLayout";

    private static final boolean ISDEBUG = false;

    /**
     * Contact application's package name and class name.
     */
    private static final String CONTACT_APP_PKG_NAME = "com.android.contacts";
    private static final String CONTACT_APP_CLS_NAME = "com.android.contacts.DialtactsActivity";

    /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
    // I have no choice beside this
    private static int sInstanceCount = 0;
    /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */

    private Context mContext;

    private ContactSpace mSpace;

    private ProgressBar mProgressBar;

    /**
     * The list of contacts that will be added.
     */
    private ArrayList<ContactItemInfo> mAddItemsList;

    /**
     * Item info of the add button.
     */
    // private ContactItemInfo mLastItemInfo;

    /**
     * Contact widget should be updated when receiving a broadcast.
     */
    private BroadcastReceiver mReceiver;

    private ContactsData mData;

    private Intent mDialIntent;

    public ContactAddLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        initData();
    }

    public ContactAddLayout(Context context) {
        super(context);
        mContext = context;
        initData();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        init();
    }

    /**
     * Initialize data of this widget.
     */
    private void initData() {
        mData = ContactsData.getInstance(mContext);
        mAddItemsList = new ArrayList<ContactItemInfo>();
        mDialIntent = new Intent();
        mDialIntent.setAction(Intent.ACTION_MAIN);
        mDialIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        mDialIntent.setComponent(new ComponentName(CONTACT_APP_PKG_NAME,
                CONTACT_APP_CLS_NAME));

        registerReceiver();

        /* SPRD: bug320628 2014-06-06 call onDestroy logic error. @{ */
        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
        addOnAttachStateChangeListener(new OnAttachStateChangeListener() {

            @Override
            public void onViewAttachedToWindow(View v) {
                sInstanceCount++;
                ContactAddLayout.this.removeCallbacks(mDelayTrick);
            }

            @Override
            public void onViewDetachedFromWindow(View v) {
                sInstanceCount--;
                // delay to check this is move from one CellLayout to another
                // or delete this widget, if it is delete operation, then
                // onViewAttachedToWindow won't be called, and the onDestroy
                // will be called.
                ContactAddLayout.this.postDelayed(mDelayTrick, 1000);
            }});
        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */
    }

    private Runnable mDelayTrick = new Runnable() {
        @Override
        public void run() {
            ContactAddLayout.this.onDestory();
        }
    };
    /* SPRD: bug320628 2014-06-06 call onDestroy logic error. @} */

    private void init() {
        initViews();

        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
        // only need register once.
        if (sInstanceCount == 0) {
            // register contact observer
            mData.registerContentObserver();
        }
        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */

        // load data
        mData.updateContactsData(this, true);
    }

    /**
     * Finds all the views we need and configure them properly.
     */
    private void initViews() {
        mSpace = (ContactSpace) findViewById(R.id.contact_space);
        mProgressBar = (ProgressBar) findViewById(R.id.progressbar);

        setOnLongClickListener(this);
        mSpace.setOnLongClickListener(this);
    }

    /**
     * Register the observer of contactcursor and the broadcast receiver of
     * updating widget.
     */
    private void registerReceiver() {
        mReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {
                if (intent.getAction().equals(ContactsData.CONTACT_WIDGET_UPDATE)) {
                    notifyChange();
                } else if (intent.getAction().equals(ContactsData.CONTACT_WIDGET_UPDATE_MISS)) {
                    mSpace.configureMissedPhoneAndMsg();
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(ContactsData.CONTACT_WIDGET_UPDATE);
        filter.addAction(ContactsData.CONTACT_WIDGET_UPDATE_MISS);
        mContext.registerReceiver(mReceiver, filter, null, null);
    }

    /**
     * Update widget when data has changed.
     */
    public void notifyChange() {
        if (mAddItemsList == null) {
            mAddItemsList = new ArrayList<ContactItemInfo>();
        }
        mAddItemsList.clear();
        mAddItemsList = mData.getSelectedItemsList();
        if (ISDEBUG) {
            Log.v(TAG,
                    "notifyChange: mAddItemsList size:" + mAddItemsList.size());
        }

        mSpace.mPhotoLoader.clear();
        mSpace.updateLayout(mAddItemsList);

        setProgressBarVisibility(false);
    }

    public void setProgressBarVisibility(boolean isVisible) {
        if (isVisible) {
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mProgressBar.setVisibility(View.GONE);
        }
        postInvalidate();
    }

    @Override
    public void upAnimation() {

    }

    @Override
    public void cancelAnimation() {

    }

    /**
     * When launcher onResume and the contacts database changed start loading
     * data again.
     */
    @Override
    public void upData() {
        if (mData.hasChanged()) {
            mData.updateOnResume(this);
            mSpace.mPhotoLoader.resume();
        }
        mData.updateMissedData();
    }

    @Override
    public void onClick(View v) {
        try {
        } catch (Exception e) {
        }
    }

    /**
     * Pass up the widget's press event if we do not need it.
     */
    public boolean onLongClick(View v) {
        return performLongClick();
    }

    /**
     * When the Activity is destroyed or removed the Widget, unregister of
     * broadcast receiver and observer.
     */
    @Override
    public void onDestory() {
        // Separately unregister
        try {
            /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
            mContext.unregisterReceiver(mReceiver);
            if (sInstanceCount == 0) {
                mData.unregisterObserver();
                mData = null;
            }
            /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */
        } catch (Exception e) {
        }
        mSpace.mPhotoLoader.stop();
    }
}
