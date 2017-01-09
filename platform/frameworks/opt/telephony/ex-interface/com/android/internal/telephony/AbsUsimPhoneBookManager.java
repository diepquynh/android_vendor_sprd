package com.android.internal.telephony;

import java.util.ArrayList;
import java.util.HashMap;
import android.os.Handler;
import com.android.internal.telephony.uicc.AdnRecordEx;

/**
 * {@hide}
 */
public class AbsUsimPhoneBookManager extends Handler {

    public AbsUsimPhoneBookManager(){
        super();
    }

    public ArrayList<AdnRecordEx> loadEfFilesFromUsimEx() {
        return null;
    }

    public ArrayList<byte[]> getIapFileRecord(int recNum) {
        return null;
    }

    public int getNewSubjectNumber(int type, int num, int efid, int index,
            int adnNum, boolean isInIap) {
        return -1;
    }

    public void removeSubjectNumFromSet(int type, int num, int efid, int index,
            int anrNum) {

    }

    public int[] readFileSizeAndWait(int efId) {
        return null;
    }

    public int getEfIdByTag(int recordNum, int fileTag) {
        return -1;
    }

    public int findEFEmailInfo(int index) {
        return 0;
    }

    public int findEFAnrInfo(int index) {
        return 0;
    }

    public void setIapFileRecord(int recNum, int index, byte value, int numInIap) {

    }

    public int[] getAdnRecordSizeArray() {
        return null;
    }

    public int getNumRecs() {
        return 0;
    }

    public int getAnrNum() {
        return 0;
    }

    public int getPhoneNumMaxLen() {
        return -1;
    }

    public int getEmailType() {
        return 1;
    }

    public int getEmailNum() {
        return -2;
    }

    public int[] getValidNumToMatch(AdnRecordEx adn, int type, int[] subjectNums) {
        return null;
    }

    public int[] getAvalibleSubjectCount(int num, int type, int efid,
            int adnNum, int[] subjectNums) {
        return null;
    }

    public int[] getAvalibleAnrCount(String name, String number,
            String[] emails, String anr, int[] anrNums) {
        return null;
    }

    public int[] getAvalibleEmailCount(String name, String number,
            String[] emails, String anr, int[] emailNums) {
        return null;
    }

    public int[] getSubjectEfids(int type, int num) {
        return null;
    }

    public int[][] getSubjectTagNumberInIap(int type, int num) {
        return null;
    }

    public int[][] getAnrTagNumberInIap(int num) {
        return null;
    }

    public boolean isSubjectRecordInIap(int type, int num, int indexOfEfids) {
        return false;
    }

    public int findEFInfo(int index) {
        return -1;
    }

    public int findExtensionEFInfo(int index) {
        return 0;
    }

    public int findEFIapInfo(int index) {
        return 0;
    }

    public int findEFGasInfo() {
        return 0;
    }

    public void updateGasList(String groupName, int groupId) {

    }

    public ArrayList<String> loadGasFromUsim() {
        return null;
    }

    public void setPhoneBookRecords(int index, AdnRecordEx adn) {

    }

    public int getPhoneBookRecordsNum() {
        return 0;
    }

    public synchronized int[] getAdnRecordsSize() {
        return null;
    }

    public boolean isPbrFileExisting() {
        return false;
    }

    public boolean isContainAdnInPbr(){
        return false;
    }

    public HashMap<Integer, int[]> getRecordsSize() {
        return null;
    }

    public int getGrpCount() {
        return 0;
    }

    public ArrayList<String> loadAasFromUsim() {
        return null;
    }

    public int findEFAasInfo() {
        return 0;
    }

    public void updateAasList(String aas, int aasIndex) {

    }

    public int getSneSize() {
        return 0;
    }

    public int[] getSneLength() {
        return null;
    }

    public int findEFSneInfo(int index) {
        return -1;
    }

    public void updateUidForAdn(int adnEf, int recNum, int adnIndex, AdnRecordEx adn) {

    }
}