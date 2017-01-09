
package com.android.internal.telephony.cat;

import java.util.HashMap;
import java.util.Locale;

/**
 * Class used to pass STK messages from telephony to application. Application
 * should call getXXX() to get commands's specific values.
 *
 */
public class CatLanguageDecoder {

    private HashMap <String,String>mLanguageToCountryMap;
    private static CatLanguageDecoder mInstance;

    private CatLanguageDecoder() {
        mLanguageToCountryMap = new HashMap<String,String>();

        //South Africa
        mLanguageToCountryMap.put("af","ZA");

        //Albania
        mLanguageToCountryMap.put("sq","AL");

        //Alba
        mLanguageToCountryMap.put("ar","SA");

        //Chinese
        mLanguageToCountryMap.put("zh","CN");

        //Denmark
        mLanguageToCountryMap.put("da","DK");

        //Holland
        mLanguageToCountryMap.put("nl","NL");

        //England
        mLanguageToCountryMap.put("en","GB");

        //Iran
        mLanguageToCountryMap.put("fa","IR");

        //Finland
        mLanguageToCountryMap.put("fi","FI");

        //France
        mLanguageToCountryMap.put("fr","FR");

        //Germany
        mLanguageToCountryMap.put("de","DE");

        //Indonesia
        mLanguageToCountryMap.put("id","ID");

        //Italy
        mLanguageToCountryMap.put("it","IT");

        //Japan
        mLanguageToCountryMap.put("ja","JP");

        //Korean
        mLanguageToCountryMap.put("ko","KR");

        //Latvija
        mLanguageToCountryMap.put("lv","LV");

        //Norway
        mLanguageToCountryMap.put("nb","NO");

        //Poland
        mLanguageToCountryMap.put("pl","PL");

        //Potugal
        mLanguageToCountryMap.put("pt","PT");

        //Russia
        mLanguageToCountryMap.put("ru","RU");

        //Spain
        mLanguageToCountryMap.put("es","ES");

        //Sweden
        mLanguageToCountryMap.put("sv","SE");

        //Vietnam
        mLanguageToCountryMap.put("vi","VN");

        //Uk
        mLanguageToCountryMap.put("uk","UA");

        //Turkey
        mLanguageToCountryMap.put("tr","TR");

        //Thiland
        mLanguageToCountryMap.put("th","TH");
    }

    public static CatLanguageDecoder getInstance() {
        if(mInstance == null) {
            mInstance = new CatLanguageDecoder();
        }

        return mInstance;
    }

    public String getCountryFromLanguage(String language) {
        if(language == null) {
            return null;
        }
        return mLanguageToCountryMap.get(language);
    }
    //CR119858 Modify Start
    public String getDefaultLanguage() {
        return "en";
    }
    //CR119858 Modify End

}
