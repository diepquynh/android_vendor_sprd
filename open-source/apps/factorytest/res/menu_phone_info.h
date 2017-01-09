#ifndef MENUTITLE

#define __MAKE_MENUTITLE_ENUM__
#define MENUTITLE(num,title, func)	B_PHONE_##title,
enum {
#endif

	MENUTITLE(0,CALI_INFO, test_cali_info)
	MENUTITLE(0,VERSION, test_version_show)
	MENUTITLE(0,PHONE_INFO_TEST, test_phone_info_show)//++++++++++

#ifdef __MAKE_MENUTITLE_ENUM__
	K_MENU_INFO_CNT,
};

#undef __MAKE_MENUTITLE_ENUM__
#undef MENUTITLE
#endif
