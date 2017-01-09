/*
 * Copyright (C) 2006-2007 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.internal.telephony.cat;

import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.cat.Duration.TimeUnit;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.cat.CatLog;
import com.android.internal.telephony.cat.Duration;
import com.android.internal.telephony.cat.FontSize;
import com.android.internal.telephony.cat.Item;
import com.android.internal.telephony.cat.ResultExceptionSprd;
import com.android.internal.telephony.cat.TextAlignment;
import com.android.internal.telephony.cat.TextAttribute;
import com.android.internal.telephony.cat.TextColor;

import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

abstract class ValueParserSprd {

    /**
     * Search for a Command Details object from a list.
     *
     * @param ctlv List of ComprehensionTlv objects used for search
     * @return An CtlvCommandDetails object found from the objects. If no
     *         Command Details object is found, ResultException is thrown.
     * @throws ResultException
     */
    static CommandDetailsSprd retrieveCommandDetails(ComprehensionTlvSprd ctlv)
            throws ResultExceptionSprd {

    	CommandDetailsSprd cmdDet = new CommandDetailsSprd();
        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        try {
            cmdDet.compRequired = ctlv.isComprehensionRequired();
            cmdDet.commandNumber = rawValue[valueIndex] & 0xff;
            cmdDet.typeOfCommand = rawValue[valueIndex + 1] & 0xff;
            cmdDet.commandQualifier = rawValue[valueIndex + 2] & 0xff;
            return cmdDet;
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }
    }

    /**
     * Search for a Device Identities object from a list.
     *
     * @param ctlv List of ComprehensionTlv objects used for search
     * @return An CtlvDeviceIdentities object found from the objects. If no
     *         Command Details object is found, ResultException is thrown.
     * @throws ResultException
     */
    static DeviceIdentitiesSprd retrieveDeviceIdentities(ComprehensionTlvSprd ctlv)
            throws ResultExceptionSprd {

        DeviceIdentitiesSprd devIds = new DeviceIdentitiesSprd();
        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        try {
            devIds.sourceId = rawValue[valueIndex] & 0xff;
            devIds.destinationId = rawValue[valueIndex + 1] & 0xff;
            return devIds;
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }
    }

    /**
     * Retrieves Duration information from the Duration COMPREHENSION-TLV
     * object.
     *
     * @param ctlv A Text Attribute COMPREHENSION-TLV object
     * @return A Duration object
     * @throws ResultException
     */
    static Duration retrieveDuration(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {
        int timeInterval = 0;
        TimeUnit timeUnit = TimeUnit.SECOND;

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();

        try {
            timeUnit = TimeUnit.values()[(rawValue[valueIndex] & 0xff)];
            timeInterval = rawValue[valueIndex + 1] & 0xff;
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }
        return new Duration(timeInterval, timeUnit);
    }

    /**
     * Retrieves Item information from the COMPREHENSION-TLV object.
     *
     * @param ctlv A Text Attribute COMPREHENSION-TLV object
     * @return An Item
     * @throws ResultException
     */
    static Item retrieveItem(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {
        Item item = null;

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        int length = ctlv.getLength();

        if (length != 0) {
            int textLen = length - 1;

            try {
                int id = rawValue[valueIndex] & 0xff;
                String text = IccUtils.adnStringFieldToString(rawValue,
                        valueIndex + 1, textLen);
                item = new Item(id, text);
            } catch (IndexOutOfBoundsException e) {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        }

        return item;
    }

    /**
     * Retrieves Item id information from the COMPREHENSION-TLV object.
     *
     * @param ctlv A Text Attribute COMPREHENSION-TLV object
     * @return An Item id
     * @throws ResultException
     */
    static int retrieveItemId(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {
        int id = 0;

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();

        try {
            id = rawValue[valueIndex] & 0xff;
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }

        return id;
    }

    /**
     * Retrieves icon id from an Icon Identifier COMPREHENSION-TLV object
     *
     * @param ctlv An Icon Identifier COMPREHENSION-TLV object
     * @return IconId instance
     * @throws ResultException
     */
    static IconIdSprd retrieveIconId(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {
        IconIdSprd id = new IconIdSprd();

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        try {
            id.selfExplanatory = (rawValue[valueIndex++] & 0xff) == 0x00;
            id.recordNumber = rawValue[valueIndex] & 0xff;
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }

        return id;
    }

    /**
     * Retrieves item icons id from an Icon Identifier List COMPREHENSION-TLV
     * object
     *
     * @param ctlv An Item Icon List Identifier COMPREHENSION-TLV object
     * @return ItemsIconId instance
     * @throws ResultException
     */
    static ItemsIconIdSprd retrieveItemsIconId(ComprehensionTlvSprd ctlv)
            throws ResultExceptionSprd {
        CatLog.d("ValueParser", "retrieveItemsIconId:");
        ItemsIconIdSprd id = new ItemsIconIdSprd();

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        int numOfItems = ctlv.getLength() - 1;
        id.recordNumbers = new int[numOfItems];

        try {
            // get icon self-explanatory
            id.selfExplanatory = (rawValue[valueIndex++] & 0xff) == 0x00;

            for (int index = 0; index < numOfItems;) {
                id.recordNumbers[index++] = rawValue[valueIndex++];
            }
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }
        return id;
    }

    /**
     * Retrieves text attribute information from the Text Attribute
     * COMPREHENSION-TLV object.
     *
     * @param ctlv A Text Attribute COMPREHENSION-TLV object
     * @return A list of TextAttribute objects
     * @throws ResultException
     */
    static List<TextAttribute> retrieveTextAttribute(ComprehensionTlvSprd ctlv)
            throws ResultExceptionSprd {
        ArrayList<TextAttribute> lst = new ArrayList<TextAttribute>();

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        int length = ctlv.getLength();

        if (length != 0) {
            // Each attribute is consisted of four bytes
            int itemCount = length / 4;

            try {
                for (int i = 0; i < itemCount; i++, valueIndex += 4) {
                    int start = rawValue[valueIndex] & 0xff;
                    int textLength = rawValue[valueIndex + 1] & 0xff;
                    int format = rawValue[valueIndex + 2] & 0xff;
                    int colorValue = rawValue[valueIndex + 3] & 0xff;

                    int alignValue = format & 0x03;
                    TextAlignment align = TextAlignment.fromInt(alignValue);

                    int sizeValue = (format >> 2) & 0x03;
                    FontSize size = FontSize.fromInt(sizeValue);
                    if (size == null) {
                        // Font size value is not defined. Use default.
                        size = FontSize.NORMAL;
                    }

                    boolean bold = (format & 0x10) != 0;
                    boolean italic = (format & 0x20) != 0;
                    boolean underlined = (format & 0x40) != 0;
                    boolean strikeThrough = (format & 0x80) != 0;

                    TextColor color = TextColor.fromInt(colorValue);

                    TextAttribute attr = new TextAttribute(start, textLength,
                            align, size, bold, italic, underlined,
                            strikeThrough, color);
                    lst.add(attr);
                }

                return lst;

            } catch (IndexOutOfBoundsException e) {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        }
        return null;
    }


    /**
     * Retrieves alpha identifier from an Alpha Identifier COMPREHENSION-TLV
     * object.
     *
     * @param ctlv An Alpha Identifier COMPREHENSION-TLV object
     * @return String corresponding to the alpha identifier
     * @throws ResultException
     */
    static String retrieveAlphaId(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {

        if (ctlv != null) {
            byte[] rawValue = ctlv.getRawValue();
            int valueIndex = ctlv.getValueIndex();
            int length = ctlv.getLength();
            if (length != 0) {
                try {
                    return IccUtils.adnStringFieldToString(rawValue, valueIndex,
                            length);
                } catch (IndexOutOfBoundsException e) {
                    throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
                }
            } else {
                CatLog.d("ValueParser", "Alpha Id length=" + length);
                return null;
            }
        } else {
            /* Per 3GPP specification 102.223,
             * if the alpha identifier is not provided by the UICC,
             * the terminal MAY give information to the user
             * noAlphaUsrCnf defines if you need to show user confirmation or not
             */
            boolean noAlphaUsrCnf = false;
            Resources resource = Resources.getSystem();
            try {
                noAlphaUsrCnf = resource.getBoolean(
                        com.android.internal.R.bool.config_stkNoAlphaUsrCnf);
            } catch (NotFoundException e) {
                noAlphaUsrCnf = false;
            }
            return (noAlphaUsrCnf ? null : CatServiceSprd.STK_DEFAULT);
        }
    }

    /**
     * Retrieves text from the Text COMPREHENSION-TLV object, and decodes it
     * into a Java String.
     *
     * @param ctlv A Text COMPREHENSION-TLV object
     * @return A Java String object decoded from the Text object
     * @throws ResultException
     */
    static String retrieveTextString(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {
        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        byte codingScheme = 0x00;
        String text = null;
        int textLen = ctlv.getLength();

        // In case the text length is 0, return a null string.
        if (textLen == 0) {
            return text;
        } else {
            // one byte is coding scheme
            textLen -= 1;
        }

        try {
            codingScheme = (byte) (rawValue[valueIndex] & 0x0c);

            if (codingScheme == 0x00) { // GSM 7-bit packed
                text = GsmAlphabet.gsm7BitPackedToString(rawValue,
                        valueIndex + 1, (textLen * 8) / 7);
            } else if (codingScheme == 0x04) { // GSM 8-bit unpacked
                text = GsmAlphabet.gsm8BitUnpackedToString(rawValue,
                        valueIndex + 1, textLen);
            } else if (codingScheme == 0x08) { // UCS2
                text = new String(rawValue, valueIndex + 1, textLen, "UTF-16");
            } else {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }

            return text;
        } catch (IndexOutOfBoundsException e) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        } catch (UnsupportedEncodingException e) {
            // This should never happen.
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }
    }

    /* SPRD: Add here for BIP function @{ */
    static byte[] retrieveByteArray(ComprehensionTlvSprd ctlv, int offset) throws ResultExceptionSprd {

        byte[] ret;
        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        int length = ctlv.getLength();
        if (length != 0 && length > offset) {
            try {
                ret = new byte[length - offset];
                System.arraycopy(rawValue, valueIndex + offset, ret, 0, ret.length);
                return ret;
            } catch (IndexOutOfBoundsException e) {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        }
        return null;
    }
    /* @} */

    /* SPRD: Add DTMF function. @{ */
    static String retrieveDTMF(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        int length = ctlv.getLength();

        if (length != 0) {
            try {
                return bcdToString_Dtmf(rawValue, valueIndex, length);
            } catch (IndexOutOfBoundsException e) {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        }
        return null;
    }
    /* @} */

    /* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
    static String retrieveLanguage(ComprehensionTlvSprd ctlv) throws ResultExceptionSprd {

        byte[] rawValue = ctlv.getRawValue();
        int valueIndex = ctlv.getValueIndex();
        int length = ctlv.getLength();

        if (length != 0) {
            System.out.println("retrieveLanguage valueIndex = " + valueIndex);
            System.out.println("retrieveLanguage rawValue[valueIndex] = " + rawValue[valueIndex]);
            System.out.println("retrieveLanguage rawValue[valueIndex + 1] = " + rawValue[valueIndex + 1]);
            byte[] temp = {rawValue[valueIndex],rawValue[valueIndex + 1]};
            return new String(temp);
        }
        return null;
    }
    /* @} */


    static final int DTMF_NUM_0 = 0x0;
    static final int DTMF_NUM_1 = 0x1;
    static final int DTMF_NUM_2 = 0x2;
    static final int DTMF_NUM_3 = 0x3;
    static final int DTMF_NUM_4 = 0x4;
    static final int DTMF_NUM_5 = 0x5;
    static final int DTMF_NUM_6 = 0x6;
    static final int DTMF_NUM_7 = 0x7;
    static final int DTMF_NUM_8 = 0x8;
    static final int DTMF_NUM_9 = 0x9;
    static final int DTMF_STAR      = 0xa;
    static final int DTMF_HASH      = 0xb;
    static final int DTMF_PAUSE     = 0xc;
    static final int DTMF_WILD      = 0xd;
    static final int DTMF_EXPANSION = 0xe;
    static final int DTMF_FILLER    = 0xf;

    private static String bcdToString_Dtmf(byte[] data, int offset, int length) {

        StringBuilder ret = new StringBuilder(length * 2);

        for (int i = offset; i < offset + length; i++) {
            byte b;
            int v[] = new int[2];

            v[0] = data[i] & 0xf;
            v[1] = (data[i] >> 4) & 0xf;

            for (int j = 0; j < 2; j++) {
                switch (v[j]) {
                    case DTMF_NUM_0:
                    case DTMF_NUM_1:
                    case DTMF_NUM_2:
                    case DTMF_NUM_3:
                    case DTMF_NUM_4:
                    case DTMF_NUM_5:
                    case DTMF_NUM_6:
                    case DTMF_NUM_7:
                    case DTMF_NUM_8:
                    case DTMF_NUM_9:
                        ret.append((char) (v[j] + '0'));
                        break;
                    case DTMF_STAR:
                        ret.append('*');
                        break;
                    case DTMF_HASH:
                        ret.append('#');
                        break;
                    case DTMF_PAUSE:
                        ret.append('P');
                        break;
                    case DTMF_WILD:
                        ret.append('w');
                        break;
                    case DTMF_FILLER:
                        // do nothing
                        break;
                    case DTMF_EXPANSION:
                    default:
                        j = 2;
                        break;
                }
            }
        }

        return ret.toString();
    }
    /* @} */
}
