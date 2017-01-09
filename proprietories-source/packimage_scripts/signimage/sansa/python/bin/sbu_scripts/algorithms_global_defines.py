####################################################################
#****************************************************************
#*  Copyright 2014 (c) Discretix Technologies Ltd.              *
#*  This software is protected by copyright, international      *
#*  treaties and various patents. Any copy, reproduction or     *
#*  otherwise use of this software must be authorized in a      *
#*  license agreement and include this Copyright Notice and any *
#*  other notices specified in the license agreement.           *
#*  Any redistribution in binary form must be authorized in the *
#*  license agreement and include this Copyright Notice and     *
#*  any other notices specified in the license agreement and/or *
#*  in materials provided with the binary distribution.         *
#****************************************************************


# Filename - algorithmsglobaldefines.py
# Description - This file contains the algorithms (RSA & HASH)
#               global defines
####################################################################

# Definition for HASH algorithm
#==============================
# HASH definitions according to follwoing table :
# 0x01 = SHA256
# 0x02 = SHA256 truncated to 128 bits
HASH_ALGORITHM_SUPPORT_SHA256 = 0x01
HASH_ALGORITHM_SUPPORT_SHA256_TRUNC = 0x02

# HASH algorithms sizes
HASH_ALGORITHM_SHA256_SIZE_IN_WORDS = 8
HASH_ALGORITHM_SHA256_128TRUNC_SIZE_IN_WORDS = HASH_ALGORITHM_SHA256_SIZE_IN_WORDS/2


# Definition for RSA algorithm
#============================
# RSA defitions accroding to following table :
# 0x01 = RSA-PSS-2048 with SHA 256
RSA_ALGORITHM_SUPPORT_RSA_PSS_2048_SHA256 = 0x01
RSA_ALGORITHM_SUPPORT_RSA_PKCS15_2048_SHA256 = 0x02


