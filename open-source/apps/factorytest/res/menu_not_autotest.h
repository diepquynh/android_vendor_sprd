#ifndef MENUTITLE

#define __MAKE_MENUTITLE_ENUM__
#define MENUTITLE(num ,title, func)	D_PHONE_##title,
enum {
#endif
	MENUTITLE(CASE_TEST_TEL,TEST_TEL, test_tel_start)
	MENUTITLE(CASE_TEST_OTG,TEST_OTG, test_otg_start)

#ifdef __MAKE_MENUTITLE_ENUM__
	K_MENU_NOT_AUTO_TEST_CNT,
};

#undef __MAKE_MENUTITLE_ENUM__
#undef MENUTITLE
#endif
