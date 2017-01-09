package com.android.contacts.widget;

import com.android.contacts.common.lettertiles.LetterTileDrawable;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;

import com.android.contacts.R;
/*
 * SPRD:AndroidN porting add sim icon feature.
 * @{
 */
import com.sprd.contacts.common.model.account.SimAccountType;
import com.sprd.contacts.common.model.account.USimAccountType;
import android.telephony.TelephonyManager;
import com.android.contacts.common.model.account.AccountWithDataSet;
/*
 * @}
 */
/**
 * An {@link ImageView} designed to display QuickContact's contact photo. When requested to draw
 * {@link LetterTileDrawable}'s, this class instead draws a different default avatar drawable.
 *
 * In addition to supporting {@link ImageView#setColorFilter} this also supports a {@link #setTint}
 * method.
 *
 * This entire class can be deleted once use of LetterTileDrawable is no longer used
 * inside QuickContactsActivity at all.
 */
public class QuickContactImageView extends ImageView {

    private Drawable mOriginalDrawable;
    private BitmapDrawable mBitmapDrawable;
    private int mTintColor;
    private boolean mIsBusiness;

    /*
     * SPRD:AndroidN porting add sim icon feature.
     * SPRD: add for bug621379, add for fdn feature bugfix
     * @{
     */
    private AccountWithDataSet mAccount;
    private boolean mIsSdnContact;
    private boolean mIsFdnContact;
    private int mFdnPhoneId;
    protected int[] mSimBitmapRes = {
            R.drawable.person_white_540dp_sim1, R.drawable.person_white_540dp_sim2,
            R.drawable.person_white_540dp_sim3, R.drawable.person_white_540dp_sim4,
            R.drawable.person_white_540dp_sim5
    };
    protected int[] mSimSdnBitmapRes = {
            R.drawable.person_white_540dp_sim1_sdn, R.drawable.person_white_540dp_sim2_sdn,
            R.drawable.person_white_540dp_sim3_sdn, R.drawable.person_white_540dp_sim4_sdn,
            R.drawable.person_white_540dp_sim5_sdn
    };
    protected int[] mSimFdnBitmapRes = {
            R.drawable.person_white_540dp_sim1_fdn, R.drawable.person_white_540dp_sim2_fdn
    };
    /*
     * @}
     */

    public QuickContactImageView(Context context) {
        this(context, null);
    }

    public QuickContactImageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public QuickContactImageView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setTint(int color) {
        if (mBitmapDrawable == null || mBitmapDrawable.getBitmap() == null
                || mBitmapDrawable.getBitmap().hasAlpha()) {
            setBackgroundColor(color);
        } else {
            setBackground(null);
        }
        mTintColor = color;
        postInvalidate();
    }

    public boolean isBasedOffLetterTile() {
        return mOriginalDrawable instanceof LetterTileDrawable;
    }

    public void setIsBusiness(boolean isBusiness) {
        mIsBusiness = isBusiness;
    }

    /*
     * SPRD:AndroidN porting add sim icon feature.
     * SPRD: add for bug621379, add for fdn feature bugfix
     * @{
     */
    public void setAccount(AccountWithDataSet account) {
        mAccount = account;
    }

    public void setIsSdnContact(boolean isSdnContact) {
        mIsSdnContact = isSdnContact;
    }

    public void setIsFdnContact(boolean isFdnContact) {
        mIsFdnContact = isFdnContact;
    }

    public void setFdnPhoneId(int fdnPhoneId) {
        mFdnPhoneId = fdnPhoneId;
    }
    /*
     * @}
     */

    @Override
    public void setImageDrawable(Drawable drawable) {
        // There is no way to avoid all this casting. Blending modes aren't equally
        // supported for all drawable types.
        BitmapDrawable bitmapDrawable = null;
        if (drawable == null || drawable instanceof BitmapDrawable) {
            bitmapDrawable = (BitmapDrawable) drawable;
        } else if (drawable instanceof LetterTileDrawable) {
            /*
             * SPRD:AndroidN porting add sim icon feature.
             * Original code:
            if (!mIsBusiness) {
             * @{
             */
            if (mAccount != null && (mAccount.type.equals(USimAccountType.ACCOUNT_TYPE) ||
                    mAccount.type.equals(SimAccountType.ACCOUNT_TYPE))) {
                String simAccountName = mAccount.name;
                int phoneId = 0;
                boolean isSingleSim = ((TelephonyManager) TelephonyManager.from(mContext)).getPhoneCount() == 1 ? true
                        : false;
                /* SPRD: Bug 599439 outofmemoryerror @{ */
                try {
                    if (isSingleSim) {
                        bitmapDrawable = (BitmapDrawable) getResources().getDrawable(R.drawable.person_white_540dp_sim);
                    } else {
                        phoneId = Integer.parseInt(simAccountName.substring(3))-1;
                        if (mIsSdnContact) {
                            bitmapDrawable = (BitmapDrawable) getResources().getDrawable(mSimSdnBitmapRes[phoneId]);
                        } else {
                            bitmapDrawable = (BitmapDrawable) getResources().getDrawable(mSimBitmapRes[phoneId]);
                        }
                    }
                } catch (OutOfMemoryError e) {
                    Log.e(this.getClass().getSimpleName(), e.getMessage());
                }
                /* @} */

            } else if (!mIsBusiness) {
            /*
             * @}
             */
                /*
                 * SPRD: 594221 outofmemoryerror
                 * SPRD: add for bug621379, add for fdn feature bugfix
                 * 
                 * @{
                 */
                try {
                    if (mIsFdnContact) {
                        bitmapDrawable = (BitmapDrawable) getResources().getDrawable(mSimFdnBitmapRes[mFdnPhoneId]);
                    } else {
                        bitmapDrawable = (BitmapDrawable) getResources()
                                .getDrawable(R.drawable.person_white_540dp);
                    }
                } catch (OutOfMemoryError e) {
                    Log.e(this.getClass().getSimpleName(), e.getMessage());
                }
                /*
                 * @}
                 */
            } else {
                bitmapDrawable = (BitmapDrawable) getResources().getDrawable(
                        R.drawable.generic_business_white_540dp);
            }
        } else {
            throw new IllegalArgumentException("Does not support this type of drawable");
        }

        mOriginalDrawable = drawable;
        mBitmapDrawable = bitmapDrawable;
        setTint(mTintColor);
        super.setImageDrawable(bitmapDrawable);
    }

    @Override
    public Drawable getDrawable() {
        return mOriginalDrawable;
    }
}
