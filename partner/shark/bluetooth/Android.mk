ifeq ($(BOARD_HAVE_BLUETOOTH),true)
	include $(call all-subdir-makefiles)
endif
