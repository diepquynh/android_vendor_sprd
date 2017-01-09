// 
// Spreadtrum Auto Tester
//
// anli   2012-12-10
//

#include <binder/IPermissionController.h>
#include <binder/IServiceManager.h>
#include <string.h>

#include "type.h"
#include "perm.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
namespace at_perm {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace android;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static String16 _permission("permission");

class PermController : public BnPermissionController
{
public: 
    virtual bool checkPermission(const String16& permission, int32_t pid, int32_t uid) {
        DBGMSG("checkPermission: pid = %d, uid = %d\n", pid, uid);
        return true;
    }    
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int permInstallService( const char * permission )
{
    FUN_ENTER;    
    
    sp<IServiceManager> sm   = defaultServiceManager();
    sp<IBinder>         perm = sm->checkService(_permission);
    if( perm.get() == NULL ) {
        DBGMSG("!! permission service is null !!\n");
        sm->addService(_permission, new PermController);
    }

    FUN_EXIT;
    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
