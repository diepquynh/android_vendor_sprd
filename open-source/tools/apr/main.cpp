
#include "common.h"
#include "confile.h"
#include "AprData.h"
#include "XmlStorage.h"
#include "InotifyThread.h"
#include "Anr.h"
#include "NativeCrash.h"
#include "ModemThread.h"
#include "JavaCrashThread.h"

int main(int argc, char *argv[])
{
	int retval;
	int swAnr = 1;
	int swNativeCrash = 1;
	int swJavaCrash = 0;
	int swModemAssert = 1;
	int swCrashClass = 1;

	self_inspection_apr_enabled();

	INI_CONFIG* pIniCfg;
	pIniCfg = ini_config_create_from_file("/data/apr.conf", 0);

	if (pIniCfg) {
		swAnr = ini_config_get_bool(pIniCfg, "exceptions", "anr", 1);
		swNativeCrash = ini_config_get_bool(pIniCfg, "exceptions", "nativeCrash", 1);
		swJavaCrash = ini_config_get_bool(pIniCfg, "exceptions", "javaCrash", 0);
		swModemAssert = ini_config_get_bool(pIniCfg, "exceptions", "modemAssert", 1);
		swCrashClass = ini_config_get_bool(pIniCfg, "exceptions", "crashClass", 1);

		ini_config_destroy(pIniCfg);
	}

	AprData aprData;
	Anr anr(&aprData);
	NativeCrash nativeCrash(&aprData);
	InotifyThread anrThread(&anr);
	InotifyThread nativeCrashThread(&nativeCrash);
	ModemThread modemThread(&aprData);
	JavaCrashThread javaCrashThread(&aprData);

	/* switch CrashClass */
	if (swCrashClass) {
		aprData.setBootModeSW(true);
	}

	XmlStorage xmlStorage(&aprData, (char*)"/data/sprdinfo", (char*)"apr.xml");
	aprData.addObserver(&xmlStorage);
	/* switch anr */
	if (swAnr) {
		anrThread.StartWithPipe(NULL);
	}
	/* switch nativeCrash */
	if (swNativeCrash) {
		nativeCrashThread.StartWithPipe(NULL);
	}
	/* switch modemAssert */
	if (swModemAssert) {
		modemThread.StartWithPipe(NULL);
	}
	/* switch javaCrash */
	if (swJavaCrash) {
		javaCrashThread.Start(NULL);
	}

	while(1)
	{
		// waiting 60 seconds.
		sleep(60);
		/* update the <endTime></endTime> for all Observers */
		aprData.setChanged();
		aprData.notifyObservers(NULL);
	}

	aprData.deleteObserver(&xmlStorage);
	return 0;
}

