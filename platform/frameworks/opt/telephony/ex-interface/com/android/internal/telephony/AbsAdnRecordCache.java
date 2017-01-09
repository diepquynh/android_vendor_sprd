package com.android.internal.telephony;

import android.os.Handler;
import android.os.Message;
import java.util.ArrayList;

import com.android.internal.telephony.gsm.UsimPhoneBookManager;
import com.android.internal.telephony.uicc.AdnRecordEx;

/**
 * {@hide}
 */
public abstract class AbsAdnRecordCache extends Handler {

    public AbsAdnRecordCache() {
        super();
    }

    public ArrayList<String> loadGasFromUsim() {
        return null;
    };

    public ArrayList<AdnRecordEx> getRecordsIfLoadedEx(int efid) {
        return null;
    }

    public void removedRecordsIfLoaded(int efid) {

    };

    public void updateAdnByIndexEx(int efid, AdnRecordEx newAdn, int recordIndex, String pin2,
            Message response) {

    }
    public void updateAdnBySearchEx(int efid, AdnRecordEx oldAdn, AdnRecordEx newAdn,
            String pin2, Message response) {

    }
    public synchronized void updateUSIMAdnBySearch(int efid, AdnRecordEx oldAdn,
            AdnRecordEx newAdn, String pin2, Message response) {

    }

    public void insertLndBySearch(int efid, AdnRecordEx oldLnd, AdnRecordEx newLnd, String pin2,
            Message response) {

    }

    public int updateGasBySearch(String oldGas, String newGas) {
        return -1;
    }

    public int updateGasByIndex(String newGas, int groupId) {
        return -1;
    }

    public synchronized void updateUSIMAdnByIndex(int efid, int simIndex, AdnRecordEx newAdn,
            String pin2, Message response) {

    }

    public int getAdnIndex(int efid, AdnRecordEx oldAdn) {
        return -1;
    }

    public UsimPhoneBookManager getUsimPhoneBookManager() {
        return null;
    }

    public int getAdnLikeSize() {
        return -1;
    }

    public int getInsertId() {
        return -1;
    }

    public ArrayList<String> loadAasFromUsim() {
        return null;
    }

    public int updateAasBySearch(String oldAas, String newAas) {
        return -1;
    }

    public int updateAasByIndex(String newAas, int aasIndex) {
        return -1;
    }

    public int getSneSize() {
        return 0;
    }

    public int[] getSneLength(){
        return null;
    }
}