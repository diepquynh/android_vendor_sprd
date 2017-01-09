package com.android.internal.telephony;

import com.android.internal.telephony.uicc.AdnRecordEx;

/**
 * {@hide}
 */
interface IIccPhoneBookEx {

    List<AdnRecordEx> getAdnRecordsInEfForSubscriber(int subId, int efid);

    int[] getAdnRecordsSizeForSubscriber(int subId, int efid);

    boolean updateAdnRecordsInEfBySearchForSubscriber(int subId, int efid,
            String oldTag, String oldPhoneNumber,
            String newTag, String newPhoneNumber,
            String pin2);

    int updateAdnRecordsInEfBySearchForSubscriberEx(int subId, int efid, String oldTag,
            String oldPhoneNumber, in String[] oldEmailList, String oldAnr,
            String oldSne, String oldGrp,
            String newTag, String newPhoneNumber, in String[] newEmailList,
            String newAnr, String newAas, String newSne, String newGrp,
            String newGas, String pin2);

    int updateAdnRecordsInEfByIndexForSubscriber(int subId, int efid, String newTag,
            String newPhoneNumber, in String[] newEmailList, String newAnr,
            String newAas, String newSne, String newGrp, String newGas,
            int index, String pin2);

    int updateUsimGroupBySearchForSubscriber(int subId, String oldName,String newName);

    int updateUsimGroupByIndexForSubscriber(int subId, String newName,int groupId);

    List<String> getGasInEfForSubscriber(int subId);

    boolean isApplicationOnIcc(int appType, int subId);

    int[] getEmailRecordsSize(int subId);

    int[] getAnrRecordsSize(int subId);

    int getAnrNum(int subId);

    int getEmailNum(int subId);

    int getInsertIndex(int subId);

    int[] getAvalibleEmailCount(String name, String number,in String[] emails, String anr, in int[] emailNums, int subId);

    int [] getAvalibleAnrCount(String name, String number,in String[] emails, String anr, in int[] anrNums, int subId);

    int getEmailMaxLen(int subId);

    int getPhoneNumMaxLen(int subId);

    int getUsimGroupNameMaxLen(int subId);

    int getUsimGroupCapacity(int subId);

    List<String> getAasInEfForSubscriber(int subId);

    int updateUsimAasBySearchForSubscriber(String oldName,String newName, int subId);

    int updateUsimAasByIndexForSubscriber(String newName,int aasIndex, int subId);

    int getSneSize(int subId);

    int[] getSneLength(int subId);
}
