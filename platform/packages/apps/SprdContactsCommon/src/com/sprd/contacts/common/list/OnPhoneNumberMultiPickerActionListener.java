
package com.sprd.contacts.common.list;

import java.util.HashMap;

/**
 * Action callbacks that can be sent by a phone number picker.
 */
public interface OnPhoneNumberMultiPickerActionListener  {

    /**
     * Returns the selected phone number to the requester.
     */
    void onPickPhoneNumberAction(HashMap<String,String> pairs);
    void onCancel();

}
