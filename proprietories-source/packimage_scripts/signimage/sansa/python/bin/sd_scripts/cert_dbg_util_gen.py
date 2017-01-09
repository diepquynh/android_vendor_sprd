#!/usr/bin/python3
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

# This file contains the general functions that are used in both certificates

DEBUG_PRIM_TOKEN = 0x5364656E
DEBUG_SEC_TOKEN = 0x53646465
DEBUG_CERT_VERSION = 0x00000001
WORD_SIZE_IN_BYTES = 4
SOC_ID_SIZE_IN_BYTES = 32
KEY_CERT_SIZE_IN_BYTES = 588

KEY_CERT_EXIST_FLAG_BIT = 16
LCS_ID_FLAG_BIT = 8
CERT_TYPE_PRIM = 1
CERT_TYPE_SEC = 2

import configparser
import sys
from cert_util_helper import *
from cert_dbg_util_data import *

# The function GetRSAKeyParams reads the key file (in PEM format) parse it and gets the public and private RSA key data.
# Set the N buffer (in binary representation) and calculates the Np (Barrett n' value).
def GetRSAKeyParams(logFile, RSAKeyFileName, PassphraseFileName, SBU_Crypto):
    publicKey = create_string_buffer(RSA_SIGNATURE_SIZE_BYTES + NP_SIZE_IN_BYTES)
    result = SBU_Crypto.SBU_GetNAndNpFromKeyPair(str.encode(RSAKeyFileName), str.encode(PassphraseFileName), publicKey)
    if result != 0:
        print_and_log (logFile, "Error in public key | Np")
        sys.exit(1)
      
    # Create the public key object and return it in binary format
    return CertNPublicKey(publicKey)
# End of GetRSAKeyParams

# The function GetRSAPubKeyParams reads the public  key file (in PEM format) parse it and gets the public RSA key data.
# Set the N buffer (in binary representation) and calculates the Np (Barrett n' value). the function returns string of N + Np
def GetRSAPubKeyParams(logFile, RSAKeyFileName, SBU_Crypto):
    publicKey = create_string_buffer(RSA_SIGNATURE_SIZE_BYTES + NP_SIZE_IN_BYTES)
    result = SBU_Crypto.SBU_GetNAndNpFromPubKey(str.encode(RSAKeyFileName), publicKey)    
    if result != 0:
        print_and_log (logFile, "Error in public key | Np")
        sys.exit(1)
      
    # Create the public key object and return it in binary format
    return CertNPublicKey(publicKey)
# End of GetRSAKeyParams

# The function GetRSASignature calculates the RSA signature
def GetRSASignature(logFile, DataIn, PrivKeyFile, PassphraseFile, CryptoDLL_handle):
    try:
        DataInSize = len(DataIn)
        Signature = create_string_buffer(RSA_SIGNATURE_SIZE_BYTES)
        
        # Do Rsa Sign and get the signature
        # N, D and DataIn are sent to the function as int arrays     
        p1=str.encode(PrivKeyFile)
        p2=str.encode(PassphraseFile)
        p3=DataIn.encode('iso-8859-1')
        result = CryptoDLL_handle.SBU_RSA_Sign(1, p3, DataInSize, p1, p2, Signature)
        if result != 0:
            print_and_log(logFile, "\n SBU_CRYPTO_DLL.SBU_RSA_Sign returned an error !!")                
            raise NameError

    except NameError:        
        sys.exit(1)
    #return CertRSASignature(ReverseBytesinBinString(Signature))
    return CertRSASignature(Signature)
# End of GetRSASignature

# build flags word
def build_flag_word_prim (isKeyCertExist, hbkId, lcsId):
    flag = 0x00000000
    flag = flag | hbkId 
    flag = flag + (lcsId << LCS_ID_FLAG_BIT)
    
    flag = flag | (isKeyCertExist << KEY_CERT_EXIST_FLAG_BIT)

    return flag
# End of build_flag_word_prim

# build flags word for secodary debug certificate (reserved)
def build_flag_word_sec (isKeyCertExist):
    flag = 0x00000000

    return flag
# End of build_flag_word_sec

# the function builds the certificate header, header contains token, version, length
def build_certificate_header (token, certVersion, certType, isKeyCertExist, hbkId, lcsId, primCertSize):
    
    certLength = 4*WORD_SIZE_IN_BYTES # token + version + length + flags

    if isKeyCertExist == 1: # in case the cert contains key need to add the key cert size
        certLength = certLength + KEY_CERT_SIZE_IN_BYTES

    if certType == CERT_TYPE_PRIM:
        certLength = certLength + PUBKEY_SIZE_BYTES + NP_SIZE_IN_BYTES + PUBKEY_SIZE_BYTES + NP_SIZE_IN_BYTES + 1*WORD_SIZE_IN_BYTES + 1*WORD_SIZE_IN_BYTES
        flag = build_flag_word_prim(isKeyCertExist, hbkId, lcsId)
    else:
        certLength = certLength + primCertSize +  WORD_SIZE_IN_BYTES + SOC_ID_SIZE_IN_BYTES
        flag = build_flag_word_sec(isKeyCertExist)


    headerStrBin = struct.pack('<I', token) + struct.pack('<I', certVersion) + struct.pack('<I', certLength) + struct.pack('<I', flag)
    
    return byte2string(headerStrBin)
# End of build_certificate_header
