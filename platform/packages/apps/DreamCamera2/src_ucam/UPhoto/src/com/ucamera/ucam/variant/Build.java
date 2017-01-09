/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.variant;

import static com.ucamera.ucam.variant.Build.Feature.*;


public class Build {

    /**
     * Variant Name for this build.
     *
     * NOTE: May be changed when building.
     */
    public static final String VARIANT                  = "UCam";

    //Convinience Methods for variant compare
    public static final boolean isCK()        {return VARIANT.equals("CK");        }
    public static final boolean isDoov()      {return VARIANT.equals("Doov");      }
    public static final boolean isKonka()     {return VARIANT.equals("Konka");     }
    public static final boolean isMeizu()     {return VARIANT.equals("Meizu");     }
    public static final boolean isOrange()    {return VARIANT.equals("Orange");    }
    public static final boolean isSpeedup()   {return VARIANT.equals("SpeedUp");   }
    public static final boolean isKDDI()      {return VARIANT.equals("KDDI");      }
    public static final boolean isTapnow()    {return VARIANT.equals("Tapnow");    }
    public static final boolean isQrd()       {return VARIANT.equals("Qrd");       }
    public static final boolean isMopita()    {return VARIANT.equals("Mopita");    }
    public static final boolean isSNApp()     {return VARIANT.equals("SourceNext");}
    public static final boolean isDocomo()    {return VARIANT.equals("Docomo");    }
    public static final boolean isKingSun()   {return VARIANT.equals("KingSun");   }
    public static final boolean isQiHu()      {return VARIANT.equals("QiHu");      }
    public static final boolean isLajiao()    {return VARIANT.equals("Lajiao");    }
    public static final boolean isHiApk()     {return VARIANT.equals("HiApk");     }
    public static final boolean isDopod()     {return VARIANT.equals("Dopod");     }
    public static final boolean isHosin()     {return VARIANT.equals("Hosin");     }
    public static final boolean isAnZhiSP()   {return VARIANT.equals("ANZHISP");   }
    public static final boolean isPepper()    {return VARIANT.equals("Pepper");    }
    public static final boolean isGoogle()    {return VARIANT.equals("Google");    }
    public static final boolean isGooglePro() {return VARIANT.equals("GooglePro"); }
    public static final boolean isHiApkNor()  {return VARIANT.equals("HIAPK-NOR"); }
    public static final boolean is91()        {return VARIANT.equals("91");        }
    public static final boolean is91Nor()     {return VARIANT.equals("91-NOR");    }
    public static final boolean isVestel()    {return VARIANT.equals("Vestel");    }
    public static final boolean isHoDai()     {return VARIANT.equals("HODAI");     }
    public static final boolean isTelecomCloud()   {return VARIANT.equals("TelecomCloud") || "CT_CLOUDPHONE".equals(android.os.SystemProperties.get("Build.VERSION.INCREMENTAL"));}
    public static final boolean isSourceNext(){
        return isKDDI() || isTapnow() || isMopita() || isSNApp() || isDocomo() || isHoDai();
    }
    public static final boolean isPreInstall() {
        return isQrd() || isTelecomCloud() || isDopod() || isSpeedup() || isHosin() || isPepper() || isKonka();
    }

    public static final boolean isBaiduFamily() {
        return VARIANT.equals("Baidu") || VARIANT.equals("91") || VARIANT.equals("HiApk");
    }

    /////////////////////////////////////////////////////////////////
    //NOTE: Features may be changed when building
    /////////////////////////////////////////////////////////////////
    public static final Feature SHUTTER_SOUND_ENFORCED  = OFF;
    public static final Feature LICENSE_CHECK           = OFF;
    public static final Feature MAKE_UP                 = ON;
    public static final Feature PANORAMA                = ON;
    public static final Feature PORTRAIT_PANORAMA       = ON;
    public static final Feature SCENERY                 = ON;
    public static final Feature SHOW_PERMISSION_NOTICE  = OFF;
    public static final Feature UCAM_FULL_SIZE          = OFF;
    public static final Feature FEEDBACK                = ON;
    public static final Feature RATING                  = ON;
    public static final Feature UPGRADE_CHECK           = ON;
    public static final Feature ABOUT_US                = ON;
    public static final Feature SPLASHACTIVITY          = OFF;
    public static final Feature LAUNCHERON              = OFF;
    public static final Feature MANGA                   = ON;
    public static final Feature IDPHOTO                 = OFF;
    public static final Feature SHAKE                   = ON;
    public static final Feature XIAOMI_PUSH             = ON;

   /*
    *  Whether this release is for test only.
    *  Used for KDDI/Docomo Update notice check.
    *
    *  Please see: com.ucamera.ucam.launcher.settings.CheckVersion.Release.fromPkg(String)
    */
    public static final Feature TEST_VERSION            = OFF;

    // BELOW NOT USED YET
    public static final Feature MAGIC_LENS              = OFF;
    public static final Feature OPTIMIZED_TIMESTAMP     = OFF;
    public static final Feature MAGNIFIER               = ON;
    public static final Feature EVENT                   = ON;
    public static final Feature HOTAPP                  = ON;
    public static final Feature SNS_SITE                = ON;
    public static final Feature SNS_SHARE_ON            = OFF;
    public static final Feature UCAM_NAME               = ON;
    public static final Feature UPHOTO_NAME             = ON;
    public static final Feature UGIF_NAME               = ON;
    public static final Feature UCAM_ICON               = ON;
    public static final Feature UPHOTO_ICON             = ON;
    public static final Feature UGIF_ICON               = ON;
    public static final Feature CREATE_SHORTCUT_UPHOTO  = ON;
    public static final Feature CREATE_SHORTCUT_UGIF    = ON;
    public static final Feature SHORTCUT_NOTIFICATION   = ON;
    public static final Feature RESOURCE_DOWNLOAD       = ON;
    public static final Feature PURCHASE_PRO_VERSION    = ON;
    public static final Feature FONT_DOWNLOAD           = ON;
    public static final Feature USHARE                  = OFF;
    public static final Feature UCAM_MENU               = OFF;
    public static final Feature UCAM_KPI                = OFF;
    public static final Feature BUILD_IN_GALLERY        = ON;
    public static final Feature MODULE_MENU_GRID        = ON;
    public static final Feature MODULE_MENU_GALLERY     = OFF;
    public static final Feature CONFIRM_NETWORK_PERMISSION = OFF;

    public static enum Feature {
        ON, OFF, CUSTOM;
        public boolean isOn()     { return this != OFF;    }
        public boolean isOff()    { return this == OFF;    }
        public boolean isCustom() { return this == CUSTOM; }
    }
}
