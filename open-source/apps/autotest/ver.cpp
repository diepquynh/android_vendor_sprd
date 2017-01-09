// 
// Spreadtrum Auto Tester
//
// anli   2013-01-08
//

#include "type.h"
#include "ver.h"
#include "debug.h"

#include <cutils/properties.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_ver {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// version:
// 0.0.1: first release
// 0.0.2: add version info
// 0.0.3: retry bt inquire and wifi scan
// 0.0.4: modify wifi scan
// 0.1.0: modify charging,keypad backlight,cft post mode
// 0.1.1: modify camera,speaker,sensor,keypad
// 0.1.2: modify for keyboard backlight, compatile keyboard and buttons
// 0.2.0: remove especial audio out path control, fm to libfm, ioctl code to standards
// 0.3.1: disable unused service, add gps, modify wifi scan, all to lib ...
//------------------------------------------------------------------------------
const char * verGetVer( void )
{
    return "v0.3.1";
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char * verGetDescripter( void )
{
	static char desc[128];

    char relver[PROPERTY_VALUE_MAX];
	if( property_get("ro.build.version.release", relver, NULL) <= 0 ) {
		strncpy(relver, "2.3", 8);
	}

    snprintf(desc, 128, "AutoTest for android %s, %s %s", relver, __DATE__, __TIME__ );
    
    return desc;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
