
package com.spreadtrum.sgps;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.regex.PatternSyntaxException;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

public class NmeaParser {

    private static final String TAG = NmeaParser.class.getSimpleName();

    private static final String NMEA_HEADER_GPGGA = "$GPGGA";
    private static final String NMEA_HEADER_GPRMC = "$GPRMC";
    private static final String NMEA_HEADER_GPGLL = "$GPGLL";
    private static final String NMEA_HEADER_GPGSA = "$GPGSA";
    private static final String NMEA_HEADER_GPVTG = "$GPVTG";

    private static final String NMEA_HEADER_SUFFIX_GGA = "GGA";
    private static final String NMEA_HEADER_SUFFIX_GSA = "GSA";
    private static final String NMEA_HEADER_SUFFIX_GSV = "GSV";
    private static final String NMEA_HEADER_SUFFIX_VTG = "VTG";
    private static final String NMEA_HEADER_GPS = "$GPGSV";
    private static final String NMEA_HEADER_GLONASS = "$GLGSV";
    private static final String NMEA_HEADER_BDS = "$BDGSV";

    // contains header tag,total sentence count,current sentence number,total satellites
    private static final int NMEA_STATEMENT_GSV_HEADERS_COUNT = 4;
    // A statement contains 4 satellite information
    private static final int NMEA_STATEMENT_GSV_SATELLITES_COUNT = 4;
    private static final int NMEA_STATEMENT_HEADER_LENGTH = 6;
    private static final int MAX_NMEA_STATEMENT_COUNT = 9;
    private static final int MIN_NMEA_STATEMENT_INDEX = 0;
    private static final int NUM_SATELLITES = 256;

    private static List<GpsSatellite> mSatellitesList = null;
    private static List<Integer> mPrnsList = null;
    private static boolean mFixAvailable = false;

    private LocalNmeaListener localNmeaListener = null;

    interface LocalNmeaListener {
        void updateSatelliteView(List<com.spreadtrum.sgps.GpsSatellite> list);

        void locationFixed(boolean available);
        void setSatelliteStatusForGe2(List<com.spreadtrum.sgps.GpsSatellite> list);
    }

    void setLocalNmeaListener(LocalNmeaListener listener) {
        localNmeaListener = listener;
    }

    NmeaParser(Context context) {
        mPrnsList = new ArrayList<Integer>();
        mSatellitesList = new ArrayList<GpsSatellite>();
    }

    public void parserNmeaStatement(String nmea) {
        Log.d(TAG, "parserNmeaStatement input nmea is " + nmea);
        if (isValidStatement(nmea)) {
        } else {
            Log.d(TAG, "Nmea statement can not parser");
        }
    }

    boolean isValidStatement(String nmea) {
        // TODO only support ge2 need support ge
        if (!TextUtils.isEmpty(nmea) && nmea.length() > NMEA_STATEMENT_HEADER_LENGTH) {
            String header = nmea.substring(0, 6);
            Log.d(TAG, "nmea header is " + header);
            if (header.endsWith(NMEA_HEADER_SUFFIX_GSA)) {
                parserNmeaGPGSA(nmea);
                return true;
            } else if (header.endsWith(NMEA_HEADER_SUFFIX_GSV)) {
                parserNmeaGPGSV(nmea);
                return true;
            } else if (header.endsWith(NMEA_HEADER_SUFFIX_GGA)) {
                // ge2 once start
                resetmSatellitesData();
                return true;
            } else if (header.endsWith(NMEA_HEADER_SUFFIX_VTG)) {
                // ge2 once end and update ui
                if (localNmeaListener != null) {
                    Log.d(TAG, "updateSatelliteViewAndLocationInfo + mFixAvailable : "
                            + mFixAvailable);
                    localNmeaListener.locationFixed(mFixAvailable);
                    localNmeaListener.updateSatelliteView(mSatellitesList);
                    localNmeaListener.setSatelliteStatusForGe2(mSatellitesList);
                }
                return true;
            }
        }
        return false;
    }

    private void parserNmeaGPGSV(String nmea) {
        List<String> gpgsv = splitNmeaStatement(nmea);
        if (gpgsv != null) {
            int totalSentence = Integer.parseInt(gpgsv.get(1));
            int currentSentence = Integer.parseInt(gpgsv.get(2));
            if (currentSentence > totalSentence) {
                return;
            }
            int gpgsvSize = gpgsv.size();
            // gpgsvSize = 4(HEADER,total sentence,current sentence,total satellite) +
            // 4(prn,elevation,azimuth,snr)*gpsSatelliteCount
            // $GPGSV,3,2,9,22,36,78,30,28,83,280,36,3,5,,38,33,5,,33*4E
            int gpsSatelliteCount = (gpgsvSize - NMEA_STATEMENT_GSV_HEADERS_COUNT)
                    / NMEA_STATEMENT_GSV_SATELLITES_COUNT;
            try {
                for (int i = 1; i <= gpsSatelliteCount; i++) {
                    int prn = 0;
                    float elevation = 0.0f;
                    float azimuth = 0.0f;
                    float snr = 0.0f;
                    for (int j = 1; j <= 4; j++) {
                        // 3,0~3 is header tag,total sentence count,current sentence number,total satellites index
                        // (i - 1) * 4, is which satellites
                        // j,one satellite info (prn,elevation,azimuth,snr)
                        int index = 3 + (i - 1) * 4 + j;
                        String item = gpgsv.get(index);
                        if (j == 1) {
                            if (!TextUtils.isEmpty(item)) {
                                prn = Integer.parseInt(item);
                            }
                        } else if (j == 2) {
                            if (!TextUtils.isEmpty(item)) {
                                elevation = Float.parseFloat(item);
                            }
                        } else if (j == 3) {
                            if (!TextUtils.isEmpty(item)) {
                                azimuth = Float.parseFloat(item);
                            }
                        } else if (j == 4) {
                            if (index == gpgsvSize - 1) {
                                String s = gpgsv.get(gpgsvSize - 1);
                                if (!TextUtils.isEmpty(s)) {
                                    int getTempIndex = s.indexOf("*");
                                    if (getTempIndex > 0) {
                                        snr = Float.parseFloat(s.substring(0, getTempIndex));
                                    }
                                }
                            } else {
                                if (!TextUtils.isEmpty(item)) {
                                    snr = Float.parseFloat(item);
                                }
                            }
                        }
                    }
                    GpsSatellite satellite = new GpsSatellite(prn);
                    satellite.mValid = true;
                    satellite.mSnr = snr;
                    satellite.mElevation = elevation;
                    satellite.mAzimuth = azimuth;
                    if (mPrnsList != null && !mPrnsList.isEmpty() && mPrnsList.contains(prn)) {
                        Log.d(TAG, "prn " + prn
                                + "set m HasAlmanac , mHasEphemeris and mUsedInFix TRUE");
                        satellite.mHasEphemeris = true;
                        satellite.mHasAlmanac = true;
                        satellite.mUsedInFix = true;
                    }
                    mSatellitesList.add(satellite);
                    Log.d(TAG, "satellite " + i + " , prn is " + prn + " , elevation is "
                            + elevation
                            + " , azimuth is " + azimuth + " , snr is " + snr);
                }
                Collections.sort(mSatellitesList);
            } catch (NumberFormatException e) {
                e.printStackTrace();
            } catch (IndexOutOfBoundsException e) {
                e.printStackTrace();
            }
        } else {
            Log.d(TAG, "parser nmea GPGSV error !");
        }
    }

    private void parserNmeaGPGSA(String nmea) {
        List<String> gpgsa = splitNmeaStatement(nmea);
        if (gpgsa != null) {
            // parser only 2D/3D
            // gpgsa.get(2) is 2 means ,2D 3 means 3D , 1 means fix not available
            boolean fixAvailable = !"1".equals(gpgsa.get(2));
            mFixAvailable = fixAvailable;
            if (fixAvailable) {
                for (int i = 3; i <= 14; i++) {
                    if (TextUtils.isEmpty(gpgsa.get(i))) {
                        Log.d(TAG, "parser nmea GPGSA field " + i + " no value !");
                        break;
                    } else {
                        mPrnsList.add(Integer.parseInt(gpgsa.get(i)));
                    }
                }
            }
        } else {
            Log.d(TAG, "parser nmea GPGSA error !");
        }
    }

    private static List<String> splitNmeaStatement(String nmea) {
        try {
            return Arrays.asList(nmea.split(","));
        } catch (NullPointerException e) {
            e.printStackTrace();
        } catch (PatternSyntaxException e) {
            e.printStackTrace();
        }
        return null;
    }

    private void resetmSatellitesData() {
        if (mPrnsList != null) {
            mPrnsList.clear();
        }
        if (mSatellitesList != null) {
            mSatellitesList.clear();
        }
    }
}
