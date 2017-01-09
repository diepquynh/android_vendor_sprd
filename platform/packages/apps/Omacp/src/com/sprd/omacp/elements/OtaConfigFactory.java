
package com.sprd.omacp.elements;

import com.sprd.xml.parser.itf.ISet;

class OtaConfigFactory {
    static public ISet CreateInstanceByType(int nType) {

        switch (nType) {
            case ISet.OTA_EMAIL:
                return new Email(nType);
            case ISet.OTA_APN:
                return new APN(nType);
            case ISet.OTA_STARTPAGE:
                return new HomePage(nType);
            case ISet.OTA_BOOKMARK:
                return new BookMark(nType);

                // extended class
            default:
                // print error code
                PrintErrorException(nType);
                return null;

        }
    }

    private static void PrintErrorException(int nType) {
        System.out.println("=====================================================");
        System.out.println("Debug : ======= " + "CreateInstanceByType()"
                + "============>>> has no the type you had put in:" + nType);
        System.out.println("=====================================================");
        System.out.println("\r\n");
    }
}
