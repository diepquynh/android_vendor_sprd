package com.android.messaging.smilplayer.model;

import android.util.Log;

//import com.android.mms.ui.MessageUtils;

import com.google.android.mms.ContentType;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;
//import com.google.android.mms.pdu.PduPersisterSprd;
import com.google.android.mms.pdu.PduPersister;

public class SmilHelperSprd {
    protected static final String EXTENDS_TAG_VCARD = "vcf";
    protected static final String ELEMENT_TAG_VCARD = "text";
    protected static final String PAR_TAG = "par";
    protected static final String ELEMENT_TAG_VCALENDAR = "text";
    protected static final String ELEMENT_TAG_VCALENDAR_FILE = "ics";   //coolCalendar.ics for bug182791
    protected static final String ELEMENT_TAG_VCARD_FILE  = "vcard";
    protected static final String EXTENDS_TAG_OGG = "ogg";


    private static boolean isOsDebug(){
          return true;
    }

    /**
     * SPRD: @{
     */
     public static boolean containsTag(PduPart part, String tag){
         if(part == null){
             return false;
         }

         if (checkVcf(part.getName(), tag)) {
             return true;
         }

         if (checkVcf(part.getFilename(), tag)) {
             return true;
         }

         if (checkVcf(part.getContentLocation(), tag)) {
             return true;
         }

         if (checkVcf(part.getContentId(), tag)) {
             return true;
         }
         return false;
     }
     private static boolean checkVcf(byte[] partField, String tag){
         if (partField == null) {
             return false;
         }
         String value = PduPersister.toIsoString(partField).toLowerCase();
         return value != null && value.length() > 0 && value.contains(".")
         && value.substring(value.lastIndexOf('.') + 1, value.length()).equals(tag);
     }
     // @}

     public static boolean isNeedAdapteVcard(PduBody body) {
         int partNum = body.getPartsNum();
         if (partNum <= 2) {
             for (int i = 0; i < partNum; i++) {
                 PduPart part = (PduPart) body.getPart(i);
                 String contenttype = PduPersister.toIsoString(part.getContentType());
                 if (containsTag(part, EXTENDS_TAG_VCARD) || isVcard(part)) {
                     return true;
                 }
             }
         }
         return false;
     }

     public static boolean isWrongVcardSmilPart(PduPart part) {
         if(part != null && part.getData() != null) {
             String smil = new String(part.getData());
             if (isOsDebug()) {
                 Log.d("SmilHelperSprd", "parse smil string = " + smil);
             }
             if (!smil.contains(PAR_TAG)) {
                 return true;
             }
             if (!smil.contains(ELEMENT_TAG_VCARD) && !smil.contains(EXTENDS_TAG_VCARD)) {
                 return true;
             }
         }
         return false;
     }
     /*
      * SPRD:Bug#271098,Sometimes vcard's content-type is 'application/oct-stream'. @{
      */
      public static boolean isVcard(PduPart part) {
          String filename = null;
          final String type = new String(part.getContentType());
          byte[] cl = part.getContentLocation();
          byte[] name = part.getName();
          byte[] ci = part.getContentId();
          byte[] fn = part.getFilename();
          
          if (cl != null) {
              filename = new String(cl);
              return (ContentType.TEXT_VCARD.equalsIgnoreCase(type) ||
                      ((type.equals("application/oct-stream") || type.equals("application/octet-stream")) &&
                      filename.endsWith(".vcf")));
          } else if (name != null){
              filename = new String(name);
              return (ContentType.TEXT_VCARD.equalsIgnoreCase(type) ||
                      ((type.equals("application/oct-stream") || type.equals("application/octet-stream")) &&
                      filename.endsWith(".vcf")));
          } else if (ci != null){
              filename = new String(ci);
              return (ContentType.TEXT_VCARD.equalsIgnoreCase(type) ||
                      ((type.equals("application/oct-stream") || type.equals("application/octet-stream")) &&
                      filename.endsWith(".vcf")));
          } else if (fn != null){
              filename = new String(fn);
              return (ContentType.TEXT_VCARD.equalsIgnoreCase(type) ||
                      ((type.equals("application/oct-stream") || type.equals("application/octet-stream")) &&
                      filename.endsWith(".vcf")));
          } else {
              return false;
          }  
      }
      /*
      * SPRD:Bug#270871,Sometimes vcalendar's content-type is 'application/oct-stream'. @{
      */
      public static boolean isVCalendar(PduPart part) {
          if (part != null) {
              String filename = null;
              final String type = new String(part.getContentType());
              byte[] cl = part.getContentLocation();
              byte[] name = part.getName();
              byte[] ci = part.getContentId();
              byte[] fn = part.getFilename();

              if (cl != null) {
                  filename = new String(cl);
                  return (ContentType.TEXT_VCALENDAR.equalsIgnoreCase(type) || ((type
                          .equals("application/oct-stream") || type
                          .equals("application/octet-stream") || type
                          .equals("application/x-octet-stream")) && filename
                          .endsWith(".vcs")));
              } else if (name != null) {
                  filename = new String(name);
                  return (ContentType.TEXT_VCALENDAR.equalsIgnoreCase(type) || ((type
                          .equals("application/oct-stream") || type
                          .equals("application/octet-stream") || type
                          .equals("application/x-octet-stream")) && filename
                          .endsWith(".vcs")));
              } else if (ci != null) {
                  filename = new String(ci);
                  return (ContentType.TEXT_VCALENDAR.equalsIgnoreCase(type) || ((type
                          .equals("application/oct-stream") || type
                          .equals("application/octet-stream")|| type
                          .equals("application/x-octet-stream")) && filename
                          .endsWith(".vcs")));
              } else if (fn != null) {
                  filename = new String(fn);
                  return (ContentType.TEXT_VCALENDAR.equalsIgnoreCase(type) || ((type
                          .equals("application/oct-stream") || type
                          .equals("application/octet-stream") || type
                          .equals("application/x-octet-stream")) && filename
                          .endsWith(".vcs")));
              } else {
                  return false;
              }
          }
          return false;
      }
      /* @} */
}
