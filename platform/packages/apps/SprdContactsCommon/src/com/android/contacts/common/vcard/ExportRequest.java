/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.contacts.common.vcard;

import android.net.Uri;
/**
* SPRD:
* @{
*/
import java.util.ArrayList;
/**
* @}
*/
public class ExportRequest {
    public final Uri destUri;
    /**
     * Can be null.
     */
    public final String exportType;

    public ExportRequest(Uri destUri) {
        /**
        * SPRD:Bug474746 Add Import/Export features for contacts.
        * Original Android code:
        this(destUri, null);
        *
        * @{
        */
        this(destUri, null, null);
        /**
        * @}
        */
    }

    /**
    * SPRD:Bug474746 Add Import/Export features for contacts.
    * Original Android code:
    public ExportRequest(Uri destUri, String exportType) {
        this.destUri = destUri;
        this.exportType = exportType;
    }
    *
    * @{
    */
    public ExportRequest(Uri destUri, String exportType) {
        this(destUri, exportType, null);
        }
    /**
    * @}
    */

    /**
    * SPRD:474746 Add Import/Export features for contacts.
    *
    * @{
    */
    public final ArrayList<String> selectedContacts;

    public ExportRequest(Uri destUri,ArrayList<String> selectedContacts) {
        this(destUri, null,selectedContacts);
     }

    public ExportRequest(Uri destUri, String exportType, ArrayList<String> selectedContacts) {
        this.destUri = destUri;
        this.exportType = exportType;
        this.selectedContacts=selectedContacts;
        }
    /**
    * @}
    */
}
