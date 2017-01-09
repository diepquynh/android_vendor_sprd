#ifndef MENUTITLE


#define __MAKE_MENUTITLE_ENUM__
#define MENUTITLE(num,title, func)	K_ROOT_##title,
enum {
#endif

	MENUTITLE(0,BOARD_AUTOTEST, PCBA_auto_all_test)
	MENUTITLE(0,PHONE_AUTOTEST, auto_all_test)
	MENUTITLE(0,BOARD_SINGLETEST, show_pcba_test_menu)
	MENUTITLE(0,PHONE_SINGLETEST, show_phone_test_menu)
	MENUTITLE(0,NOT_AUTO_TEST, show_suggestion_test_menu)
	MENUTITLE(0,PHONE_INFO, show_phone_info_menu)
	MENUTITLE(0,BOARD_REPORT, show_pcba_test_result)
	MENUTITLE(0,PHONE_REPORT, show_phone_test_result)
	MENUTITLE(0,NOT_AUTO_REPORT, show_suggestion_test_result)
	MENUTITLE(0,PHONE_REBOOT, phone_reboot)
#ifdef __MAKE_MENUTITLE_ENUM__

	K_MENU_ROOT_CNT,
};

#undef __MAKE_MENUTITLE_ENUM__
#undef MENUTITLE
#endif
