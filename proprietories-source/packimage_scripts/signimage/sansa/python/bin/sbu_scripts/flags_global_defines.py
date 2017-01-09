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


# Filename - flagsglobaldefines.py
# Description - This file contains the flags bit positions
####################################################################

# Definition for HASH bit positions
#==================================
# Bits 0-7 for HASH algorithm
HASH_ALGORITHM_SHA256_BIT_POS       = 0
HASH_ALGORITHM_SHA256_TRUNC_BIT_POS = 1


# Definition for RSA bit positions
#+================================
# Bits 8-15 saved for public key algorithm
RSA_ALGORITHM_RSAPSS_2048_SHA256_BIT_POS = 8

# Definition for certificate type
#================================
# Bits 17-18 saved for cert type 
CERT_TYPE_BIT_POS = 17
CERT_TYPE_KEY     = 1
CERT_TYPE_CONTENT = 2


# Definition for RSA Np or H usage
#+================================
# This flag is set to 1 for Np usage in case H should be used set it to 0
RSA_ALG_USE_Np = 1

# This definition is for the secondary HASH of public key calculation. The calculation is done according to the 
# calculation of HASH of public key (as saved in the OTP) and is set according to platform type  
OTP_HASH_CALC_ON_N_AND_NP = 0
OTP_HASH_CALC_ON_N = 1
OTP_HASH_ON_E_AND_N = 2 

# This definition should be set according to platform and project 
SECONDARY_KEY_HASH_CALC = OTP_HASH_CALC_ON_N_AND_NP

# Definitions for specific projects, should be set to 0 if no additional data is required
SPECIAL_ADDITIONAL_DATA_USED = 1

# Definitions for Code encryption support
#========================================
CODE_ENCRYPTION_SUPPORT_BIT_POS       = 16

# Bits 19 - 31 are reserved 
####################################################################

# Bit positions of signature offset and number of s/w components
#================================================================
NUM_OF_SW_COMPS_BIT_POS = 16

SIGNATURE_OFFSET_BIT_POS = 0


