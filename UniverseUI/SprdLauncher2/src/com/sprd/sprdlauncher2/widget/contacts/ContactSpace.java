/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.widget.contacts;

import java.util.ArrayList;
import java.util.Map;

import com.android.sprdlauncher2.R;
import com.android.sprdlauncher2.Launcher;


import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.QuickContact;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Interpolator;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.QuickContactBadge;
import android.widget.Scroller;
import android.widget.TextView;
import android.widget.Toast;

import android.sim.Sim;
import android.sim.SimManager;
import com.android.internal.telephony.TelephonyIntents;

public class ContactSpace extends ViewGroup implements View.OnClickListener,
View.OnLongClickListener {
    // SPRD: coverity 82708
    public static final String TAG = "ContactSpace";

    private static final float BASELINE_FLING_VELOCITY = 2500.f;

    private static final float FLING_VELOCITY_INFLUENCE = 0.4f;

    private static final String SCROLL_DIRECTION = "V";

    private static final String CALL_SETTINGS_CLASS_NAME = "com.android.phone.CallFeaturesSetting";
    private static final String PHONE_PACKAGE = "com.android.phone";

    private Context mContext;

    private Scroller mScroller;

    private WorkspaceOvershootInterpolator mScrollInterpolator;

    private ArrayList<ContactItemInfo> mAllContactsList;

    public ContactPhotoLoader mPhotoLoader;

    public int mXCount = 3;

    public int mYCount = 3;

    public boolean mLockWorkSpace = false;

//    private Launcher mLauncher;

    public ContactSpace(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        init();
    }

    public ContactSpace(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mContext = context;
        init();
    }

    /**
     * Initializes various states for this space.
     */
    private void init() {
        mScrollInterpolator = new WorkspaceOvershootInterpolator();
        mScroller = new Scroller(mContext, mScrollInterpolator);
        ContactCellLayout.setContactCellCount(mXCount, mYCount);
        mAllContactsList = new ArrayList<ContactItemInfo>();
        mPhotoLoader = new ContactPhotoLoader(mContext,
                R.drawable.widgets_desktopcontact_icon_noperson);
    }

    public void updateLayout(ArrayList<ContactItemInfo> allList) {
        mAllContactsList = allList;
        initLayout();
    }

    private void initLayout() {
        if (mAllContactsList == null || mAllContactsList.size() == 0) {
            return;
        }
        // add new view
        removeAllViews();
        ContactCellLayout cellLayout = new ContactCellLayout(mContext);
        cellLayout.setOnLongClickListener(this);
        cellLayout.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
                LayoutParams.FILL_PARENT));
        addView(cellLayout, 0);
        setViewInLayout(mAllContactsList);
        configureMissedPhoneAndMsg();
    }

    private void setViewInLayout(ArrayList<ContactItemInfo> allContactsList) {
        int size = allContactsList.size();
        for (int i = 0; i < mYCount * mXCount; i++) {
            if (i >= size) {
                break;
            }
            ContactItemInfo contactInfo = allContactsList.get(i);
            View view = initCellView(contactInfo);
            /* SPRD: coverity 82708 @{*/
            if (view != null) {
                addInScreen(view, ((ContactItemInfo) view.getTag()).mFastNumber - 1);
            } else {
                Log.e(TAG, "setViewInLayout() the view is null");
            }
            /* @} */
        }
    }

    private View initCellView(final ContactItemInfo contactInfo) {
        /* SPRD: coverity 72244 @{*/
        if (contactInfo != null) {
            View addItemView = getCellView(contactInfo);
            addItemView.setTag(contactInfo);
            addItemView.setOnClickListener(this);
            addItemView.setOnLongClickListener(this);
            return addItemView;
        }
        return null;
        /* @} */
    }

    /**
     * Get view by contact info.
     */
    private View getCellView(ContactItemInfo contactInfo) {
        final View addItemView = View.inflate(mContext,
                R.layout.widget_contact_add_item, null);
        ContactAddItem addItem = new ContactAddItem();
        addItem.mPhotoView = (ImageView) addItemView
                .findViewById(R.id.contacts_cell_photo);
        addItem.mNameView = (TextView) addItemView
                .findViewById(R.id.contacts_cell_name);
        addItem.mMessageIcon = (ImageView) addItemView
                .findViewById(R.id.contacts_cell_msg_icon);
        addItem.mMessageCount = (TextView) addItemView
                .findViewById(R.id.contacts_cell_msg_count);
        addItem.mPhoneIcon = (ImageView) addItemView
                .findViewById(R.id.contacts_cell_phone_icon);
        addItem.mPhoneCount = (TextView) addItemView
                .findViewById(R.id.contacts_cell_phone_count);
        addItem.badge = (QuickContactBadge) addItemView
                .findViewById(R.id.contacts_cell_badge);
        Uri lookupUri = Contacts.getLookupUri(contactInfo.mContactId, contactInfo.mLookUpKey);
        if(contactInfo.mFastNumber==1){
            if(contactInfo.mIsMissed){
                addItem.mNameView.setVisibility(View.GONE);
                addItem.mPhotoView.setScaleType(ScaleType.CENTER);
                addItem.mPhotoView.setImageResource(R.drawable.widgets_desktopcontact_voice_mail);
            }else{
                // SPRD : fix bug205152 voice mail string error
                addItem.mNameView.setText(R.string.fast_dail_voice_mail);
                addItem.mPhotoView.setScaleType(ScaleType.FIT_XY);
                addItem.mPhotoView.setImageResource(R.drawable.widgets_desktopcontact_voice_mail);
                addItem.badge.assignContactUri(lookupUri);
                addItem.badge.setMode(QuickContact.MODE_SMALL);
            }
            addItem.badge.setVisibility(View.GONE);
            return addItemView;
        }
        if (contactInfo.mIsMissed) {
            addItem.mNameView.setVisibility(View.GONE);
            addItem.mPhotoView.setScaleType(ScaleType.CENTER);
            addItem.mPhotoView.setImageResource(R.drawable.widgets_desktopcontact_button_add);
            addItem.badge.setVisibility(View.GONE);
        } else {
            addItem.mNameView.setText(contactInfo.mNameStr);
            addItem.mPhotoView.setScaleType(ScaleType.FIT_XY);
            mPhotoLoader.loadPhoto(addItem.mPhotoView, contactInfo.mPhotoId);
            if (contactInfo.mContactId != -1) {
                addItem.badge.assignContactUri(lookupUri);
                addItem.badge.setMode(QuickContact.MODE_SMALL);
            }else{
                addItem.badge.setVisibility(View.GONE);
            }
        }
        return addItemView;
    }

    /*
     * handle the missed msg and phone in one mothod so that it can refresh
     * easily
     */
    public void configureMissedPhoneAndMsg() {
        View addItem = null;
        ContactItemInfo info = null;
        Map<Integer,Integer> missCall=ContactsData.getInstance(mContext).getMissedCallCount();
        Map<Integer,Integer> missSms=ContactsData.getInstance(mContext).getMissedSmsCount();
        for (int j = 0; j < this.getChildCount(); j++) {
            ContactCellLayout cell = (ContactCellLayout) getChildAt(j);
            for (int i = 1; i < cell.getChildCount(); i++) {
                addItem = cell.getChildAt(i);
                info = (ContactItemInfo) addItem.getTag();
                // SPRD: fix bug213909 missSms missCall may be null
                if (missSms !=null && missCall!=null && !info.mIsMissed && info.mContactId != -1) {
                    if (missSms.get(i) == null) {
                        // if no mapping for the specified key is found, we will get a null,
                        // and then cast it to be int will caught a NullPointer,so we just make this item gone
                        addItem.findViewById(R.id.contacts_cell_msg_count)
                        .setVisibility(View.GONE);
                        addItem.findViewById(R.id.contacts_cell_msg_icon)
                        .setVisibility(View.GONE);
                    } else {
                        int mSmsCount = missSms.get(i);
                        if (mSmsCount != 0) {
                            addItem.findViewById(R.id.contacts_cell_msg_count)
                            .setVisibility(View.VISIBLE);
                            addItem.findViewById(R.id.contacts_cell_msg_icon)
                            .setVisibility(View.VISIBLE);
                            ((TextView) (addItem
                                    .findViewById(R.id.contacts_cell_msg_count)))
                                    .setText(mSmsCount + "");
                        } else {
                            addItem.findViewById(R.id.contacts_cell_msg_count)
                            .setVisibility(View.GONE);
                            addItem.findViewById(R.id.contacts_cell_msg_icon)
                            .setVisibility(View.GONE);
                        }
                    }
                    if (missCall.get(i) == null) {
                        addItem.findViewById(R.id.contacts_cell_phone_icon)
                        .setVisibility(View.GONE);
                        addItem.findViewById(R.id.contacts_cell_phone_count)
                        .setVisibility(View.GONE);
                    } else {
                        int mCallCount = missCall.get(i);
                        if (mCallCount != 0) {
                            addItem.findViewById(R.id.contacts_cell_phone_icon)
                            .setVisibility(View.VISIBLE);
                            addItem.findViewById(R.id.contacts_cell_phone_count)
                            .setVisibility(View.VISIBLE);
                            ((TextView) (addItem
                                    .findViewById(R.id.contacts_cell_phone_count)))
                                    .setText(mCallCount + "");
                        } else {
                            addItem.findViewById(R.id.contacts_cell_phone_icon)
                            .setVisibility(View.GONE);
                            addItem.findViewById(R.id.contacts_cell_phone_count)
                            .setVisibility(View.GONE);
                        }
                    }
                }
            }
        }
    }

    private class ContactAddItem {
        ImageView mPhotoView;
        TextView mNameView;
        ImageView mPhoneIcon;
        TextView mPhoneCount;
        ImageView mMessageIcon;
        TextView mMessageCount;
        QuickContactBadge badge;
    }

    private void addInScreen(View child, int index) {
        final ContactCellLayout group = (ContactCellLayout) getChildAt(0);
        group.addView(child);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        final int childCount = getChildCount();

        if (SCROLL_DIRECTION.equals("H")) {
            int childLeft = 0;
            for (int i = 0; i < childCount; i++) {
                final View childView = getChildAt(i);

                if (childView.getVisibility() != View.GONE) {
                    final int childWidth = childView.getMeasuredWidth();
                    childView.layout(childLeft, 0, childLeft + childWidth,
                            childView.getMeasuredHeight());
                    childLeft += childWidth;
                }
            }
        } else if (SCROLL_DIRECTION.equals("V")) {
            int childTop = 0;
            for (int i = 0; i < childCount; i++) {
                final View childView = getChildAt(i);

                if (childView.getVisibility() != View.GONE) {
                    final int childHeight = childView.getMeasuredHeight();
                    childView.layout(0, childTop, childView.getMeasuredWidth(),
                            childTop + childHeight);
                    childTop += childHeight;
                }
            }
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        final int width = MeasureSpec.getSize(widthMeasureSpec);
        final int height = MeasureSpec.getSize(heightMeasureSpec);

        final int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        if (widthMode != MeasureSpec.EXACTLY) {
            throw new IllegalStateException(
                    "ScrollLayout only canmCurScreen run at EXACTLY mode!");
        }

        final int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        if (heightMode != MeasureSpec.EXACTLY) {
            throw new IllegalStateException(
                    "ScrollLayout only can run at EXACTLY mode!");
        }

        // The children are given the same width and height as the scrollLayout
        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            getChildAt(i).measure(widthMeasureSpec, heightMeasureSpec);
        }
        if (SCROLL_DIRECTION.equals("H")) {
            scrollTo(0 * width, 0);
        } else if (SCROLL_DIRECTION.equals("V")) {
            scrollTo(0, 0 * height);
        }

    }

    public void snapToScreen(int whichScreen, int velocity) {
        snapToScreen(whichScreen, velocity, false);
    }

    private void snapToScreen(int whichScreen, int velocity, boolean settle) {
        if (!mScroller.isFinished()) {
            mScroller.abortAnimation();
        }
        // get the valid layout page
        whichScreen = Math.max(0, Math.min(whichScreen, getChildCount() - 1));
        final int screenDelta = Math.max(1, Math.abs(whichScreen - 0));
        int duration = (screenDelta + 1) * 100;
        velocity = Math.abs(velocity);

        if (velocity > 0) {
            duration += (duration / (velocity / BASELINE_FLING_VELOCITY))
                    * FLING_VELOCITY_INFLUENCE;
        } else {
            duration += 100;
        }

        awakenScrollBars(duration);

        if (settle) {
            mScrollInterpolator.setDistance(whichScreen);
        } else {
            mScrollInterpolator.disableSettle();
        }

        if (SCROLL_DIRECTION.equals("H")
                && getScrollX() != (whichScreen * getWidth())) {

            final int deltaX = whichScreen * getWidth() - getScrollX();
            mScroller.startScroll(getScrollX(), 0, deltaX, 0, duration);

        } else if (SCROLL_DIRECTION.equals("V")
                && getScrollY() != (whichScreen * getHeight())) {

            final int deltaY = whichScreen * getHeight() - getScrollY();
            mScroller.startScroll(0, getScrollY(), 0, deltaY,
                    Math.abs(deltaY) * 2);

        }
        // Redraw the layout
        invalidate();
    }

    @Override
    public void computeScroll() {
        if (mScroller.computeScrollOffset()) {
            scrollTo(mScroller.getCurrX(), mScroller.getCurrY());
            postInvalidate();
        }
    }

    class WorkspaceOvershootInterpolator implements Interpolator {
        private static final float DEFAULT_TENSION = 1.3f;
        private float mTension;

        public WorkspaceOvershootInterpolator() {
            mTension = DEFAULT_TENSION;
        }

        public void setDistance(int distance) {
            mTension = distance > 0 ? DEFAULT_TENSION / distance
                    : DEFAULT_TENSION;
        }

        public void disableSettle() {
            mTension = 0.f;
        }

        public float getInterpolation(float t) {
            t -= 1.0f;
            return t * t * ((mTension + 1) * t + mTension) + 1.0f;
        }
    }

    public void onClick(View v) {
        try {
            if (v.getTag() instanceof ContactItemInfo) {
                ContactItemInfo itemInfo = (ContactItemInfo) (v.getTag());
                if(itemInfo.mFastNumber==1){
                    /* SPRD: Fix bug 265596 @{ */
                    Sim[] sims = null;

                    /* SPRD: bug316052 2014-06-05 OOM problem optimize. @{ */
                    SimManager mSimManager = SimManager.get(mContext.getApplicationContext());
                    /* SPRD: bug316052 2014-06-05 OOM problem optimize. @} */

                    MobileSimChooserDialog mAlertDialog = new MobileSimChooserDialog(mContext);
                    sims = mSimManager.getActiveSims();
                    if (sims == null) {
                        return;
                    }
                    int length = sims.length;
                    if (0 == length) {
                        Toast toast = Toast
                                .makeText(mContext, R.string.no_sim_text, Toast.LENGTH_SHORT);
                        toast.show();
                    } else if (1 == length) {
                        int phoneId = sims[0].getPhoneId();
                        String voiceMailNumber = ContactsData.getInstance(mContext).getVoiceMailNumber(phoneId);
                        if (!TextUtils.isEmpty(voiceMailNumber)) {
                            Intent tel=new Intent(Intent.ACTION_CALL,Uri.parse("tel:"+voiceMailNumber));
                            mContext.startActivity(tel);
                        } else {
                            final Intent intent = new Intent(Intent.ACTION_MAIN);
                            intent.setClassName(PHONE_PACKAGE, CALL_SETTINGS_CLASS_NAME);
                            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                            intent.putExtra(TelephonyIntents.EXTRA_PHONE_ID, phoneId);
                            mContext.startActivity(intent);
                        }
                    } else {
                        mAlertDialog.setListener(new MobileSimChooserDialog.OnSimPickedListener() {
                            public void onSimPicked(int phoneId) {
                                String voiceMailNumber = ContactsData.getInstance(mContext).getVoiceMailNumber(phoneId);
                                if (!TextUtils.isEmpty(voiceMailNumber)) {
                                    Intent tel=new Intent(Intent.ACTION_CALL,Uri.parse("tel:"+voiceMailNumber));
                                    mContext.startActivity(tel);
                                } else {
                                    final Intent intent = new Intent(Intent.ACTION_MAIN);
                                    intent.setClassName(PHONE_PACKAGE, CALL_SETTINGS_CLASS_NAME);
                                    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                                    intent.putExtra(TelephonyIntents.EXTRA_PHONE_ID, phoneId);
                                    mContext.startActivity(intent);
                                }
                            }
                        });
                        mAlertDialog.show();
                    }
                    /* @} */
                }else if (itemInfo.mIsMissed) {
                    ContactsData.getInstance(mContext).setCurrRequest(
                            ((ContactItemInfo) v.getTag()).mFastNumber);
                    // Start the Contacts list application
//                    mLauncher = ContactsData.getInstance(mContext).mLauncher;
                    ((Launcher)mContext).startActivityForResult(itemInfo.mIntent,
                            Launcher.REQUEST_FOR_CONTACT_WIDGET);
                } else {
                    // only number no contact
                    Log.i("jxt", "call......");
                    Intent tel = new Intent(Intent.ACTION_CALL, Uri.parse("tel:" + itemInfo.mCallNumber));
                    //                    tel.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK );
                    mContext.startActivity(tel);
                }
            }
        } catch (Exception e) {
        }
    }

    /**
     * Pass up the widget's press event if we do not need it.
     */
    public boolean onLongClick(View v) {
        return performLongClick();
    }
}