/*
 * Copyright (C) 2011 Deutsche Telekom, A.G.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contributed by: Giesecke & Devrient GmbH.
 */

package org.simalliance.openmobileapi.service.security.arf.PKCS15;

import android.util.Log;

import java.util.Vector;

import org.simalliance.openmobileapi.service.security.ApduFilter;
import org.simalliance.openmobileapi.service.security.ChannelAccess;
import org.simalliance.openmobileapi.service.security.arf.ASN1;
import org.simalliance.openmobileapi.service.security.arf.DERParser;
import org.simalliance.openmobileapi.service.security.arf.SecureElement;
import org.simalliance.openmobileapi.internal.ByteArrayConverter;
import org.simalliance.openmobileapi.internal.Util;
import org.simalliance.openmobileapi.service.security.gpac.dataobjects.AID_REF_DO;
import org.simalliance.openmobileapi.service.security.gpac.dataobjects.Hash_REF_DO;

/**
 * EF_ACConditions related features
 ***************************************************/
public class EFACConditions extends EF {

    public static final String TAG = "ACE ARF EF_ACConditions";

    // Identification of the cardlet
    private AID_REF_DO mAid_Ref_Do=null;

    private byte[] mData = null;

    /**
     * Constructor
     * @param secureElement SE on which ISO7816 commands are applied
     * @param AID Identification of the applet
     */
    public EFACConditions(SecureElement handle,AID_REF_DO Aid_Ref_Do) {
         super( handle );
         mAid_Ref_Do=Aid_Ref_Do;
    }

    /**
     * Decodes EF_ACConditions file
     * @param buffer ASN.1 data
     */
    private void decodeDER(byte[] buffer)
        throws PKCS15Exception
    {
        Log.d(TAG, "decodeDER DER > " + ByteArrayConverter.byteArrayToHexString(buffer));
        byte[] certificateHash=null;
        DERParser DER=new DERParser(buffer);

        // the default channelAccess will deny every access.
        ChannelAccess channelAccess = new ChannelAccess();
        Hash_REF_DO hash_ref_do = new Hash_REF_DO();
        Log.d(TAG,"default hash_ref_do : " + hash_ref_do.toString());

        // empty condition file
        if (DER.isEndofBuffer()) {
            mSEHandle.putAccessRule(mAid_Ref_Do, hash_ref_do, channelAccess);
            return;
        }
        Log.d(TAG, "condition file not empty!");

        //----
        // 2012-04-16
        /*
           Condition ::= SEQUENCE {
                  cert   CertHash OPTIONAL,
                  accessRules    [0]AccessRules OPTIONAL
            }

            AccessRules ::= SEQUENCE OF AccessRule

            AccessRule ::=CHOICE {
                apduAccessRule    [0]APDUAccessRule,
                nfcAccessRule    [1]NFCAccessRule
            }

            APDUAccessRule ::= CHOICE {
                   apduPermission [0] APDUPermission,
                   apduFilter [1] APDUFilter
            }

            APDUFilters ::= SEQUENCE OF APDUFilter

            NFCAccessRule ::= CHOICE {
                   nfcPermission [0] NFCPermission
            }
         */
        try {
            while(!DER.isEndofBuffer()) {

                // if a hash value was found then access is allowed
                // even if NO more access rule is given.
                // missing APDU Permission will always allow APDU access
                // missing NFC Permission will always allow NFC event.
                // See GPAC Chapter 7.1.7
                // See Examples in Annex C of GPAC
                channelAccess = new ChannelAccess();
                channelAccess.setAccess(ChannelAccess.ACCESS.ALLOWED, "");
                channelAccess.setApduAccess(ChannelAccess.ACCESS.ALLOWED);
                channelAccess.setNFCEventAccess(ChannelAccess.ACCESS.ALLOWED);
                Log.d(TAG, "decodeDER setUseApduFilter false");
                boolean ruleParseSuccess = true;
                boolean certificateContextWrong = false;
                channelAccess.setUseApduFilter(false);

                if ( DER.parseTLV(ASN1.TAG_Sequence) > 0 ) {                // Check for 0x30
                    Log.d(TAG, "decodeDER derRule > Check for 0x30;");
                    byte[] DERdata = DER.getTLVData();
                    Log.d(TAG, "decodeDER derRule > " + ByteArrayConverter.byteArrayToHexString(DERdata));
                    if (DERdata.length >= 2 && DERdata[0] == ASN1.TAG_Padding
                            && DERdata[1] == ASN1.TAG_Padding) {
                        Log.d(TAG, "decodeDER derRule certificateContextWrong");
                        certificateContextWrong = true;
                    }

                    DERParser derRule = new DERParser(DERdata);
                    short[] context = null;
                    try {

                        context = derRule.saveContext();
                        derRule.parseTLV(ASN1.TAG_OctetString);                 // Check for 0x04 which is used for cert hash, throws exception if not found
                        certificateHash=derRule.getTLVData();

                        if (certificateHash.length!=Hash_REF_DO._SHA1_LEN &&
                                certificateHash.length!=0) {
                            // other hash than SHA-1 hash values are not supported.
                            throw new PKCS15Exception("Invalid hash found!");
                        } else {
                            Log.d(TAG, "certificateHash : " + ByteArrayConverter.byteArrayToHexString(certificateHash));
                            hash_ref_do =new Hash_REF_DO(certificateHash);
                        }
                    } catch ( PKCS15Exception e ) {

                        // cert hash is OPTIONAL so TAG_OctetString might be missing
                        // this means this access condiction applies to any device application.
                        hash_ref_do =new Hash_REF_DO();
                        Log.d(TAG,"certificateHash catch PKCS15Exception" + hash_ref_do.toString());
                        // if a context could be stored, restore it so that we can continue parsing...
                        if( context != null ){
                            derRule.restoreContext(context);
                        }
                    }

                    // 2012-04-16
                    // parse optional Access Rule.
                    if( !derRule.isEndofBuffer() ) {

                        Log.d(TAG, "derRule parse optional Access Rule into");
                        if( derRule.parseTLV() == (byte)0xA0 ) {
                            Log.d(TAG, "derRule.parseTLV == 0xA0");
                            byte[] derRuletlvdata = derRule.getTLVData();
                            Log.d(TAG, "decodeDER derAccessRules > " + ByteArrayConverter.byteArrayToHexString(derRuletlvdata));
                            DERParser derAccessRules = new DERParser(derRuletlvdata);

                            while(!derAccessRules.isEndofBuffer()) {
                                Log.d(TAG, "derAccessRules into");
                                switch( derAccessRules.parseTLV() ){
                                // APDU Access Rule
                                case (byte)0xA0:
                                Log.d(TAG, "derAccessRules.parsxeTLV() == 0xA0");
                                byte[] derAccessRulestlvdata = derAccessRules.getTLVData();
                                Log.d(TAG, "decodeDER derAccessRules > " + ByteArrayConverter.byteArrayToHexString(derAccessRulestlvdata));
                                DERParser derApduRule = new DERParser(derAccessRulestlvdata);
                                byte tagApduAccessRule = derApduRule.parseTLV();


                                if( tagApduAccessRule == (byte)0x80 ) { // APDU Permission  (primitive)
                                    Log.d(TAG, "tagApduAccessRule == 0x80");
                                    ChannelAccess.ACCESS apduAccess = derApduRule.getTLVData()[0] == 0x01 ? ChannelAccess.ACCESS.ALLOWED : ChannelAccess.ACCESS.DENIED;
                                    channelAccess.setApduAccess(apduAccess);
                                } else if( tagApduAccessRule == (byte)0xA1 ) { // APDU Filter (constructed)
                                    Log.d(TAG, "tagApduAccessRule == 0xA1");
                                    byte[] derApduRuledata = derApduRule.getTLVData();
                                    Log.d(TAG, "decodeDER derApduFilter > " + ByteArrayConverter.byteArrayToHexString(derApduRuledata));
                                    DERParser derApduFilter = new DERParser(derApduRuledata);
                                    byte tag = derApduFilter.parseTLV();

                                    if( tag == ASN1.TAG_OctetString ) {

                                        Log.d(TAG, "tag == ASN1.TAG_OctetString");
                                        Vector<ApduFilter> apduFilter = new Vector<ApduFilter>();

                                        // collect all apdu filter tlvs.
                                        apduFilter.add(new ApduFilter( derApduFilter.getTLVData()));

                                        while( !derApduFilter.isEndofBuffer()) {
                                            if( derApduFilter.parseTLV() == ASN1.TAG_OctetString ) {
                                                byte[] derApduFilterdata = derApduFilter.getTLVData();
                                                Log.d(TAG, "decodeDER derApduFilter > " + ByteArrayConverter.byteArrayToHexString(derApduFilterdata));
                                                apduFilter.add(new ApduFilter(derApduFilterdata));
                                            }
                                        }
                                        channelAccess.setUseApduFilter(true);
                                        channelAccess.setApduFilter(apduFilter.toArray(new ApduFilter[apduFilter.size()]));
                                    } else {
                                        Log.d(TAG, "PKCS15Exception-1");
                                        throw new PKCS15Exception("Invalid element found!");
                                    }

                                } else {
                                    Log.d(TAG, "PKCS15Exception-2");
                                    throw new PKCS15Exception("Invalid element found!");
                                }
                                break;
                                // NFC Access Rule
                                case (byte)0xA1:
                                    Log.d(TAG, "nfc derAccessRules == 0xA1");
                                DERParser derNfc = new DERParser(derAccessRules.getTLVData());

                                if( derNfc.parseTLV() == (byte)0x80 ) { // NFC Permission (primitive)

                                    channelAccess.setNFCEventAccess(
                                            derNfc.getTLVData()[0] == (byte)0x01 ? ChannelAccess.ACCESS.ALLOWED : ChannelAccess.ACCESS.DENIED);
                                } else {
                                    Log.d(TAG, "PKCS15Exception-3");
                                    throw new PKCS15Exception("Invalid element found!");
                                }
                                break;
                                default:
                                    Log.d(TAG, "PKCS15Exception-4");
                                    throw new PKCS15Exception("Invalid element found!");
                                }
                            }
                        } else {
                            ruleParseSuccess = false;
                            Log.d(TAG, "no explicit access rule given");
                            // no explicit access rule given.
                        }
                    } else {
                        Log.d(TAG, "parse optional Access Rule derRule isEndofBuffer");
                    }
                } else {
                    Log.d(TAG, "coding 30 00 -> empty hash value given (all applications)");
                    // coding 30 00 -> empty hash value given (all applications)
                }
                //----
                if (!ruleParseSuccess) {
                    Log.d(TAG, "ruleParseSuccess false set ChannelAccess DENIED");
                    channelAccess.setApduAccess(ChannelAccess.ACCESS.DENIED);
                    mSEHandle.setChannelAccessDenied(mAid_Ref_Do);
                } else if (certificateContextWrong) {
                    Log.d(TAG, "certificateContextWrong set ChannelAccess DENIED");
                    channelAccess.setApduAccess(ChannelAccess.ACCESS.DENIED);
                    mSEHandle.setChannelAccessDenied(mAid_Ref_Do);
                }
                Log.d(TAG, "putAccessRule");
                mSEHandle.putAccessRule(mAid_Ref_Do, hash_ref_do, channelAccess);
            } ;
        } catch (Exception e){
            mSEHandle.setChannelAccessDenied(mAid_Ref_Do);
            Log.e(TAG, "Exception: " + e.getMessage());
        }
    }


    /**
     * Stores a restricted list of certificate hashes
     * @param path Path of the "EF_ACConditions" file
     */
    public void addRestrictedHashes(byte[] path) {
        try {
            Log.v(TAG,"Reading and analysing EF_ACConditions...");
            if (selectFile(path) == APDU_SUCCESS) {
                mData = readBinary(0,Util.END);
                decodeDER(mData);
            } else {
                Log.e(TAG,"EF_ACConditions not found!");
            }
        } catch (Exception e) {
            /*Nothing to do*/
            Log.e( TAG, "Exception: " + e.getMessage());
        }
    }

    /**
     * Stores a restricted list of certificate hashes
     * @param path Path of the "EF_ACConditions" file
     */
    public void addRestrictedHashesFromData(byte[] data) {
        try {
            Log.v(TAG,"Analysing cached EF_ACConditions data...");
            if( data != null ) {
                mData = data;
                decodeDER(mData);
            } else {
                Log.e(TAG,"EF_ACConditions data not available!");
            }
        } catch (Exception e) {
            /*Nothing to do*/
            Log.e( TAG, "Exception: " + e.getMessage());
        }
    }

    public byte[] getData() {
        return mData;
    }

}

