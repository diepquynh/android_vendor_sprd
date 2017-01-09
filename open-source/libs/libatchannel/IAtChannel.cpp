#define LOG_TAG "IAtChannel"

#include <IAtChannel.h>
#include <binder/Parcel.h>
#include <cutils/jstring.h>

namespace android {

enum {
	TRANSACTION_sendAt = IBinder::FIRST_CALL_TRANSACTION + 0
};

static char *
strdupReadString(Parcel &p) {
    size_t stringlen;
    const char16_t *s16;

    s16 = p.readString16Inplace(&stringlen);

    return strndup16to8(s16, stringlen);
}

class BpAtChannel: public BpInterface<IAtChannel>
{
public:
    BpAtChannel(const sp<IBinder>& impl)
        : BpInterface<IAtChannel>(impl)
    {
    }

    const char* sendAt(const char* atCmd)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAtChannel::getInterfaceDescriptor());
        data.writeString16(String16(atCmd));
        if (remote()->transact(TRANSACTION_sendAt, data, &reply) != NO_ERROR) {
            ALOGD("sendAt could not contact remote\n");
            return "ERROR";
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("sendAt caught exception %d\n", err);
            return "ERROR";
        }
        return strdupReadString(reply);
    }
};

IMPLEMENT_META_INTERFACE(AtChannel, "com.sprd.internal.telephony.IAtChannel");

// ----------------------------------------------------------------------

};
