package android.telephony;

/**
 * Various utilities for dealing with phone number strings.
 */
public class PhoneNumberUtilsEx
{
    /*
     * Special characters
     *
     * (See "What is a phone number?" doc)
     * 'p' --- GSM pause character, same as comma
     * 'w' --- GSM wait character
     */
    public static final char PAUSE = ',';
    public static final char WAIT = ';';

    /**
     * SPRD: Porting send P/W to CP
     * Strips separators from a phone number string.Except p w
     * @param phoneNumber phone number to strip.
     * @return phone string stripped of separators.
     * @hide
     */
     public static String stripSeparatorsExceptPW(String phoneNumber) {
         if (phoneNumber == null) {
             return null;
         }
         int len = phoneNumber.length();
         StringBuilder ret = new StringBuilder(len);
         for (int i = 0; i < len; i++) {
             char c = phoneNumber.charAt(i);
             // Character.digit() supports ASCII and Unicode digits (fullwidth,Arabic-Indic, etc.)
             int digit = Character.digit(c, 10);
             if (digit != -1) {
                 ret.append(digit);
             }else if (PhoneNumberUtils.isNonSeparator(c)) {
                 ret.append(c);
             }else if (c == 'P' || c == 'p' || c == 'W' || c == 'w') {
                 ret.append(c);
             }
         }
         return ret.toString();
     }

     /**
      * SPRD: exchange P & W to , & ;
      * @hide
      */
     public final static String pAndwToCommaAndSemicolon(String str) {
         if (null != str) {
             StringBuilder strBlder = new StringBuilder();
             int len = str.length();
             for (int i = 0; i < len; i++) {
                 switch (str.charAt(i)) {
                 case 'p':
                 case 'P':
                     strBlder.append(PAUSE);
                     break;
                 case 'w':
                 case 'W':
                     strBlder.append(WAIT);
                     break;
                 default:
                     strBlder.append(str.charAt(i));
                 }
             }
             return strBlder.toString();
         } else {
             return null;
         }
     }

     /**
      * SPRD:exchange , & ; to P & W
      * @hide
      */
     public final static String CommaAndSemicolonTopAndw(String str) {
         if (null != str) {
             StringBuilder strBlder = new StringBuilder();
             int len = str.length();
             for (int i = 0; i < len; i++) {
                 switch (str.charAt(i)) {
                 case PAUSE:
                     strBlder.append('P');
                     break;
                 case WAIT:
                     strBlder.append('W');
                     break;
                 default:
                     strBlder.append(str.charAt(i));
                 }
             }
             return strBlder.toString();
         } else {
             return null;
         }
     }
}
