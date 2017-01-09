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

import struct
import math
from global_defines import *
from flags_global_defines import *
from algorithms_global_defines import *
from cert_basic_utilities import *
from hash_basic_utility import *
from ctypes import *
import global_defines

####################################################################
# Filename - datastructures.py
# Description - This file contains the data structures used in the
#               SB utility
####################################################################

# This class represents the certificate header. The header contains the magic number, certificate version, certificate size and Flags
class CertHeader:
    InternalNumOfHeaderFields = 4
    flag = 0
    CertSize = 0
    MagicNum = 0
    CertVersion = 0
      
    # Constructor, creates the flag, size and initializes the magic number
    def __init__(self, log, hbkId, certType, isEncrypted, numOfComps, prjDefines):
        self.PrjDefines = prjDefines
        self.CertHeaderFlagCreate(log, hbkId, certType, isEncrypted)
        self.CertHeaderLenCreate(log, numOfComps)
        self.MagicNum = CertMagicNum
        self.CertVersion = ( (self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_VERSION_MAJOR")]) << 16) | (self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_VERSION_MINOR")])
       
    # This method creates the flag word 
    def CertHeaderFlagCreate(self, log, hbkId, certType, isEncrypted):
        flag = 0
        # build the flag value, first add the hash algorithm
        flag = hbkId
        
        # Add the RSA algorithm
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("RSA_ALGORITHM")] == RSA_ALGORITHM_SUPPORT_RSA_PSS_2048_SHA256:                
            flag = flag | 1 << RSA_ALGORITHM_RSAPSS_2048_SHA256_BIT_POS
        else:        #RSA_ALGORITHM_SUPPORT_RSA_PKCS15_2048_SHA256
            flag = flag | 1 << (RSA_ALGORITHM_RSAPSS_2048_SHA256_BIT_POS + 1) #move to the 9th bit
        
        # Add is encrypted flag
        flag = flag| (isEncrypted << CODE_ENCRYPTION_SUPPORT_BIT_POS)
        # Add the certificate type
        flag = flag | certType << CERT_TYPE_BIT_POS
        self.flag = flag
        return 

    # This method creates the certificate size
    def CertHeaderLenCreate(self, log, numOfComps):
        self.CertSize = 0
        SigOffset = 0

        # Calculate the offset to the signature (the offset will be set in words) :
        # Header size = 4 words
        # RSA N = 64 words
        # RSA Np = 5 words || 64 words for H
        # sw revocation counter ID = 1 word
        # sw version = 1 word                
        # HASH of pubkey = HASH size        
        # Set the number of components in the appropriate field

        HASHSize = HASH_ALGORITHM_SHA256_SIZE_IN_WORDS

        if numOfComps > 32769: # 2^15 + 1
            print_and_log(log, "\nIllegal number of s/w components !!")
            sys.exit()
        self.CertSize = self.CertSize | numOfComps << NUM_OF_SW_COMPS_BIT_POS 
        if numOfComps > 0: #this is a content certificate
            SigOffset = NONCE_SIZE_IN_WORDS
        else: #this is a key certificate 
            SigOffset = HASHSize
        
        # in case of key certificate this will be set to 0
        SigOffset =  SigOffset + numOfComps * (HASHSize + 2)
           
        if RSA_ALG_USE_Np == 1: # use Np
            SigOffset = SigOffset + self.InternalNumOfHeaderFields + RSA_SIGNATURE_SIZE_IN_WORDS + NP_SIZE_IN_WORDS + \
                        SW_VERSION_OBJ_SIZE_IN_WORDS 
        else: # H is used
            SigOffset = SigOffset + self.InternalNumOfHeaderFields + RSA_SIGNATURE_SIZE_IN_WORDS + RSA_H_SIZE_IN_WORDS + \
                        SW_VERSION_OBJ_SIZE_IN_WORDS

        self.CertSize = self.CertSize | SigOffset
        return

    # This method is used to return the header as binary string
    def VarsToBinString(self):
        DataBinStr = str()
        DataBinStr1 = str()
        DataBinStr2 = str()
        DataBinStr3 = str()
        
        DataBinStr = struct.pack('<I', self.MagicNum) 
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:        
            DataBinStr = ReverseBytesinBinString(DataBinStr)

        DataBinStr1 = struct.pack('<I', self.CertVersion) 
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:        
            DataBinStr1 = ReverseBytesinBinString(DataBinStr1)
        
        DataBinStr2 = struct.pack('<I', self.CertSize)
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr2 = ReverseBytesinBinString(DataBinStr2)            
        
        DataBinStr3 = struct.pack('<I', self.flag)
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr3 = ReverseBytesinBinString(DataBinStr3)
            
        DataBinStr =  DataBinStr +  DataBinStr1 + DataBinStr2 + DataBinStr3

        return byte2string(DataBinStr)

# End of class CertHeader


# This class represents SW version fields in the certificate
class CertSwVersion:
    RevocationCntrId = 0
    SwCurrentVersion = 0    
    InternalNumOfObjects = 2 
    # Costructor
    def __init__(self, prjDefines, RecCntrId=0, RecSwVersion=0):
        self.RevocationCntrId = RecCntrId
        self.SwCurrentVersion = RecSwVersion
        self.PrjDefines = prjDefines

    # len returns the size of the object
    def __len__(self):
        return WORD_SIZE_IN_BYTES*InternalNumOfObjects
    
    # This method returns a binary string
    def VarsToBinString(self):
        DataBinStr = str()
        DataBinStr1 = str()
        
        DataBinStr = struct.pack('<I', self.RevocationCntrId) 
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr = ReverseBytesinBinString(DataBinStr)

        DataBinStr1 = struct.pack('<I', self.SwCurrentVersion) 
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr1 = ReverseBytesinBinString(DataBinStr1)
        
        DataBinStr = DataBinStr + DataBinStr1
        return byte2string(DataBinStr)
# End of CertSwVersion   

# This class represents N public key data
class CertNPublicKey:

    # Constructor
    # Np can stand either for Np or for H
    def __init__(self, PubKeyBinStr):
        self.PubKey = PubKeyBinStr

    # The method __len__ returns the size of pubkey N and Np (string size in bytes)
    def __len__(self):
        return(RSA_SIGNATURE_SIZE_IN_BYTES + NP_SIZE_IN_BYTES)

    # This method returns a binary string of the N string and Np string (N is set as big endian
    # Np is set as little endian)   
    def VarsToBinString(self):
        DataBinStr = str()
        PubKey = self.PubKey

        for i in range(RSA_SIGNATURE_SIZE_IN_BYTES + NP_SIZE_IN_BYTES): 
            byte = PubKey[i]
            DataBinStr = DataBinStr + byte2string(byte)

        return DataBinStr

# End of CertNPublicKey

# This class represents N public key data
class CertNPublicKeyHData:

    # Constructor
    # Np can stand either for Np or for H
    def __init__(self, PubKeyBinStr, HStr):
        self.PubKey = PubKeyBinStr
        self.HStr = HStr

    # The method __len__ returns the size of pubkey N and H (string size in bytes)
    def __len__(self):
        return(RSA_SIGNATURE_SIZE_IN_BYTES + RSA_H_SIZE_IN_BYTES)

    # This method returns a binary string of the N string and Np string (N is set as big endian
    # Np is set as little endian)   
    def VarsToBinString(self):
        DataBinStr = str()
        DataBinStr1 = str()

        PubKey = self.PubKey
        for i in range(RSA_SIGNATURE_SIZE_IN_BYTES):            
            byte = PubKey[i]
            DataBinStr = DataBinStr +  byte2string(byte)         
            
        Hstr = self.HStr        
        for i in range(RSA_H_SIZE_IN_BYTES):  
            DataBinStr1 =  DataBinStr1 + chr(int(Hstr[i*2:i*2+2],16))
        
        
        DataBinStr = DataBinStr + DataBinStr1    
        return byte2string(DataBinStr)
# End of CertNPublicKeyHData

# This class holds the RSA signature
class CertRSASignature:

    # Constructor
    def __init__(self, SignatureBinStr):
        self.SignatureStr = SignatureBinStr

    # The method returns the signature size
    def __len__(self):
        return RSA_SIGNATURE_SIZE_IN_BYTES

    # This method returns the binary signature
    def VarsToBinString(self):
        DataBinStr = self.SignatureStr
     
        return byte2string(DataBinStr)

# End of CertRSASignature

# This class holds the secondary N HASH value
class CertPubKeyHASHData:

    # Constructor
    def __init__(self, HASHData):
        self.PubHashData = HASHData

    # The method returns the signature size
    def __len__(self):
        return len(self.PubHashData)

    # This method returns the binary signature
    
    def VarsToBinString(self):
        DataBinStr = str()        

        for i in range(SHA_256_HASH_SIZE_IN_BYTES): 
            byte = self.PubHashData[i]
            DataBinStr = DataBinStr + byte2string(byte)

        return DataBinStr

# End of CertPubKeyHASHData 


