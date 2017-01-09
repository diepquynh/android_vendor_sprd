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

from datetime import datetime
#
import sys
import struct
from ctypes import *

BYTES_WITHIN_WORD = 4

DX_MNG_DEVICE_MANUFACTURE_LCS = 0x1
DX_MNG_SECURE_LCS = 0x5

#so name
SBU_CRYPTO_LIB_DIR = "lib"
SBU_CRYPTO_LIB_Name = SBU_CRYPTO_LIB_DIR + "/" + "libsbu_crypto.so"
SBU_OSSL_CRYPTO_LIB_Name = "libcrypto.so.1.0.0"
SBU_OSSL_LIB_Name = "libssl.so.1.0.0"


CURRENT_PATH = sys.path[0]


# The function returns the path of the DLL - fixed path (relative to script path)
def GetDLLPath(FileName):

    path = str()
    path = CURRENT_PATH
    # split according to dir names
    if sys.platform != "win32" :
        path_div = "/"
    else : #platform = win32
        path_div = "\\"

    path_new = path + path_div + ".." + path_div

    path_new = path_new + FileName        
    return path_new
# End of GetDLLPath

#
# The function loads the crypto DLL and returns its handle
def LoadDLLGetHandle():
    # Load the crypto libraries
    SBU_Crypto = cdll.LoadLibrary(GetDLLPath(SBU_CRYPTO_LIB_Name))
    return SBU_Crypto
# End of LoadDLLGetHandle

# The function free the DLL - // not being used in Linux
def FreeDLLGetHandle(DllHandle):
    # free the libraries
    cdll.FreeLibrary(DllHandle)
    return 0
# End of LoadDLLGetHandle


# Create a log file handle
def create_log_file (log_file_path):

    log_file = open(log_file_path, 'w')
    return log_file;

# Print (stdout) and output also to log file given text
def print_and_log (log_file, text):
    #print (text)
    log_file.write(text)
    sys.stdout.flush()
    log_file.flush()


# Do synchronous print to appear immediately on the console
def print_sync (text):
    print (text)
    sys.stdout.flush()


# Do synchronous write to log file
def log_sync (log_file, text):
    log_file.write(text)
    log_file.flush()

# Convert bytes to string
def byte2string (DataBinStr):
    if type(DataBinStr).__name__ == 'str' :
        return DataBinStr
    ResStr = str()
    for i in range(len(DataBinStr)) :
        ResStr = ResStr + chr(DataBinStr[i])
    return ResStr

# The ReverseBytesinBinString function takes a binary string and reverse it
def ReverseBytesinBinString(binStr):
    return binStr[::-1]
# End of ReverseBytesinBinString


# The CreateCertBinFile opens a binary and text file and writes the certificate data into it
def CreateCertBinFile(logFile, binStr, certFileName):
    try:
        # Open a binary file and write the data to it
        FileObj = open(certFileName, "wb")
        FileObj.write(bytes(binStr.encode('iso-8859-1')))
        FileObj.close()

    except IOError as Error7:
        (errno, strerror) = Error7.args
        print_and_log(logFile, "Error in openning file - %s" %certFileName)
        sys.exit(1)
    return
# End of CreateCertBinFile

# The GetDataFromBinFile gets the data from a binary file
def GetDataFromBinFile(logFile, certFileName):
    binStr = str()
    try:
        # Open a binary file and write the data to it
        FileObj = open(certFileName, "rb")
        binStr = FileObj.read()
        FileObj.close()

    except IOError as Error7:
        (errno, strerror) = Error7.args
        print_and_log(logFile, "Error in openning file - %s" %certFileName)
        sys.exit(1)
    return binStr
