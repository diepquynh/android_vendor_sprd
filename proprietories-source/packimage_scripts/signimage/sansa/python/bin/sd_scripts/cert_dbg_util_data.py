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

# This file contains the general classes used in the code

PUBKEY_SIZE_BYTES = 256
NP_SIZE_IN_BYTES = 20
RSA_SIGNATURE_SIZE_BYTES = 256

import configparser
import sys
from cert_util_helper import *


# This class represents N public key data
class CertNPublicKey:

    # Constructor
    # Np can stand either for Np or for H
    def __init__(self, PubKeyBinStr):
        self.PubKey = PubKeyBinStr

    # The method __len__ returns the size of pubkey N and Np (string size in bytes)
    def __len__(self):
        return(PUBKEY_SIZE_BYTES + NP_SIZE_IN_BYTES)

    # This method returns a binary string of the N string and Np string (N is set as big endian
    # Np is set as little endian)   
    def VarsToBinString(self):
        DataBinStr = str()
        PubKey = self.PubKey

        for i in range(PUBKEY_SIZE_BYTES + NP_SIZE_IN_BYTES): 
            byte = PubKey[i]
            DataBinStr = DataBinStr + byte2string(byte)

        return DataBinStr

# End of CertNPublicKey

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
        DataBinStr = str()
        sign = self.SignatureStr

        for i in range(RSA_SIGNATURE_SIZE_BYTES): 
            byte = sign[i]
            DataBinStr = DataBinStr + byte2string(byte)

        return DataBinStr

# End of CertRSASignature

