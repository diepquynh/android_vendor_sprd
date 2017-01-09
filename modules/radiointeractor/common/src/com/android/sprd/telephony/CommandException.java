
package com.android.sprd.telephony;

import com.android.internal.telephony.RILConstants;

import android.telephony.Rlog;

/**
 * {@hide}
 */
public class CommandException extends RuntimeException {
    private Error mError;

    public enum Error {
        RADIO_NOT_AVAILABLE,
        INVALID_RESPONSE,
        GENERIC_FAILURE,
        /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
        MISSING_RESOURCE,
        NO_SUCH_ELEMENT,
        PASSWORD_INCORRECT,
        /* @} */
    }

    public CommandException(Error e) {
        super(e.toString());
        mError = e;
    }

    public static CommandException
    fromRilErrno(int ril_errno) {
        switch(ril_errno) {
            case RILConstants.RADIO_NOT_AVAILABLE:
                return new CommandException(Error.RADIO_NOT_AVAILABLE);
            case RILConstants.GENERIC_FAILURE:
                return new CommandException(Error.GENERIC_FAILURE);
            /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
            case RIConstants.MISSING_RESOURCE:
                return new CommandException(Error.MISSING_RESOURCE);
            case RIConstants.NO_SUCH_ELEMENT:
                return new CommandException(Error.NO_SUCH_ELEMENT);
            /* @} */
            case RILConstants.PASSWORD_INCORRECT:
                return new CommandException(Error.PASSWORD_INCORRECT);
            default:
                Rlog.e("GSM", "Unrecognized RILIL errno " + ril_errno);
                return new CommandException(Error.INVALID_RESPONSE);
        }
    }

    public Error getCommandError() {
        return mError;
    }



}
