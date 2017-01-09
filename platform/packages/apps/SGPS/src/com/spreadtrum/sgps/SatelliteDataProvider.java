package com.spreadtrum.sgps;

public interface SatelliteDataProvider {
    // !!== Enlarge maxSatellites from 15 to 24 for AGPS usage ==
    // !!== Enlarge maxSatellites from 24 to max for multi-GNSS ==
    int MAX_SATELLITES_NUMBER = 256;
    int SATELLITES_MASK_SIZE = 8;
    int SATELLITES_MASK_BIT_WIDTH = 32;

    void setSatelliteStatus(int svCount, int[] prns, float[] snrs,
            float[] elevations, float[] azimuths, int ephemerisMask,
            int almanacMask, int[] usedInFixMask);

    int getSatelliteStatus(int[] prns, float[] snrs, float[] elevations,
            float[] azimuths, int ephemerisMask, int almanacMask,
            int[] usedInFixMask);
};
