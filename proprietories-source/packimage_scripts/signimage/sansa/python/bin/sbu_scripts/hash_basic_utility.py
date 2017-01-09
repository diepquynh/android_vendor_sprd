
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

import string
from global_defines import *
import hashlib

####################################################################
# Filename -    hashbasicutillity
# Description - This file contains the main functionality of the
#               secure boot utility. The utility creates a certificate
#               that is used in the secure boot process
####################################################################


########### Basic Utilities ###############################

# This function calculates HASH SHA256 on binary data and return the HASH result 
def HASH_SHA256(BinData, OutputRep):

    # Calculate SHA 256 on given binary data
    HashObj = hashlib.sha256()
    HashObj.update(BinData)

    if OutputRep == HASH_BINARY_REPRESENTATION:
        HashRes = HashObj.digest()
    else:
        HashRes = HashObj.hexdigest()
    
    return HashRes
# End of HASH_SHA256


######################################## END OF FILE ########################################
