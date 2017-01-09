/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.util;

import java.lang.reflect.Method;
import java.util.Arrays;

import android.os.Build;
import android.util.Log;
import android.os.SystemProperties;

// fix bug 5420 at 2011-10-31
// get special values for the special phone platform
public class Compatible {

    // CID 109299 : SS: Unread field should be static (FB.SS_SHOULD_BE_STATIC)
    // private final String TAG = "Compatible";
    private static final String TAG = "Compatible";
    public boolean mIsLephone = false; // for lephone
    public boolean mIsHTCSensation = false; //for htc sensation
    public boolean mIsHTCMYtouchMIUI =false;
    public boolean mIsHTCG11 = false; // for htc g11
    public boolean mIsHTCHD2 = false; // for htc hd2
    public boolean mIsHTCDesireHD = false; // for htc desire hd
    public boolean mIsHTCG12NotMIUI = false;
    public boolean mIsHTCOneX = false; // for htc one x
    public boolean mIsHTCDesire = false; // for htc G7
    public boolean mIsSamsung9xxx = false; // for Samsung 9100 9220.
    public boolean mIsSamsung9100 = false;
    public boolean mIsSEX10i = false; // for sony erisson Xperia 10
    public boolean mIsVerizonDROIDX = false; // for verizon DROIDX (moto?)
    //for HUAWEI U8800
    public boolean mIsU8800 = false;
    //for HUAWEI initParam_U8860
    public boolean mIsU8860 = false;
    // for HUAWEI C8812
    public boolean mIsC8812 = false;
    // HTC Sensation XL with Beats Audio X315e
    public boolean mIsHTCX315E = false;
    public boolean mIsHTCX315E_ICS = false;
    // XT910
    public boolean mIsXT910 = false;
    // for ME525
    public boolean mIsMe525 = false;
    public boolean mIsMb525 = false;
    public boolean mIsMB526 = false;
    // for Moto XT928
    public boolean mIsXT928 = false;
    public boolean mIsHTCManufacturer = false; // for htc devices
    public boolean mIsMotoManufacturer = false; // for Motorola devices
    public boolean mIsSamsungManufacturer = false; // for Samsung devices
    public boolean mIsMeizuM9 = false; // for Meizu M9
    public boolean mIsMeizuMX = false; // for Meizu MX
    public boolean mIsMeiZuM03X = false;
    public boolean mIsMeizuManufacturer = false; // for Meizu devices, include M9 and MX
    // for samsung galaxy nexus
    public boolean mIsGalaxyNexus = false;
    /* for Google Nexus S (Samsung), the preview-size switch
     * between 640*480 and 720*480 will crash occasionly.
     */
    public boolean mIsGoogleNexusS = false;
    // for samsung i9000
    public boolean mIsSamsung9000 = false;
    public boolean mIsSpreadtrum = false;
    // for samsung galaxy tab P1000
    public boolean mIsGTP1000 = false;

    // camcorderprofile of GTI9008L device is difference with others, it use the videoformat with 3
    public boolean mIsGTI9008L = false;

    // for sonyericsson LT18i ICS version
    public boolean mIsSELT18i_ICS = false;
    // for xiaomi ICS version
    public boolean mIsMIOne_ICS = false;
    // for LG P970
    public boolean mIsLGP970 = false;
    public boolean mIsCoolpad7260 = false; //for Coolpad7260

    // for ALCATEL_one_touch_995
    public boolean mIsALCATEL_one_touch_995 = false;
    // for iriver
    public boolean mIsIriverMX100 = false;
    //for LT28i
    public boolean mIsLT28i = false;
    public boolean mIsLT28i_ICS = false;
    //for LT26i
    public boolean mIsLT26i = false;
    public boolean mIsLT26i_ICS = false;
    //for sky IM-A760S
    public boolean mIsIMA760S = false;
    //for OPPO X905
    public boolean mIsOPPOX905 = false;
    //for sonyericsson IS11S version
    public boolean mIsIS11S = false;
    public boolean mIsIS12S = false;
    // for sonyericsson IS11S ICS version
    public boolean mIsIS11S_ICS = false;
    // for DOOV D3
    public boolean mIsDOOV_D3 = false;
    // for DOOV D9
    public boolean mIsDOOV_D9 = false;
    // for DOOV D50
    public boolean mIsDOOV_D50 = false;
    // for ZOPO ZP200
    public boolean mIsZP200_ICS = false;
    // for OPPO X907
    public boolean mIsOPPOX907_ICS = false;
    //for Qphone Msm7627a
    public boolean mIsMsm7627a = false;
    // for Qphone Msn8x25
    public boolean mIsMsm8x25 = false;
    //for Konka W960
    public boolean mIsW960 = false;
    // for REGZA IS04
    public boolean mIsIS04 = false;
    // for MEDIAS N-06C
    public boolean mIsN06C = false;
    // for HTC ISW13HT
    public boolean mIsISW13HT = false;
    // HTC G17
    public boolean mIsHTCG17 = false;
    //Lenovo A750
    public boolean mIsLenovoA750 = false;
    // enum of device platform, now only consider the Qcomm series
    public enum DevicePlatform {
        Unknown, QCT7201, QCT7225, QCT7x27, QCT7x30, QCT8x50, QCT8x60;
    }
    // the next variables are introduced for HTC devices
    // for support audio recorder 128K bitrate
    private boolean mSupport128kBitrate = false;
    // for support audio stereo record
    private boolean mSupportStereoRecord = false;
    // for device platform
    private DevicePlatform mDevicePlatform = DevicePlatform.Unknown;

    // the variant model names for Samsung Galaxy S
    private final String[] mSamSungGalaxySVariants = {"GT-I9000","SGH-T959","SGH-T959V","SGH-I897","SPH-D700","SCH-I500",
            "SCH-I400","SGH-I896","GT-I9000M","SGH-T959P","SGH-T959D","GT-I9000T","GT-I9000B","SHW-M110S",
            "SHW-M130L","SHW-M130K","SCH-I909","GT-I9008L","GT-I9088","SC-02B","GT-9003","GT-9000","GT-I9001"};
    // the variant model names for Samsung Galaxy S II
    private final String[] mSamSungGalaxySIIVariants = {"GT-I9100","GT-I9100G","GT-I9100T","SPH-D710","SGH-T989",
            "SGH-I777","SGH-I727","SGH-I927","SC-02C","SHW-M250S","SHW-M250K","SHW-M250L","SGH-i727R","GT-I9100M",
            "SGH-T989D"};
    // the variant model names for Samsung Galaxy R / Galaxy W
    private final String[] mSamSungGalaxyRWVariants = {"GT-I9103","SGH-T679","GT-I8150"};
    // the variant model names for Samsung Nexus S
    private final String[] mSamSungNexusSVariants = {"Google Nexus S","Nexus S","GT-I9023","GT-I9020","GT-I9020T","GT-I9020A","SPH-D720"};
    // Use a singleton.
    private static Compatible sCompatible;
    public static synchronized Compatible instance() {
        if (sCompatible == null) {
            sCompatible = new Compatible();
        }
        return sCompatible;
    }

    // the serial Model
    String mSerialModel;
    /**
     * get the main model in a serials of variant models by search the variants tables
     * @param variantModel: the device model
     * @return the main model for the matched serials; or the device model while not match
     */
    private String getSerialModelforVariant(String variantModel){
        for (String s:mSamSungGalaxySVariants){
            if (variantModel.equalsIgnoreCase(s)){
                return mSamSungGalaxySVariants[0];
            }
        }
        for (String s:mSamSungGalaxySIIVariants){
            if (variantModel.equalsIgnoreCase(s)){
                return mSamSungGalaxySIIVariants[0];
            }
        }
        for (String s:mSamSungGalaxyRWVariants){
            if (variantModel.equalsIgnoreCase(s)){
                return mSamSungGalaxyRWVariants[0];
            }
        }
        for (String s:mSamSungNexusSVariants){
            if (variantModel.equalsIgnoreCase(s)){
                return mSamSungNexusSVariants[0];
            }
        }
        return variantModel;
    }
    // initialize the BoardParams according the Build.BRAND and Build.MODEL
    private Compatible() {
        // CID 109032 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // String propValue = SystemProperties.get("ro.camera.sound.forced");

        mIsHTCManufacturer = Build.MANUFACTURER.equalsIgnoreCase("HTC");
        mIsMotoManufacturer = Build.MANUFACTURER.equalsIgnoreCase("motorola");
        mIsSamsungManufacturer = Build.MANUFACTURER.equalsIgnoreCase("samsung");

        final Class clazz = this.getClass();

        mSerialModel = getSerialModelforVariant(Build.MODEL);
        //String brand = Build.BRAND.replace('-', '_').replace(' ', '_');
        String model = mSerialModel.replace('-', '_').replace(' ', '_'); // for Google Nexus S

        String methodName = "initParam_" + model;
        Log.d(TAG, "methodName = "+methodName);
        try {
            final Method method = clazz.getDeclaredMethod(methodName);
            // call the initParam_brandxxx_modelxxx() to initial the mBoardParams
            method.invoke(this);
        } catch (Exception e) {
            Log.w(TAG, "brand is" + Build.BRAND + ", model is " + Build.MODEL);
            Log.w(TAG, "No method defined: " + methodName);
        }
    }

    /**
     * check whether the device should use
     * cam_mode=1;focus-mode=continuous-video;iso=movie when recording
     * @return true : should set the special settings when recording
     *         false: not set it
     */
    public static boolean isSamsungVideoSpecial(){
        String brand = Build.BRAND.replace('-', '_').replace(' ', '_');
        String model = instance().mSerialModel.replace('-', '_').replace(' ', '_');
        if (brand.equalsIgnoreCase("samsung") && model.equals("GT_I9100")){
            // i9100
            return true;
        }
        else if (brand.equalsIgnoreCase("samsung") && model.equals("GT_I9103")){
            // i9103
            return true;
        }
        else if (brand.equalsIgnoreCase("Android") && model.equals("Google_Nexus_S")){
            // i9023
            return true;
        }
        else if (brand.equalsIgnoreCase("samsung") && model.equals("GT_I9000")){
            // i9000
            /*
             * FIX BUG: 5731 119
             * BUG CAUSE: the setting of focus-mode=continuous-video cause a crash on i9000
             * FIX COMMENT:do not set the settings when i9000
             * DATE: 2011-11-30
             */
            return false;
        }
        return false;
    }


    public static boolean isHoneyCombo() {
        return Build.VERSION.SDK_INT > Build.VERSION_CODES.GINGERBREAD_MR1
        && Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH;
    }
    public static boolean isICS() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH;
    }
    public static boolean isBeforeICS() {
        return !isICS();
    }

    void initParam_GT_P1000() {
        mIsGTP1000 = true;
    }

    void initParam_ALCATEL_one_touch_995() {
        mIsALCATEL_one_touch_995 = true;
    }

    /**
     * Spreadtrum phone
     */
    void initParam_SP6810A() {
        mIsSpreadtrum = true;
    }

    /**
     * Spreadtrum phone
     */
    /*
     * FIX BUG: 1065
     * BUG CAUSE: SPHS_on_Hsdroid could not support camera sound mute
     * FIX COMMENT: set m_ro_camera_sound_forced is ture
     * DATE: 2012-06-14
     */
    void initParam_SPHS_on_Hsdroid() {
        mIsSpreadtrum = true;
    }
    /**
     * FIX BUG: 948
     * BUG CAUSE:  some color effects of getSupportedColorEffects
     * is not supported on Coolpad7260 platform
     * FIX COMMENT: add init function for coolpad7260
     * Date: 2012-04-28
     */
    void initParam_7260(){
        mIsCoolpad7260 = true;
    }
    /**
     * moto defy me525
     */
    void initParam_ME525() {
        /*
         * FIX BUG: 142
         * BUG CAUSE: me525 will change picture size when changing zoom values in multimode
         * FIX COMMENT:do not change zoom values in multi mode
         * DATE: 2011-11-30
         */
        mIsMe525 = true;
        initParam_MB525();
    }

    /*
     * FIX BUG: 1403
     * BUG CAUSE: MB525 MIUI ICS could not recording
     * FIX COMMENT:add correct video size and other parameters
     * DATE: 2012-08-01
     */
    void initParam_MB525(){
        // moto defy
        mIsMb525 = true;
    }

    /*
     * FIX BUG: 1142
     * BUG CAUSE: MB526 could not support "1280x720" recording
     * FIX COMMENT:initialize video size of supported for MB526
     * DATE: 2012-07-04
     */
    void initParam_MB526(){
        mIsMB526 = true;
    }

    void initParam_ME811(){
        // moto DroidX
    }

    void initParam_XT928(){
        mIsXT928 = true;
    }
    void initParam_Nexus_S(){
        initParam_Google_Nexus_S();
    }
    void initParam_Google_Nexus_S(){
        // samsung I9023 Nexus S
        mIsGoogleNexusS = true;
    }

    void initParam_SGH_I777(){
        initParam_GT_I9100();
    }

    void initParam_GT_I9100(){
        // samsung I9100 Galaxy S II
        mIsSamsung9xxx = true;
        mIsSamsung9100 = true;

    }
    void initParam_GT_N7000(){
        initParam_GT_I9220();
    }
    /*
     * FIX BUG: 1068
     * BUG CAUSE: don't add the video size for GT-I9220(USA)
     * FIX COMMENT:add the video size
     * DATE: 2012-06-01
     */
    void initParam_SGH_I717(){
        initParam_GT_I9220();
    }
    void initParam_GT_I9220(){
        // samsung I9220 Galaxy Note
        mIsSamsung9xxx = true;
    }

    void initParam_GT_I9103(){
        // samsung I9103 Galaxy W/R
        mIsSamsung9xxx = true;
    }

    void initParam_GT_I9000(){
        boolean isSDK10Later = true;
        mIsSamsung9000 = true;
        if (Build.MODEL.equalsIgnoreCase("GT-I9008L")){
            mIsGTI9008L = true;
        }
        // samsung I9000 Galaxy S
        /*
         * FIX BUG: 371
         * BUG CAUSE: the 2.2 version i9000 original system could not support camera sound mute
         * FIX COMMENT:check the sdk version to set m_ro_camera_sound_forced
         * DATE: 2012-02-03
         */
        // 2.3.3 and later version can mute the capture sound
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.GINGERBREAD_MR1){
            isSDK10Later = false;
        }
    }

    void initParam_GT_I5801(){
        // samsung I5801 / I5800?
    }

    void initParam_GT_I5508(){
        // samsung i5508
    }

    void initParam_HTC_Desire_S() {
        initParam_Desire_S();
    }
    void initParam_Desire_S(){
        if (!Build.ID.contains("MIUI")){
            mIsHTCG12NotMIUI = true;
        }
        mSupport128kBitrate = true;
        mDevicePlatform = DevicePlatform.QCT7x30;
    }

    void initParam_Galaxy_Nexus() {
        mIsGalaxyNexus = true;
    }

    void initParam_HTC_Desire_HD_A9191(){
        initParam_Desire_HD();
    }

    /*
     * Bug Fix: 1521
     * BUG CAUSE:model:HTC Desire HD can't select video size
     * FIX COMMENT: add video size for HTC Desire HD
     * Date: 2012-08-29
     */
    void initParam_HTC_Desire_HD(){
        initParam_Desire_HD();
    }
    void initParam_Desire_HD(){
        // HTC Desire HD G10
        mIsHTCDesireHD = true;

        mSupport128kBitrate = true;
        mDevicePlatform = DevicePlatform.QCT7x30;
    }

    void initParam_HTC_HD2(){
        // HTC HD2
        mIsHTCHD2 = true;
    }
    void initParam_HTC_Glacier(){
        // HTC Mytouch Glacier
        if (Build.ID.contains("MIUI")){
            mIsHTCMYtouchMIUI =true;
        }

        mSupport128kBitrate = true;
        mDevicePlatform = DevicePlatform.QCT7x30;
    }

    void initParam_HTC_Wildfire(){
        // htc g8
        mSupport128kBitrate = false;
        mDevicePlatform = DevicePlatform.QCT7225;
    }

    void initParam_MotoA953(){
        // Moto Milestone II
    }

    void initParam_HTC_Sensation_Z710e(){
        // HTC_Sensation G14
        mIsHTCSensation = true;

        mSupport128kBitrate = true;
        mSupportStereoRecord = true;
        mDevicePlatform = DevicePlatform.QCT8x60;
    }

    void initParam_HTC_S710d(){
        // HTC G11
        mIsHTCG11 = true;
    }

    void initParam_XT910() {
        mIsXT910 = true;
    }

    void initParam_HTC_Sensation_XL_with_Beats_Audio_X315e(){
        mIsHTCX315E = true;
        /*
         * Bug Fix: 1475
         * BUG CAUSE: the G21_ICS did not play the videorecord click sound
         * FIX COMMENT: force to play the sound on app level
         * Date: 2012-07-17
         */
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsHTCX315E_ICS = true;
        }

        mSupport128kBitrate = true;
        mSupportStereoRecord = true;
        mDevicePlatform = DevicePlatform.QCT7x30;
    }

    void initParam_HTC_One_X(){
        mIsHTCOneX = true;
    }

    void initParam_X10i(){
        // SonyEricssonn X10
        mIsSEX10i = true;
    }

    void initParam_DROIDX(){
        //Brand : verizon
        //Device : cdma_shadow
        //Model :DROIDX
        mIsVerizonDROIDX = true;
    }

    /*
     * FIX BUG: 1176
     * BUG CAUSE:some video size of meizu mx is not supported
     * FIX COMMENT:initialize video size of supported for meizu mx
     * DATE: 2012-07-03
     */
    void initParam_MEIZU_MX(){
        mIsMeizuManufacturer = true;
        //Model :MEIZU MX
        mIsMeizuMX = true;
    }

    void initParam_MX() {
        initParam_MEIZU_MX();
    }

    void initParam_M030(){
        mIsMeizuManufacturer = true;

        //Model :MEIZU M030
        mIsMeiZuM03X = true;
    }

    void initParam_M031(){
        initParam_M030();
    }

    void initParam_M032(){
        //MeiZu MX Quad-Core
        initParam_M030();
    }

    void initParam_M9(){
        mIsMeizuManufacturer = true;
        //Model :M9
        mIsMeizuM9 = true;
        /*
         * FIX BUG: 6527
         * BUG CAUSE: the M9 does not support 176x144 perfectly
         * FIX COMMENT:remove 176x144 video size
         * DATE: 2011-12-20
         */
    }

    void initParam_MI_ONE_Plus(){
        // xiao mi 4.0
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsMIOne_ICS = true;
        }
    }
    void initParam_LT18i(){
        // sony ericsson LT18i
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsSELT18i_ICS = true;
        }
    }

    void initParam_LG_P970(){
        mIsLGP970 = true;
    }
    void initParam_u8800(){
        // HUAWEI U8800
        mIsU8800 = true;
    }

    /*
     * FIX BUG: 1406 1409
     * BUG CAUSE: the U8860 did not play the videorecord click sound
     * FIX COMMENT: force to play the sound on app level,add video size
     * DATE: 2012-07-31 2012-08-01
     */
    void initParam_U8860(){
        mIsU8860 = true;
    }

    void initParam_HUAWEI_C8812(){
        mIsC8812 = true;
    }

    void initParam_ZTE_T_U880(){
    }

    /*
     * FIX BUG: 962
     * BUG CAUSE:the video size of v880 is not initialized
     * FIX COMMENT:initialize video size for v880
     * DATE: 2012-05-02
     */
    void initParam_ZTE_U_V880(){
    }

    void initParam_ILT_MX100(){
        mIsIriverMX100 = true;
    }

    void initParam_E800(){
    }

    void initParam_LT28i(){
        mIsLT28i = true;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsLT28i_ICS = true;
        }
        initParam_LTxxi();
    }

    void initParam_LT26i(){
        mIsLT26i = true;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsLT26i_ICS = true;
        }
        initParam_LTxxi();
    }

    void initParam_SO_02D(){
        initParam_LT26i();
    }

    void initParam_LTxxi(){

    }

    /*
     * FIX BUG: 1366
     * BUG CAUSE: the video size of ISW13HT is not initialized
     * FIX COMMENT: initialize video size for ISW13HT
     * Date: 2012-08-02
     */
    void initParam_ISW13HT(){
        mIsISW13HT = true;
    }

    /**
     * FIX BUG: 1041
     * BUG CAUSE: the video size of IM-A760S is not initialized
     * FIX COMMENT: initialize video size for IM-A760S
     * Date: 2012-05-24
     */
    void initParam_IM_A760S(){
        //sky IM-A760S
        mIsIMA760S = true;
    }

    /*
     * FIX BUG: 1038
     * BUG CAUSE: don't add the video size for opp0 X905
     * FIX COMMENT:add the video size
     * DATE: 2012-06-01
     */
    void initParam_X905(){
        //OPPO X905
        mIsOPPOX905 = true;
    }

    /*
     * FIX BUG: 1185 1189
     * BUG CAUSE: don't add the video size for Sony Ericsson IS11S
     * FIX COMMENT:add the video size,set m_ro_camera_sound_forced is false
     * DATE: 2012-07-03 2012-07-04
     */
    void initParam_IS11S(){
        mIsIS11S = true;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsIS11S_ICS = true;
        }
    }

    void initParam_SO_03D(){
        initParam_IS12S();
    }
    /*
     * FIX BUG: 1181
     * BUG CAUSE: don't add the video size for Sony Ericsson IIS12S
     * FIX COMMENT:add the video size
     * DATE: 2012-07-10
     */
    void initParam_IS12S(){
        mIsIS12S = true;
    }

    void initParam_SO_02C(){
        initParam_IS11S();
    }

    /*
     * FIX BUG: 1212 1213 1471 1473
     * BUG CAUSE: don't add the video size for DOOV D3,D9,D50
     * FIX COMMENT:add the video size
     * DATE: 2012-07-03 2012-08-17
     */
    void initParam_DOOV_D3(){
        mIsDOOV_D3 = true;
        initParam_DOOV_DXX();
    }

    void initParam_DOOV_D9(){
        mIsDOOV_D9 = true;
        initParam_DOOV_DXX();
    }

    void initParam_DOOV_D50(){
        mIsDOOV_D50 = true;
        initParam_DOOV_DXX();
    }

    void initParam_DOOV_DXX(){

    }

    /*
     * FIX BUG: 1217
     * BUG CAUSE: don't add the video size for ZOPO ZP200
     * FIX COMMENT:add the video size
     * DATE: 2012-07-04
     */
    void initParam_ZP200(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsZP200_ICS = true;
        }
    }

    void initParam_X907(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH){
            mIsOPPOX907_ICS = true;
        }
    }
    void initParam_msm7627a(){
        mIsMsm7627a = true;
    }

    /*
     * FIX BUG: 1315 1380
     * BUG CAUSE: don't add the video size for KONKA W960
     * FIX COMMENT:add the video size,add record sound
     * DATE: 2012-07-24 2012-07-30
     */
    void initParam_W960(){
        mIsW960 = true;
    }

    /*
     * FIX BUG: 1343
     * BUG CAUSE: don't add the video size for REGZA IS04
     * FIX COMMENT:add the video size
     * DATE: 2012-07-30
     */
    void initParam_IS04(){
        mIsIS04 = true;
    }

    void initParam_N_06C(){
        mIsN06C = true;
    }

    /*
     * FIX BUG: 1522
     * BUG CAUSE: don't add the video size for Lenovo A750
     * FIX COMMENT:add the video size
     * DATE: 2012-08-29
     */
    void initParam_Lenovo_A750(){
       mIsLenovoA750 = true;
    }

    /*
     * FIX BUG: 1523
     * BUG CAUSE: don't add the video size for HTC Desire G7
     * FIX COMMENT:add the video size
     * DATE: 2012-08-29
     */
    void initParam_HTC_Desire(){
       mIsHTCDesire = true;
    }

    void initParam_msm8x25(){
        mIsMsm8x25 = true;
    }

    // HTC G17
    void initParam_ISW12HT()         { mIsHTCG17 = true;}
    void initParam_HTC_EVO_3D_X515m(){ mIsHTCG17 = true;}
    void initParam_HTC_X515m()       { mIsHTCG17 = true;}

    public boolean getOrientationRecordable() {
        if (mIsMotoManufacturer) return false;
        if (mIsSamsung9000) return false;
        return !Arrays.asList(new String[]{
                    "mIsGTP1000",
                    "IS11PT",
                    "ISW11HT",
                    "PC36100",
                    "ISW11M",
                    "IS06",
                    "LG_P970"
                }).contains(mSerialModel.replace('-', '_').replace(' ', '_'));
    }

}
