/*
o * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.compatible;

import android.os.Build;

import com.ucamera.ucam.utils.LogUtils;

import java.util.Arrays;

public abstract class Models {
    // HTC
    public static final String HTCG17        = "HTCG17";
    // QRD
    public static final String Msm8x25_JB    = "MSM8X25_JB";
    public static final String Msm8x25_ICS   = "MSM8X25_ICS";
    public static final String msm7627a      = "msm7627a";
    public static final String msm8x25       = "msm8x25";
    public static final String msm8x25Q      = "QRD_8625DSDS";
    public static final String msm8x30       = "msm8x30";

    public static boolean mIsHTCManufacturer = false; // for htc devices
    public static boolean mIsMotoManufacturer = false; // for Motorola devices
    public static boolean mIsSamsungManufacturer = false; // for Samsung devices

/////////////////////////////////////////////////////////////////////////////////////////
    private static final String sModelName;
    private static final String sSeriesName;
    static {
        sModelName = Build.MODEL.replace('-', '_').replace(' ', '_');
        String series = sModelName;
        {
            String model = Build.MODEL;
            if (SamsungGalaxyS.contains(Build.MODEL)){
                series = SamsungGalaxyS.NAME;
            } else if (SamsungGalaxySII.contains(Build.MODEL)){
                series = SamsungGalaxySII.NAME;
            } else if (SamsungGalaxySIII.contains(Build.MODEL)) {
                series = SamsungGalaxySIII.NAME;
            } else if(SamsungGalaxySIV.contains(Build.MODEL)) {
                series = SamsungGalaxySIV.NAME;
            } else if (SamsungGalaxyRW.contains(Build.MODEL)) {
                series = SamsungGalaxyRW.NAME;
            } else if (SamsungNexusS.contains(Build.MODEL)) {
                series = SamsungNexusS.NAME;
            } else if (MeizuM03x.contains(model)) {
                series = MeizuM03x.NAME;
            } else if (LenovoKX.contains(sModelName)) {
                series = LenovoKX.NAME;
            } else if (OppoX.contains(sModelName)) {
                series = OppoX.NAME;
            } else if (SourceNext.contains(sModelName)) {
                series = SourceNext.NAME;
            } else if(Qrd.contains(sModelName)){
                series = Qrd.NAME;
            }
        }
        sSeriesName = series;

        mIsHTCManufacturer = Build.MANUFACTURER.equalsIgnoreCase("HTC");
        mIsMotoManufacturer = Build.MANUFACTURER.equalsIgnoreCase("motorola");
        mIsSamsungManufacturer = Build.MANUFACTURER.equalsIgnoreCase("samsung");

        LogUtils.debug("Models", "ModeName:%s, SeriesName:%s", sModelName, sSeriesName);
    }


    private Models() {}

    public static String getModel()  {return sModelName; }
    public static String getSeries() {return sSeriesName;}

    public static boolean isSamsungGalaxyS() {
        return SamsungGalaxyS.NAME.equals(sSeriesName);
    }

    public static boolean isSamsungGalaxySII() {
        return SamsungGalaxySII.NAME.equals(sSeriesName);
    }

    public static boolean isSamsungGalaxySIII() {
        return SamsungGalaxySIII.NAME.equals(sSeriesName);
    }

    public static boolean isSamsungGalaxySVI() {
        return SamsungGalaxySIV.NAME.equals(sSeriesName);
    }

    public static boolean isSamsungGalaxyRW() {
        return SamsungGalaxyRW.NAME.equals(sSeriesName);
    }

    public static boolean isSamsungNexusS() {
        return SamsungNexusS.NAME.equals(sSeriesName);
    }

    public static boolean isMeizuM03X() {
        return MeizuM03x.NAME.equals(sSeriesName);
    }
    public static boolean isMeizu() {
        return isMeizuM03X() || Meizu_M9.equals(sModelName) || Meizu_M031.equals(getModel()) || Meizu_M040.equals(getModel());
    }
    public static boolean isLenovoKX() {
        return LenovoKX.NAME.equals(sSeriesName);
    }

    public static boolean isOppoX() {
        return OppoX.NAME.equals(sSeriesName);
    }

    public static boolean isQrd(){
        return Qrd.NAME.equals(sSeriesName);
    }

    public static boolean isFrontVerticalDataMirror(){
        return Arrays.asList(new String[]{
                HTC_Glacier,Meizu_MEIZU_MX,Meizu_M030,SN_IS11N,SN_PC36100,SN_IS11SH,SN_SH_12E,HTC_DOPOD
        }).contains(sModelName);
    }

    public static boolean isFrontAllDataMirror() {
        return Arrays.asList(new String[]{
                SN_IS12SH,
                Meizu_M040
        }).contains(sModelName);
    }

    public static boolean isFrontPreviewMirror(){
        return !Arrays.asList(new String[]{
                SN_SH_12C
        }).contains(sModelName);
    }

    public static boolean isSmallMemoryDevice(){
        return Arrays.asList(new String[]{
                SP7710GA,
                SN_L_04C
        }).contains(sModelName);
    }

    public static boolean isSensorEventSupported() {
        /*
         * FIX BUG: 1482 1516 4017 4830
         * BUG CAUSE: DOOV D9 and D50 not supported SensorEventListener
         * FIX COMMENT: use SensorListener in DOOV D9 and D50
         * DATE: 2012-08-20 2012-08-28 2013-09-09
         */
        return !Arrays.asList(new String[]{msm8x25,msm8x25Q,Huawei_C8812,DOOV_D9,DOOV_D50,
                Lenovo_A288t,Lenovo_A390t,LA_M1,Samsung_GT_S6358,ZTE_V987,/* LAJIAO_LA3_W,*/
                MLW_Telecom_S7,Lenovo_P700,KONKA_K723,KONKA_K3, Coolpad_7260, SN_ISW11F,
                SN_ISW13F, SN_F_10D, Lenovo_A60, Lenovo_K900, CTB_702AK, DOOV_D800, BAY_LAKE}).contains(getModel());
    }
    /////////////////////////////////////////////////////////////////////////////////////
    private abstract static class SamsungGalaxyS {
        public static final String NAME = "GALAXY_S";
        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                "GT-9000",  "GT-9003",  "GT-I9000", "GT-I9000B", "GT-I9000M",
                "GT-I9000T","GT-I9001", "GT-I9008L","GT-I9088",  "SC-02B",
                "SCH-I400", "SCH-I500", "SCH-I909", "SGH-I896",  "SGH-I897",
                "SGH-T959", "SGH-T959D","SGH-T959P","SGH-T959V", "SHW-M110S",
                "SHW-M130K","SHW-M130L","SPH-D700", "GT-I9003"
            }).contains(model);
        }
    }

    private abstract static class SamsungGalaxySII {
        public static final String NAME = "GALAXY_SII";
        public static boolean contains(String model) {
            return Arrays.asList(new String[] {
                "GT-I9100", "GT-I9100G", "GT-I9100M", "GT-I9100T", "GT-I9108",
                "SC-02C",    "SGH-I727", "SGH-I777",  "SGH-I927",  "SGH-T989",
                "SGH-T989D", "SGH-i727R","SHW-M250K", "SHW-M250L", "SHW-M250S",
                "SPH-D710", "SC-03D", "SHV-E120L"
            }).contains(model);
        }
    }

    private abstract static class SamsungGalaxySIII {
        public static final String NAME = "GALAXY_SIII";
        public static boolean contains(String model) {
            return Arrays.asList(new String[] {
                "GT-I9300",
                "GT-I9305"
            }).contains(model);
        }
    }

    private abstract static class SamsungGalaxySIV {
        public static final String NAME = "GALAXY_SIV";
        public static boolean contains(String model) {
            return Arrays.asList(new String[] {
                    "GT-I9500",
                    "GT-I9502",
                    "GT-I9508"
            }).contains(model);
        }
    }

    private abstract static class SamsungGalaxyRW {
        public static final String NAME = "GALAXY_RW";
        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                "GT-I9103","SGH-T679","GT-I8150"
            }).contains(model);
        }
    }

    private abstract static class SamsungNexusS {
        public static final String NAME = "NEXUS_S";
        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                "GT-I9020",      "GT-I9020A","GT-I9020T", "GT-I9023",
                "Google Nexus S","Nexus S",  "SPH-D720"
            }).contains(model);
        }
    }

    private abstract static class MeizuM03x{
        public static final String NAME = "Meizu_M03x";

        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                Meizu_M030,Meizu_M032,Meizu_MEIZU_MX,Meizu_MX
            }).contains(model);
        }
    }

    private abstract static class LenovoKX{
        public static final String NAME = "Lenovo_kx";

        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                Lenovo_K860i
            }).contains(model);
        }
    }

    private abstract static class OppoX{
        public static final String NAME = "Oppo_x";

        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                Oppo_X905, Oppo_X907
            }).contains(model);
        }
    }

    private abstract static class SourceNext{
        public static final String NAME = "SourceNext";

        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                SN_ISW12HT
            }).contains(model);
        }
    }

    public abstract static class Qrd{
        public static final String NAME = "Qrd";
        public static boolean contains(String model) {
            return Arrays.asList(new String[]{
                    Msm8x25_JB,
                    Msm8x25_ICS,
                    msm8x25,
                    msm8x25Q
            }).contains(model);
        }
    }

    public static boolean isLaJiaoPepper() {
        return Arrays.asList(new String[]{
                LAJIAO_LA3_W,
                LAJIAO_LA2_W,
                LAJIAO_LA3S
        }).contains(getModel());
    }
    public static boolean isInhon() {
        return Arrays.asList(new String[]{
                Inhon_MA86,
                Inhon_Xhuriken_EVDO
        }).contains(getModel());
    }

    public static boolean isTablet() {
        return Arrays.asList(new String[]{
                BAY_LAKE
        }).contains(getModel());
    }

    public static boolean isSupportedEffect() {
        return !Arrays.asList(new String[]{Samsung_GT_S6358}).contains(getModel());
    }

    /*
     * FIX BUG: 6081
     * FIX COMMENT: The model which need not to set the mode of screen brightness;
     * Date: 2014.4.14
     */
    public static boolean isSupportedBrightnessMode() {
        return !Arrays.asList(new String[]{
                Xiaomi_MI_ONE_Plus,
                Xiaomi_MI_ONE_C1,
                Xiaomi_MI_TWO,
                Xiaomi_MI_THREE,
                Xiaomi_MI_HM
        }).contains(getModel());
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // DETAIL MODELs
    //////////////////////////////////////////////////////////////////////////////////////
    public static final String ALCATEL_one_touch_995 = "ALCATEL_one_touch_995";
    public static final String Coolpad_7260          = "7260";
    public static final String CTB_702AK             = "CTB_702AK";
    public static final String DOOV_D3               = "DOOV_D3";
    public static final String DOOV_D50              = "DOOV_D50";
    public static final String DOOV_D9               = "DOOV_D9";
    public static final String DOOV_D800             = "DOOV_D800";
    public static final String DOOV_D10              = "DOOV_D10";
    public static final String DOOV_D770             = "DOOV_D770";
    public static final String DROIDX                = "DROIDX";
    public static final String HTC_Desire            = "HTC_Desire";
    public static final String HTC_Desire_HD         = "Desire_HD"; //G10
    public static final String HTC_Desire_HD_HTC     = "HTC_Desire_HD";
    public static final String HTC_Desire_HD_A9191   = "HTC_Desire_HD_A9191";
    public static final String HTC_EVO_3D_X515m      = "HTC_EVO_3D_X515m";
    public static final String HTC_Glacier           = "HTC_Glacier";
    public static final String HTC_HD2               = "HTC_HD2";
    public static final String HTC_Desire_S_HTC      = "HTC_Desire_S"; //G12
    public static final String HTC_Desire_S          = "Desire_S";
    public static final String HTC_ISW13HT           = "ISW13HT";
    public static final String HTC_One_X             = "HTC_One_X";
    public static final String HTC_S710d             = "HTC_S710d";
    public static final String HTC_Sensation_X315e   = "HTC_Sensation_XL_with_Beats_Audio_X315e";
    public static final String HTC_Sensation_Z710e   = "HTC_Sensation_Z710e";
    public static final String HTC_Wildfire          = "HTC_Wildfire"; //G8
    public static final String HTC_X515m             = "HTC_X515m";
    public static final String HTC_Nexus_One         = "Nexus_One";
    public static final String HTC_X920e             = "HTC_X920e";
    public static final String HTC_BUFFERFLY         = "htc_butterfly";
    public static final String Huawei_C8812          = "HUAWEI_C8812";
    public static final String Huawei_U8860          = "U8860";
    public static final String Huawei_U8860_BaiduYun = "HUAWEI_U8860";
    public static final String Huawei_U9500          = "U9500";
    public static final String Huawei_u8800          = "u8800";
    public static final String Huawei_U8825D         = "HUAWEI_U8825D";
    public static final String HUAWEI_C8950D         = "HUAWEI_C8950D";
    public static final String HUAWEI_P6             = "HUAWEI_P6_T00";
    public static final String HUAWEI_T8300          = "HUAWEI_T8300";
    public static final String HUAWEI_G750_T00       = "HUAWEI_G750_T00";
    public static final String ILT_MX100             = "ILT_MX100"; //iriver
    public static final String GOOGLE_Nexus_5        = "Nexus_5";
    public static final String GOOGLE_Nexus_7        = "Nexus_7";
    public static final String KTouch_E800           = "E800";
    public static final String Konka_W960            = "W960";
    public static final String KonKa_E960            = "E960";
    public static final String Konka_V976            = "V976";
    public static final String Konka_V981            = "V981";
    public static final String LG_P970               = "LG_P970";
    public static final String LG_P990               = "LG_P990";
    public static final String LG_P500               = "LG_P500";
    public static final String LG_D802               = "LG_D802";
    public static final String LG_D820               = "LG_D820";
    public static final String LG_D821               = "LG_D821";
    public static final String SN_LG_L_02D           = "L_02D";
    public static final String SN_LG_L_04E           = "L_04E";
    public static final String Lenovo_A750           = "Lenovo_A750";
    public static final String Lenovo_K860i          = "Lenovo_K860i";
    public static final String Lenovo_A2105          = "Lenovo_A2105";
    public static final String Lenovo_A288t          = "Lenovo_A288t";
    public static final String Lenovo_A820t          = "Lenovo_A820t";
    public static final String Lenovo_A390t          = "Lenovo_A390t";
    public static final String Lenovo_A670t          = "Lenovo_A670t";
    public static final String Lenovo_S780e          = "Lenovo_S780e";
    public static final String Lenovo_A60            = "Lenovo_A60";
    public static final String MEDIAS_N_06C          = "N_06C";
    public static final String Meizu_M030            = "M030";
    public static final String Meizu_M031            = "M031";
    public static final String Meizu_M032            = "M032";
    public static final String Meizu_M040            = "M040";
    public static final String Meizu_M9              = "M9";
    public static final String Meizu_MEIZU_MX        = "MEIZU_MX";
    public static final String Meizu_MX              = "MX";
    public static final String Moto_MB525            = "MB525";
    public static final String Moto_MB526            = "MB526";
    public static final String Moto_ME525            = "ME525";
    public static final String Moto_ME811            = "ME811";
    public static final String Moto_ME860            = "ME860";
    public static final String Moto_MT870            = "MT870";
    public static final String Moto_MotoA953         = "MotoA953";
    public static final String Moto_XT883            = "XT883";
    public static final String Moto_XT910            = "XT910";
    public static final String Moto_XT928            = "XT928";
    public static final String Oppo_X905             = "X905";
    public static final String Oppo_X907             = "X907";
    public static final String Oppo_N1T              = "N1T";
    public static final String SN_IS_03              = "IS03";
    public static final String REGZA_IS04            = "IS04";
    public static final String SN_IS_06              = "IS06";
    public static final String SN_F_03D              = "F_03D";
    public static final String SN_F_03E              = "F_03E";
    public static final String SN_F_05D              = "F_05D";
    public static final String SN_F_07D              = "F_07D";
    public static final String SN_F_09D              = "F_09D";
    public static final String SN_F_10D              = "F_10D";
    public static final String SN_INFOBAR_A01        = "INFOBAR_A01";
    public static final String SN_IS04FV              = "IS04FV";
    public static final String SN_IS05               = "IS05";
    public static final String SN_IS11CA             = "IS11CA";
    public static final String SN_IS11F              = "IS11F";
    public static final String SN_IS11N              = "IS11N";
    public static final String SN_IS11PT             = "IS11PT";
    public static final String SN_IS11SH             = "IS11SH";
    public static final String SN_IS12SH             = "IS12SH";
    public static final String SN_IS12F              = "IS12F";
    public static final String SN_ISW12HT            = "ISW12HT"; //HTCG17
    public static final String SN_IS14SH             = "IS14SH";
    public static final String SN_ISW11HT            = "ISW11HT";
    public static final String SN_ISW11M             = "ISW11M";
    public static final String SN_ISW11F             = "ISW11F";
    public static final String SN_ISW13F             = "ISW13F";
    public static final String SN_ISW16SH            = "ISW16SH";
    public static final String SN_PC36100            = "PC36100"; //ISW11HT
    public static final String SN_SHL21              = "SHL21";
    public static final String SN_N06C               = "N06C";
    public static final String SN_L_01E              = "L_01E";
    public static final String SN_L_04C              = "L_04C";
    public static final String SN_L_05D              = "L_05D";
    public static final String SN_L_06D              = "L_06D";
    public static final String SN_L_07C              = "L_07C";
    public static final String SN_P_01D              = "P_01D";
    public static final String SN_P_04D              = "P_04D";
    public static final String SN_P_07D              = "P_07D";
    public static final String SN_N_03E              = "N_03E";
    public static final String SN_N_04D              = "N_04D";
    public static final String SN_N_04C              = "N_04C";
    public static final String SN_N_05D              = "N_05D";
    public static final String SN_N_06C              = "N_06C";
    public static final String SN_N_07D              = "N_07D";
    public static final String SN_SC_02C             = "SC_02C";
    public static final String SN_SC_02E             = "SC_02E";
    public static final String SN_SC_03D             = "SC_03D";
    public static final String SN_SH_01D             = "SH_01D";
    public static final String SN_SH_02E             = "SH_02E";
    public static final String SN_SH_04E             = "SH_04E";
    public static final String SN_SH_03C             = "SH_03C";
    public static final String SN_SH_06D             = "SH_06D";
    public static final String SN_SH_09D             = "SH_09D";
    public static final String SN_SH_12C             = "SH_12C";
    public static final String SN_SH_12E             = "SH_12E";
    public static final String SN_SO_01C             = "SO_01C";
    public static final String SN_SO_02E             = "SO_02E";
    public static final String SN_T_02D              = "T_02D";
    public static final String SP6810A               = "SP6810A";
    public static final String SPHS_on_Hsdroid       = "SPHS_on_Hsdroid";
    public static final String SP7710GA              = "sp7710ga";
    public static final String SP8825EA              = "sp8825ea";
    public static final String Samsung_GT_I5508      = "GT_I5508";
    public static final String Samsung_GT_I5801      = "GT_I5801";
    public static final String Samsung_GT_I9000      = "GT_I9000";
    public static final String Samsung_GT_I9001      = "GT_I9001";
    public static final String Samsung_GT_I9100      = "GT_I9100";
    public static final String Samsung_GT_I9103      = "GT_I9103"; //Galaxy W/RR
    public static final String Samsung_GT_I9195      = "GT_I9195"; //LG nexus 4 use orange rom
    public static final String Samsung_GT_I9220      = "GT_I9220"; //Galaxy Note
    public static final String Samsung_GT_I9300      = "GT_I9300";
    public static final String Samsung_GT_I9500      = "GT_I9500";
    public static final String Samsung_GT_I9502      = "GT_I9502";
    public static final String Samsung_GT_I9508      = "GT_I9508";
    public static final String Samsung_GT_N719       = "GT_N719";
    public static final String Samsung_GT_N7102      = "GT-N7102";
    public static final String Samsung_GT_N7100      = "GT-N7100";
    public static final String Samsung_GT_N7000      = "GT_N7000"; //GT-I9220?
    public static final String Samsung_SM_N900S      = "SM_N900S";
    public static final String Samsung_GT_P1000      = "GT_P1000"; //galaxy tab P1000
    public static final String Samsung_Galaxy_Nexus  = "Galaxy_Nexus";
    public static final String Samsung_Google_Nexus_S= "Google_Nexus_S";
    public static final String Samsung_Nexus_S       = "Nexus_S";
    public static final String Samsung_SGH_I717      = "SGH_I717"; //GT-I9220(USA)
    public static final String Samsung_SGH_I777      = "SGH_I777"; //GT_I9100
    public static final String Samsung_SC_02B        = "SC_02B"; //GT-I9000
    public static final String Samsung_GT_S5830i     = "GT_S5830i";
    public static final String Samsung_GT_S6358      = "GT_S6358";
    public static final String Samsung_SCH_I959      = "SCH_I959";
    public static final String Sky_IM_A760S          = "IM_A760S";
    public static final String Sony_IS11S            = "IS11S";
    public static final String Sony_IS12S            = "IS12S";
    public static final String Sony_LT18i            = "LT18i";
    public static final String Sony_LT26i            = "LT26i";
    public static final String Sony_LT28i            = "LT28i";
    public static final String Sony_LT29i            = "LT29i";
    public static final String Sony_SO_02C           = "SO_02C"; //IS11S
    public static final String Sony_SO_02D           = "SO_02D"; //LT26i
    public static final String Sony_SO_03D           = "SO_03D"; // IS12S
    public static final String Sony_X10i             = "X10i"; //sony
    public static final String ZTE_T_U880            = "ZTE_T_U880";
    public static final String ZTE_U_V880            = "ZTE_U_V880";
    public static final String ZTE_U930              = "ZTE_U930";
    public static final String ZTE_U795              = "ZTE_U795";
    public static final String ZTE_V987              = "ZTE_V987";
    public static final String ZTE_NX503A            = "NX503A";
    public static final String Zopo_ZP200            = "ZP200";
    public static final String Speedup_S6            = "S6";
    public static final String Xiaomi_MI_ONE_Plus    = "MI_ONE_Plus";
    public static final String Xiaomi_MI_ONE_C1      = "MI_ONE_C1"; // xiaomi
    public static final String Xiaomi_MI_TWO         = "MI_2";
    public static final String Xiaomi_MI_THREE         = "MI_3";
    public static final String Xiaomi_MI_HM          = "2013022";
    public static final String KDDI_HTL21            = "HTL21";
    public static final String LG_Nexus_4            = "Nexus_4";
    public static final String LA_M1                 = "LA_M1";
    public static final String GN_708W               = "GN708W";
    public static final String MT6589                = "MT6589";
    public static final String Inhon_MA86            = "Inhon_MA86";
    public static final String Inhon_Xhuriken_EVDO   = "INHON_Xhuriken_EVDO";
    public static final String Lenovo_K900           = "Lenovo_K900";
    public static final String Lenovo_P700           = "Lenovo_P700";
    public static final String MLW_Telecom_S7        = "S7";
    public static final String HUAQIN73CU_GB         = "W708&W706";
    public static final String MSM8660_SURF          = "msm8660_surf";
    public static final String KYY21                 = "KYY21";
    public static final String HOSIN_V908            = "HOSIN_V908";
    public static final String HOSIN_U9              = "HOSIN_U9";
    public static final String HOSIN_U2              = "HOSIN_U2";
    public static final String HOSIN_U6              = "HOSIN_U6";
    public static final String KONKA_K723            = "K723";
    public static final String LAJIAO_LA3_W          = "LA3_W";
    public static final String LAJIAO_LA2_W          = "LA2_W";
    public static final String LAJIAO_LA3S           = "LA3S";
    public static final String AMAZON_KFTT           = "KFTT";
    public static final String KONKA_K3              = "K3";
    public static final String VIVO_S7               = "vivo_S7";
    public static final String SH_06E                = "SH_06E";
    public static final String IS15SH                = "IS15SH";
    public static final String YUYI_D902             = "YUYI_D902";
    public static final String ThL_W11               = "ThL_W11";
    public static final String AMOI_N828             = "AMOI_N828";
    public static final String AMOI_N890             = "AMOI_N890";
    public static final String BAY_LAKE              = "baylake";
    public static final String I96T                  = "i96T";
    public static final String HTC_DOPOD             = "HTC_Flyer_P512";
    public static final String VOLLO_VI80            = "vollo_Vi80";
    public static final String OPSSON_IVO6600        = "OPSSON_IVO6600";

}
