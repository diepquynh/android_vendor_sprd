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


# Filename - globaldefinesrsaformat.py
# Description - This file contains global defines used in the RSA 
#               Format parser
####################################################################

# Parameter type
PARAM_MOD = 1
PARAM_PRIV_EXP = 2
PARAM_EXP = 3

# PEM header and footer 
PEM_START = "-----BEGIN RSA PRIVATE KEY-----\n"
PEM_END = "\n-----END RSA PRIVATE KEY-----\n"

# PEM header size
PEM_HEADER_SIZE_BYTES = 4

# PEM version size
PEM_VERSION_SIZE_BYTES = 3

# Parameters ASN.1 DER type
PARAM_HEADER_INTEGER_TYPE = 2

# Length ASN.1 DER
PARAM_LENGTH_INDICATION_BIT = 7
PARAM_LENGTH_INDICATION = 0x1 << PARAM_LENGTH_INDICATION_BIT

PARAM_LENGTH_BITS_MASK = 0x7F

# Size of expected Mod & Priv exponent
RSA_MOD_SIZE_BYTES = 256

# Modulus & Priv Exponent ASN.1 header size
MOD_HEADER_FIXED_SIZE_BYTES = 4

# Exponent ASN.1 header size
EXP_HEADER_FIXED_SIZE_BYTES = 2

# Exponent expected value
EXP_EXPECTED_VAL = 65537

# AES key fixed size
AES_KEY_SIZE_IN_BYTES = 32
