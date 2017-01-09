/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.internal.telephony;
import java.util.List;
/**
 * Interface used to interact with the phone.  Mostly this is used by the
 *  Messaging app or contact for sending mms or sms
 * Please clean them up if possible and use TelephonyManager instead.
 *
 * {@hide}
 */
interface IFdnService{

    /**
     * set the init cacheMap flag
     * @return int
     * {@hide}
     */
    int process(int nCommand, long lParam,  in List<String>  szValuslist, inout byte[] bytes);
}