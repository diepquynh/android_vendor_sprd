/*
 * Copyright (C) 2008 Esmertec AG.
 * Copyright (C) 2008 The Android Open Source Project
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

package com.sprd.pluggersprd.datamodel;


import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;

import org.w3c.dom.events.EventTarget;
import org.w3c.dom.smil.SMILDocument;
import org.w3c.dom.smil.SMILElement;
import org.w3c.dom.smil.SMILLayoutElement;
import org.w3c.dom.smil.SMILMediaElement;
import org.w3c.dom.smil.SMILParElement;
import org.w3c.dom.smil.SMILRegionElement;
import org.w3c.dom.smil.SMILRegionMediaElement;
import org.w3c.dom.smil.SMILRootLayoutElement;
import org.xml.sax.SAXException;

import android.text.TextUtils;
import android.util.Config;
import android.util.Log;

import com.android.dom.smil.SmilDocumentImpl;
import com.android.dom.smil.parser.SmilXmlParser;
import com.android.dom.smil.parser.SmilXmlSerializer;
import com.android.util.ContentType;
import com.android.mmslib.MmsException;
import com.android.mmslib.SqliteWrapper;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;


import java.util.Collections;

import android.drm.DrmManagerClient;

import java.util.List;


public class SmilHelper {
    private static final String TAG = "Mms/smil";
    private static final boolean DEBUG = false;
    private static final boolean LOCAL_LOGV = DEBUG ? Config.LOGD : Config.LOGV;

    public static final String ELEMENT_TAG_TEXT = "text";
    public static final String ELEMENT_TAG_IMAGE = "img";
    public static final String ELEMENT_TAG_AUDIO = "audio";
    public static final String ELEMENT_TAG_VIDEO = "video";
    public static final String ELEMENT_TAG_REF = "ref";
    public static final String ELEMENT_TAG_FILE = "file";

    private SmilHelper() {
        // Never instantiate this class.
    }

    public static SMILDocument getDocument(PduBody pb) {
        // Find SMIL part in the message.
        PduPart smilPart = findSmilPart(pb);
        SMILDocument document = null;

        // Try to load SMIL document from existing part.
        if (smilPart != null) {
            document = getSmilDocument(smilPart);
        }

        if (document == null) {
            // Create a new SMIL document.
            document = createSmilDocument(pb);
        }

        return document;
    }
/*
    public static SMILDocument getDocument(SlideshowModel model) {
        return createSmilDocument(model);
    }
*/
    /**
     * Find a SMIL part in the MM.
     *
     * @return The existing SMIL part or null if no SMIL part was found.
     */
    //Fix for Bug 360805 begin
    private static PduPart findSmilPart(PduBody body) {
        int partNum = body.getPartsNum();
        for(int i = 0; i < partNum; i++) {
            PduPart part = body.getPart(i);
            if (Arrays.equals(part.getContentType(),
                            ContentType.APP_SMIL.getBytes())) {
                return part;
            }else{
				Log.d(TAG, "findSmilPart part.getContentType() :["+new String(part.getContentType())+"]");
			}
        }
        return null;
    }
    //Fix for Bug 360805 end


    private static SMILDocument validate(SMILDocument in) {
        // TODO: add more validating facilities.
        return in;
    }

    /**
     * Parse SMIL message and retrieve SMILDocument.
     *
     * @return A SMILDocument or null if parsing failed.
     */
    private static SMILDocument getSmilDocument(PduPart smilPart) {
        try {
            byte[] data = smilPart.getData();
            if (data != null) {
                if (LOCAL_LOGV) {
                    Log.v(TAG, "Parsing SMIL document.");
                    Log.v(TAG, new String(data));
                }

                ByteArrayInputStream bais = new ByteArrayInputStream(data);
                SMILDocument document = new SmilXmlParser().parse(bais);
                return validate(document);
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to parse SMIL document.", e);
        } catch (SAXException e) {
            Log.e(TAG, "Failed to parse SMIL document.", e);
        } catch (MmsException e) {
            Log.e(TAG, "Failed to parse SMIL document.", e);
        }
        return null;
    }

    public static SMILParElement addPar(SMILDocument document) {
        SMILParElement par = (SMILParElement) document.createElement("par");
        // Set duration to eight seconds by default.
        par.setDur(5.0f); // SPRD:
        document.getBody().appendChild(par);
        return par;
    }

    public static SMILMediaElement createMediaElement(
            String tag, SMILDocument document, String src) {
        SMILMediaElement mediaElement =
                (SMILMediaElement) document.createElement(tag);
        mediaElement.setSrc(escapeXML(src));
        return mediaElement;
    }

    static public String escapeXML(String str) {
       if(str != null){
          return str.replaceAll("&","&amp;")
                  .replaceAll("<", "&lt;")
                  .replaceAll(">", "&gt;")
                  .replaceAll("\"", "&quot;")
                  .replaceAll("'", "&apos;");
       }
       return "";
    }

    private static SMILDocument createSmilDocument(PduBody pb) {
        //if (Config.LOGV) {
            Log.v(TAG, "Creating default SMIL document.");
        //}

        SMILDocument document = new SmilDocumentImpl();

        // Create root element.
        // FIXME: Should we create root element in the constructor of document?
        SMILElement smil = (SMILElement) document.createElement("smil");
        smil.setAttribute("xmlns", "http://www.w3.org/2001/SMIL20/Language");
        document.appendChild(smil);

        // Create <head> and <layout> element.
        SMILElement head = (SMILElement) document.createElement("head");
        smil.appendChild(head);

        SMILLayoutElement layout = (SMILLayoutElement) document.createElement("layout");
        head.appendChild(layout);

        // Create <body> element and add a empty <par>.
        SMILElement body = (SMILElement) document.createElement("body");
        smil.appendChild(body);
        SMILParElement par = addPar(document);

        // Create media objects for the parts in PDU.
        int partsNum = pb.getPartsNum();
        if (partsNum == 0) {
            return document;
        }
/*
        DrmManagerClient drmManagerClient = MmsApp.getApplication().getDrmManagerClient();
*/
        boolean hasText = false;
        boolean hasMedia = false;
        for (int i = 0; i < partsNum; i++) {
            // Create new <par> element.
            //if ((par == null) || (hasMedia && hasText)) {
            if (par == null || hasMedia || hasText) {
                par = addPar(document);
                hasText = false;
                hasMedia = false;
            }

            PduPart part = pb.getPart(i);
            String contentType = new String(part.getContentType());
/*
            if (ContentType.isDrmType(contentType)) {
                contentType = drmManagerClient.getOriginalMimeType(part.getDataUri());
            }
*/
            if (ContentType.isImageType(contentType)) {
                SMILMediaElement imageElement = createMediaElement(
                        ELEMENT_TAG_IMAGE, document, part.generateLocation());
                par.appendChild(imageElement);
                hasMedia = true;
            } else {
                // TODO: handle other media types.
                Log.w(TAG, "unsupport media type");
            }
        }

        return document;
    }

 

   
}
