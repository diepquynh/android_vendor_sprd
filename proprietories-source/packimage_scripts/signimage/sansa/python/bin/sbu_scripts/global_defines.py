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


# Filename - globaldefines.py
# Description - This file contains global defines used in the secure
#               boot utility
####################################################################

# DX default magic number for the certificate table
CertMagicNum = 0xE291F358

#E value, in a string mode. for the HASh calculation
E_VALUE_REVERSED = "00010001"
# Memory unload flag
MEM_ADDRESS_UNLOAD_FLAG = 0xFFFFFFFFFFFFFFFF

# RSA Modulus size = RSA signature size (RSA 2048 key size is used)
RSA_PRIVATE_KEY_SIZE = 2048

RSA_SIGNATURE_SIZE_IN_BYTES = 256
RSA_SIGNATURE_SIZE_IN_DOUBLE_BYTES = 512
RSA_SIGNATURE_SIZE_IN_WORDS = 64
# HASH size in SHA256 in bytes
SHA_256_HASH_SIZE_IN_BYTES = 32
# NP size in bytes
NP_SIZE_IN_BYTES = 20
NP_SIZE_IN_WORDS = 5
# H size
RSA_H_SIZE_IN_BYTES = RSA_SIGNATURE_SIZE_IN_BYTES
RSA_H_SIZE_IN_WORDS = RSA_SIGNATURE_SIZE_IN_BYTES/4
 
# Size of word in bytes
WORD_SIZE_IN_BYTES = 4
# Size of SW versions 
SW_VERSION_OBJ_SIZE_IN_WORDS = 2

# header size in bytes
HEADER_SIZE_IN_BYTES = 4 * WORD_SIZE_IN_BYTES
#size number of bytes in address
NUM_OF_BYTES_IN_ADDRESS = 8

# Public key file name
GenRSA_FileName = "key.pem"
GenPassPhraseFileName = ""
# Private key file name
#GenD_FileName = "private_d.key"

# HASH output representation
HASH_BINARY_REPRESENTATION = 1
HASH_HEX_REPRESENTATION    = 2 

# SNP size (used to calculate Np) 2048 + 132
SNp = RSA_SIGNATURE_SIZE_IN_BYTES*8 + 132

# certificate output file prefix
Cert_FileName = "Cert"
# certificate output file suffix
Cert_FileExtBin = ".bin"
Cert_FileExtTxt = ".txt"

#so name
SBU_CRYPTO_LIB_DIR = "lib"
SBU_CRYPTO_LIB_Name = SBU_CRYPTO_LIB_DIR + "/" + "libsbu_crypto.so"
SBU_OSSL_CRYPTO_LIB_Name = "libcrypto.so.1.0.0"
SBU_OSSL_LIB_Name = "libssl.so.1.0.0"

        
# definitions for code encryption
AES_IV_SIZE_IN_BYTES = 16
AES_DECRYPT_KEY_SIZE_IN_BYTES = 16
SW_COMP_FILE_NAME_POSTFIX = "_enc.bin"

NONCE_SIZE_IN_WORDS = 2

# Definitions for additional data
ADDITIONAL_DATA_SIZE = 128

# Defines for project endianity
################################
CERT_IN_LITTLE_ENDIAN = 0
CERT_IN_BIG_ENDIAN = 1

# Definition for debug mode
###########################
DEBUG_MODE_OFF = 0
DEBUG_MODE_ON = 1
DEBUG_MODE = DEBUG_MODE_OFF

# Definitions for sw version legal values
#########################################
SW_REVOCATION_MAX_NUM_OF_BITS_CNTR1 = 31
SW_REVOCATION_MAX_NUM_OF_BITS_CNTR2 = 223

# Definitions for list of configurables parameters
##################################################
LIST_OF_CONF_PARAMS = ["CERT_ENDIANITY","RSA_ALGORITHM","CERT_VERSION_MAJOR","CERT_VERSION_MINOR","NUM_OF_REVOCATION_COUNTERS_SUPPORT"]
