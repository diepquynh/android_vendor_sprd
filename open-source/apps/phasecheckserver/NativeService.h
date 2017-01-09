#ifndef ANDROID_ZPSERVICE_H   
#define ANDROID_ZPSERVICE_H   
  
#include <utils/RefBase.h>   
#include <binder/IInterface.h>   
#include <binder/Parcel.h>   
  
namespace android  
{  
    class NativeService : public BBinder  
    {  
    private:  
        //mutable Mutex m_Lock;   
        //int32_t m_NextConnId;   
  
    public:  
        static int Instance();  
        NativeService();  
        virtual ~NativeService();  
        virtual status_t onTransact(uint32_t, const Parcel&, Parcel*, uint32_t);  
    };  
}  
  
#endif  
