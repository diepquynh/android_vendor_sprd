# 64-bit MIPS R6 little-endian compiler
TARGET_PRIMARY_ARCH   := target_mips64r6el
ifeq ($(MULTIARCH),1)
TARGET_SECONDARY_ARCH := target_mips32r6el
endif
