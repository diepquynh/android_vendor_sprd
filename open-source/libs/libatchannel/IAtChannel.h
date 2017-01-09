#ifndef ANDROID_IATCHANNEL_H
#define ANDROID_IATCHANNEL_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IAtChannel: public IInterface {
public:
    DECLARE_META_INTERFACE(AtChannel);

    virtual const char* sendAt(const char* atCmd) = 0;
};

// ----------------------------------------------------------------------------

class BnAtChannel: public BnInterface<IAtChannel> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

}
; // namespace android

#endif // ANDROID_IATCHANNEL_H
