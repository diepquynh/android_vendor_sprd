/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.contacts.interactions;

import com.android.contacts.R;
import com.android.contacts.common.util.BitmapUtil;
import com.android.contacts.common.util.ContactDisplayUtils;

import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.provider.CallLog.Calls;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.text.BidiFormatter;
import android.text.Spannable;
import android.text.TextDirectionHeuristics;
/**
 * SPRD:
 *
 * @{
 */
import com.android.contacts.common.CallUtil;
import android.os.SystemProperties;
import android.os.UserManager;
/**
 * @}
 */

/**
 * Represents a call log event interaction, wrapping the columns in
 * {@link android.provider.CallLog.Calls}.
 *
 * This class does not return log entries related to voicemail or SIP calls. Additionally,
 * this class ignores number presentation. Number presentation affects how to identify phone
 * numbers. Since, we already know the identity of the phone number owner we can ignore number
 * presentation.
 *
 * As a result of ignoring voicemail and number presentation, we don't need to worry about API
 * version.
 */
public class CallLogInteraction implements ContactInteraction {

    private static final String URI_TARGET_PREFIX = "tel:";
    private static final int CALL_LOG_ICON_RES = R.drawable.ic_phone_24dp;
    private static final int CALL_ARROW_ICON_RES = R.drawable.ic_call_arrow;
    /**
     * SPRD:Bug615804 the arrow shows not completely
     * @{
     */
    private static final int CALL_ARROW_ICON_OUTGOING = R.drawable.ic_call_arrow_outgoing;
    /**
     * @}
     */
    private static BidiFormatter sBidiFormatter = BidiFormatter.getInstance();
    /**
     * SPRD:Bug474802 Distinguish phone callLog and video callLog on recentCard；Bug454332
     *
     * @{
     */

    private static final int CALL_LOG_VIDEO_ICON_RES = R.drawable.ic_videocam;
    private static final String CALL_ORIGIN_QUICK_CONTACTS_ACTIVITY =
            "com.android.contacts.quickcontact.QuickContactActivity";
    private static final int IS_VIDEO_CALL = 1;
    private static final int IS_VOLTE_CALL = 2;
    private static final int IS_VOLTE_VIDEO_CALL = 3;
    /**
     * @}
     */
    private ContentValues mValues;

    public CallLogInteraction(ContentValues values) {
        mValues = values;
    }

    @Override
    public Intent getIntent() {
        String number = getNumber();
        /**
         * SPRD:Bug474802 Distinguish phone callLog and video callLog on recentCard；Bug454332
         *
         * @{
         */
        if (getFeature() == IS_VIDEO_CALL || getFeature() == IS_VOLTE_VIDEO_CALL) {
            return number == null ? null : CallUtil.getVideoCallIntent(number,
                    CALL_ORIGIN_QUICK_CONTACTS_ACTIVITY).setData(
                    Uri.parse(URI_TARGET_PREFIX + number));
        }
        /**
         * @}
         */
        return number == null ? null : new Intent(Intent.ACTION_CALL).setData(
                Uri.parse(URI_TARGET_PREFIX + number));
    }

    /**
     * SPRD Bug523445 after switching to guest, click icon of call log still display as a video
     * call start
     * SPRD Bug540972 after disable volte, click video call log in recent log of contacts detail
     * interface, would call out as video call
     * @{
     */
    @Override
    public Intent getIntent(Context context, boolean enableButton) {
        String number = getNumber();
        UserManager userManager = (UserManager)context.getSystemService(Context.USER_SERVICE);
        if (userManager.isSystemUser()) {
            if ((getFeature() == IS_VOLTE_VIDEO_CALL) && enableButton) {
                if (enableButton) {
                    return number == null ? null : CallUtil.getVideoCallIntent(number,
                            CALL_ORIGIN_QUICK_CONTACTS_ACTIVITY).setData(
                            Uri.parse(URI_TARGET_PREFIX + number));
                }
            } else if (getFeature() == IS_VIDEO_CALL) {
                return number == null ? null : CallUtil.getVideoCallIntent(number,
                        CALL_ORIGIN_QUICK_CONTACTS_ACTIVITY).setData(
                        Uri.parse(URI_TARGET_PREFIX + number));
            }
        }
        return number == null ? null : new Intent(Intent.ACTION_CALL).setData(
                Uri.parse(URI_TARGET_PREFIX + number));
    }

    @Override
    public Drawable getIcon(Context context, boolean enableButton) {
        /**
         * sprd Bug521642 while switch to guest, icons of call log still display as video_call
         * @{
         */
        UserManager userManager = (UserManager)context.getSystemService(Context.USER_SERVICE);
        if (!userManager.isSystemUser()) {
            return context.getResources().getDrawable(CALL_LOG_ICON_RES);
        }
        /**
         * @}
         */
        /**
         * sprd Bug532444 video call log show common call in contacts recently calls, but call
         * out as a video call log
         */
        if ((getFeature() == IS_VOLTE_VIDEO_CALL) && enableButton) {
            return context.getResources().getDrawable(CALL_LOG_VIDEO_ICON_RES);
        } else if (getFeature() == IS_VIDEO_CALL) {
            return context.getResources().getDrawable(CALL_LOG_VIDEO_ICON_RES);
        }
        return context.getResources().getDrawable(CALL_LOG_ICON_RES);
    }
    /**
     * @}
     */

    @Override
    public String getViewHeader(Context context) {
        return getNumber();
    }

    @Override
    public long getInteractionDate() {
        Long date = getDate();
        return date == null ? -1 : date;
    }

    @Override
    public String getViewBody(Context context) {
        Integer numberType = getCachedNumberType();
        if (numberType == null) {
            return null;
        }
        return Phone.getTypeLabel(context.getResources(), getCachedNumberType(),
                getCachedNumberLabel()).toString();
    }

    @Override
    public String getViewFooter(Context context) {
        Long date = getDate();
        return date == null ? null : ContactInteractionUtil.formatDateStringFromTimestamp(
                date, context);
    }

    /**
     * SPRD: Bug474802 Distinguish phone calllog and video calllog on recentcard；Bug454332
     *
     * Original Android code:
    @Override
    public Drawable getIcon(Context context) {
        return context.getResources().getDrawable(CALL_LOG_ICON_RES);
    }
     * @{
     */
    @Override
    public Drawable getIcon(Context context) {
        /**
         * sprd Bug521642 while switch to guest, icons of call log still display as video_call
         * @{
         */
        UserManager userManager = (UserManager)context.getSystemService(Context.USER_SERVICE);
        if (!userManager.isSystemUser()) {
            return context.getResources().getDrawable(CALL_LOG_ICON_RES);
        }
        /**
         * @}
         */
        /**
         * sprd Bug532444 video call log show common call in contacts recently calls, but call
         * out as a video call log
         */
        if (getFeature() == IS_VIDEO_CALL || getFeature() == IS_VOLTE_VIDEO_CALL) {
            return context.getResources().getDrawable(CALL_LOG_VIDEO_ICON_RES);
        }
        return context.getResources().getDrawable(CALL_LOG_ICON_RES);
    }
    public Integer getFeature() {
        /**
         * SPRD:Bug517209 When video call turn off, the video call log should not available.
         * Original Android code:
        return mValues.getAsInteger(Calls.FEATURES);
         * @{
         */
        boolean isVideoEnabled = SystemProperties.getBoolean("persist.sys.support.vt", false);
        Integer feature = mValues.getAsInteger(Calls.FEATURES);
        return (!isVideoEnabled &&
                (feature == IS_VIDEO_CALL || feature == IS_VOLTE_VIDEO_CALL)) ? 0 : feature;
        /**
         * @}
         */
    }

    /**
     * @}
     */
    @Override
    public Drawable getBodyIcon(Context context) {
        return null;
    }

    @Override
    public Drawable getFooterIcon(Context context) {
        Drawable callArrow = null;
        Resources res = context.getResources();
        Integer type = getType();
        if (type == null) {
            return null;
        }
        switch (type) {
            case Calls.INCOMING_TYPE:
                callArrow = res.getDrawable(CALL_ARROW_ICON_RES);
                callArrow.setColorFilter(res.getColor(R.color.call_arrow_green),
                        PorterDuff.Mode.MULTIPLY);
                break;
            case Calls.MISSED_TYPE:
                /**
                 * SPRD:Bug525410 Can't update calllog
                 * Original code:
                 * callArrow = res.getDrawable(CALL_ARROW_ICON_RES);
                 * @{
                 */
                callArrow = res.getDrawable(R.drawable.ic_call_arrow).mutate();
                /**
                 * @}
                 */
                callArrow.setColorFilter(res.getColor(R.color.call_arrow_red),
                        PorterDuff.Mode.MULTIPLY);
                break;
            case Calls.OUTGOING_TYPE:
                /**
                 * SPRD:Bug615804 the arrow shows not completely
                 * @{
                 */
                callArrow = res.getDrawable(CALL_ARROW_ICON_OUTGOING);
                /**
                 * @}
                 */
                callArrow.setColorFilter(res.getColor(R.color.call_arrow_green),
                        PorterDuff.Mode.MULTIPLY);
                break;
        }
        return callArrow;
    }

    public String getCachedName() {
        return mValues.getAsString(Calls.CACHED_NAME);
    }
    public String getCachedNumberLabel() {
        return mValues.getAsString(Calls.CACHED_NUMBER_LABEL);
    }

    public Integer getCachedNumberType() {
        return mValues.getAsInteger(Calls.CACHED_NUMBER_TYPE);
    }

    public Long getDate() {
        return mValues.getAsLong(Calls.DATE);
    }

    public Long getDuration() {
        return mValues.getAsLong(Calls.DURATION);
    }

    public Boolean getIsRead() {
        return mValues.getAsBoolean(Calls.IS_READ);
    }

    public Integer getLimitParamKey() {
        return mValues.getAsInteger(Calls.LIMIT_PARAM_KEY);
    }

    public Boolean getNew() {
        return mValues.getAsBoolean(Calls.NEW);
    }

    public String getNumber() {
        final String number = mValues.getAsString(Calls.NUMBER);
        return number == null ? null :
            sBidiFormatter.unicodeWrap(number, TextDirectionHeuristics.LTR);
    }

    public Integer getNumberPresentation() {
        return mValues.getAsInteger(Calls.NUMBER_PRESENTATION);
    }

    public Integer getOffsetParamKey() {
        return mValues.getAsInteger(Calls.OFFSET_PARAM_KEY);
    }

    public Integer getType() {
        return mValues.getAsInteger(Calls.TYPE);
    }

    @Override
    public Spannable getContentDescription(Context context) {
        final String phoneNumber = getViewHeader(context);
        final String contentDescription = context.getResources().getString(
                R.string.content_description_recent_call,
                getCallTypeString(context), phoneNumber, getViewFooter(context));
        return ContactDisplayUtils.getTelephoneTtsSpannable(contentDescription, phoneNumber);
    }

    private String getCallTypeString(Context context) {
        String callType = "";
        Resources res = context.getResources();
        Integer type = getType();
        if (type == null) {
            return callType;
        }
        switch (type) {
            case Calls.INCOMING_TYPE:
                callType = res.getString(R.string.content_description_recent_call_type_incoming);
                break;
            case Calls.MISSED_TYPE:
                callType = res.getString(R.string.content_description_recent_call_type_missed);
                break;
            case Calls.OUTGOING_TYPE:
                callType = res.getString(R.string.content_description_recent_call_type_outgoing);
                break;
        }
        return callType;
    }

    @Override
    public int getIconResourceId() {
        return CALL_LOG_ICON_RES;
    }
}
