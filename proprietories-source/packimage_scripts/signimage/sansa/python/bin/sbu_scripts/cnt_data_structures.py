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
from hash_basic_utility import *
from ctypes import *
from cert_basic_utilities import *
import global_defines

####################################################################
# Filename - datastructures.py
# Description - This file contains the data structures used in the
#               SB utility
####################################################################


# The ReverseBytesInString function reverse the bytes in a string and returns the reversed string
def tempReverseBytesInBytesArray(array, size):   

    reversedArray = c_ubyte * size
    revArray = reversedArray()
    for i in range(size//4):
        # reverse each word
        for j in range(4):
            revArray[4*i + j] = array[4*i + 3 - j]
    return revArray
# End of ReverseBytesInString

   
# This class represent a record info structure. It includes the SW Comp address,
# the Size of the record, the place holder and the HASH result
class CertRecordInfo:
    FlashAddr = 0xabc
    MemLoadAddr = 0x100
    MemLoadAddrList = list()
    LenInWords = int(0)
    SwHASHResult = ""
    InternalNumOfIntFields = 3

    # Constructor
    def __init__(self, RecFlashAddr, RecMemAddr, RecMemAddrList, RecLen, RecHash, prjDefines):        
        self.FlashAddr = RecFlashAddr
        self.MemLoadAddr = RecMemAddr
        self.MemLoadAddrList = RecMemAddrList
        self.LenInWords = RecLen//WORD_SIZE_IN_BYTES        
        self.SwHASHResult = RecHash        
        self.PrjDefines = prjDefines
        
    # The method __len__ returns the size of the parameters in the class in bytes
    def __len__(self):
        return len(self.SwHASHResult) + WORD_SIZE_IN_BYTES*self.InternalNumOfIntFields

    # The method VarsToBinStringHashComp creates a binary string out of the hash of component
    # and the memory load address
    def VarsToBinStringHashComp(self):
        DataBinStr = str()
        DataBinStr1 = str()
        
        DataBinStr = self.SwHASHResult 
            
        DataBinStr1 = struct.pack('<Q', self.MemLoadAddr) 
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr1 = ReverseBytesinBinString(DataBinStr1)
        
        DataBinStr = DataBinStr + DataBinStr1
        return byte2string(DataBinStr)
    
    # The method VarsToBinStringParamComp creates a binary string out of the component size
    # and the storage address
    def VarsToBinStringParamComp(self):
        DataBinStr = str()
        DataBinStr1 = str()
        
        DataBinStr = struct.pack('<Q', self.FlashAddr)
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr = ReverseBytesinBinString(DataBinStr)
            
        DataBinStr1 = struct.pack('<I', self.LenInWords)        
        # In case of big endian reverse the strings        
        if self.PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_BIG_ENDIAN:
            DataBinStr1 = ReverseBytesinBinString(DataBinStr1)
             
        DataBinStr = DataBinStr + DataBinStr1             
        return byte2string(DataBinStr)
    
    # Return the load address
    def GetLoadAddress(self):
        return self.MemLoadAddr
        
# End of CertRecordInfo


# This class holds the AES encryption needed information
class CodeEncryptionData:
    
    # Constructor
    def __init__(self, keyFileName, loadAddressList, nonce, prjDefines):
        self.PrjDefines = prjDefines
        self.keyIntArray = self.extractAESKeyFromFile(keyFileName)
        self.nonceStrBin = str()
        self.IVIntArray = self.combineAESIV(loadAddressList, nonce)
        
    # This method extracts the key from the file    
    def extractAESKeyFromFile(self, keyFileName):
        try:            
            # Get the key data from the file. The key data format is text (0x01, 0x02 ,..etc)
            fob = open(keyFileName, "r")
            fileData = fob.read()
            fob.close()        
        
            IntArrayKeyParam = c_ubyte * AES_DECRYPT_KEY_SIZE_IN_BYTES
            keyIntArray = IntArrayKeyParam()
        
            # Take each of the hex representation and save it as Int in the array
            keyList = fileData.split(",")
            i = 0
            for obj in keyList:
                if i == AES_DECRYPT_KEY_SIZE_IN_BYTES:
                    print("aes key file is in illegal size")
                    break
                keyIntArray[i] = int(obj, 16)
                i = i + 1
            
        except IOError as Error1:
            (errno, strerror) = Error1.args
            print("Error in opening file - %s" %FileName)
            sys.exit()
        
        return keyIntArray    
            
    # This method combines the IV
    def combineAESIV(self, loadAddressList, nonce):
        try:                
            self.nonceStrBin = nonce             
            IntArrayIVParam = c_ubyte * AES_IV_SIZE_IN_BYTES
            IVIntArray = IntArrayIVParam()
            fillZeroes = 0
                        
            i = 0
            for char in self.nonceStrBin:                  
                IVIntArray[i] = struct.unpack("B",char)[0]                                
                i = i + 1
                
            # The IV is composed of - nonce (8 bytes) + load address (8 bytes) 
            # first need to verify that the list size is as expected
            if len(loadAddressList) < NUM_OF_BYTES_IN_ADDRESS: # need to fill zeroes before
                    fillZeroes = NUM_OF_BYTES_IN_ADDRESS - len(loadAddressList)

            for j in range(fillZeroes):
                IVIntArray[i] = int("0")                
                i = i + 1
                
            # copy each byte as int to the array
            for j in range(NUM_OF_BYTES_IN_ADDRESS-fillZeroes):
                IVIntArray[i] = int(loadAddressList[j], 16)                
                i = i + 1

            
            # return the IV
        except NameError:
            print("\n combineAESIV failed !! ")
            sys.exit()   
        return IVIntArray             
    
    # This method is responsible to write the encrypted data
    def AESEncryptDataAndHash(self, inputFileName, SBU_Crypto):
        try:
            newFileName = inputFileName[:-4] + SW_COMP_FILE_NAME_POSTFIX            
            image_size = c_uint()
            OutputDataIntArray = create_string_buffer(SHA_256_HASH_SIZE_IN_BYTES)
            
            result = SBU_Crypto.SBU_AES_CTR_EncryptFile(str.encode(inputFileName), str.encode(newFileName), self.keyIntArray, AES_DECRYPT_KEY_SIZE_IN_BYTES,self.IVIntArray, OutputDataIntArray, byref(image_size))
                        
            if result != 0:
                raise NameError
            
        except NameError:
            print("\n SBU_Crypto.SBU_AES_CTR_EncryptFile returned an error !!" + str(result))
            sys.exit()        
        
        return dict(Hash = OutputDataIntArray.raw, SizeOfRec = image_size.value)
        
     
# This class is used to keep the random nonce    
class KeyNonce:
    #The function creates a random nonce (2 words)
    def __init__(self, isCodeEncrSupported, prjDefines, DLLHandle):
        try:            
            #generate 8 bytes of random data, in binary format
            if isCodeEncrSupported == 1:                
                self.randStr = create_string_buffer(8)
                result = DLLHandle.SBU_RAND_Bytes(8, self.randStr)
                if result <= 0:
                    raise NameError                                
            else:
                self.randStr = create_string_buffer(8)
                for i in range(8):
                        self.randStr[i] = 0
                        
            self.PrjDefines = prjDefines            
                        
        except NameError:        
            print("\n CreateNonce failed, failed to create random number! ")
            sys.exit()
    
        return
    
    # This method is used to return the Nonce as binary string
    # The nonce is a byte array of 8 bytes but it is read in the sb code as 2 words, therefore in case of little endian 
    # it is reversed and in case of BIG lest as is.
    def VarsToBinString(self):
        str1 = byte2stringBytesArray(self.randStr)        
        return str1

#End of CreateNonce

# This class is used to get the additional data and return it
class AdditionalData:
    # The function opens the file and gets the data if exist 
    def __init__(self, isAddDataExist, AddDataFileName):
        try:
            if isAddDataExist == 1:
                fb = open(AddDataFileName, "rb")
                self.addData = fb.read()
                fb.close()
                if len(self.addData) != ADDITIONAL_DATA_SIZE:
                    print("\n User's additional data size is illegal")
                    sys.exit()
            else:
                strBin = str()
                # need to create a string of 128 bytes of 0
                for i in range(ADDITIONAL_DATA_SIZE):
                    strBin = strBin + '\0'
                self.addData = strBin
                
        except IOError as Error3:
            (errno, strerror) = Error3.args
            print("\n Error in opening file - %s" %FileName)
            sys.exit()
        return

    # This function returns the binary value of the classes strings
    def VarsToBinString(self):
        DataBinStr = self.addData
     
        return byte2string(DataBinStr)
#End of AdditionalData   

