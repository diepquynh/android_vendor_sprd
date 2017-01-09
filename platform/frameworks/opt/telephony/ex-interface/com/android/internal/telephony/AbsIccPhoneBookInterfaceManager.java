package com.android.internal.telephony;

import com.android.internal.telephony.uicc.AdnRecordEx;
import java.util.List;

/**
 * {@hide}
 */
public abstract class AbsIccPhoneBookInterfaceManager {

    public AbsIccPhoneBookInterfaceManager() {

    }

    public int[] getEmailRecordsSize() {
        return null;
    };

    public int[] getAnrRecordsSize() {
        return null;
    };

    public int getEmailNum() {
        return 0;
    };

    public int getAnrNum() {
        return 0;
    };

    public int getEmailMaxLen() {
        return 0;
    };

    public int getPhoneNumMaxLen() {
        return 0;
    };

    public int getUsimGroupNameMaxLen() {
        return -1;
    };

    public int getUsimGroupCapacity() {
        return 0;
    };

    public List<AdnRecordEx> getAdnRecordsInEfEx(int efid) {
        return null;
    };

    public int [] getAvalibleEmailCount(String name, String number,
            String[] emails, String anr, int[] emailNums) {
        return null;
    };

    public int [] getAvalibleAnrCount(String name, String number,
            String[] emails, String anr, int[] anrNums) {
        return null;
    };

    public int getInsertIndex() {
        return -1;
    };

    public int updateAdnRecordsInEfBySearch(int efid, String oldTag,
            String oldPhoneNumber, String[] oldEmailList, String oldAnr,
            String oldSne, String oldGrp,
            String newTag, String newPhoneNumber, String[] newEmailList,
            String newAnr, String newAas, String newSne, String newGrp,
            String newGas, String pin2) {
        return -1;
    };

    public int updateAdnRecordsInEfByIndex(int efid, String newTag,
            String newPhoneNumber, String[] newEmailList, String newAnr,
            String newAas, String newSne, String newGrp, String newGas,
            int index, String pin2) {
        return -1;
    };

    public List<String> getGasInEf() {
        return null;
    };

    public int updateUsimGroupBySearch(String oldName,String newName) {
        return -1;
    };

    public int updateUsimGroupByIndex(String newName,int groupId) {
        return -1;
    };

    public boolean isApplicationOnIcc(int type) {
        return false;
    };

    public List<String> getAasInEf(){
        return null;
    }

    public int updateUsimAasBySearch(String oldName,String newName) {
        return -1;
    }

    public int updateUsimAasByIndex(String newName,int aasIndex){
        return -1;
    }

    public int getSneSize(){
        return 0;
    }

    public int[] getSneLength(){
        return null;
    }
}