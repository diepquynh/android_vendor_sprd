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

import sys
import os 

# Adding the utility python scripts to the python PATH

# Definitions for paths
#######################
if sys.platform != "win32" :
    path_div = "/"
else : #platform = win32
    path_div = "\\"


CURRENT_PATH = sys.path[0]
# In case the scripts were run from current directory
CURRENT_PATH_SCRIPTS = path_div + 'sbu_scripts'

# this is the scripts local path, from where the program was called
sys.path.append(CURRENT_PATH+CURRENT_PATH_SCRIPTS)

# this is the path of the proj config file
PROJ_CFG_PATH = "src" + path_div
#PROJ_CONFIG = CURRENT_PATH + path_div + ".." + path_div + PROJ_CFG_PATH + 'proj.cfg'
PROJ_CONFIG = CURRENT_PATH + path_div + 'proj.cfg'


from key_data_structures import *
from cnt_data_structures import *
import string
from global_defines import *
from ctypes import *
import global_defines
import configparser
import re
from cert_basic_utilities import *

####################################################################
# Filename - securebootutility.py
# Description - This file contains the main functionality of the
#               secure boot utility. The utility creates a certificate
#               that is used in the secure boot process
####################################################################

########### Data Records analyzer functions ###########

# The ImageFileAnalyzer function analyzes the files list file and return a list of data records objects.
# The function does the following steps:
# Step 1 - Open the file (that contain the files list)
# Step 2 - Get each line in the file and insert it into a list of dictionaries
#          each dictionary contain:
#               image name (SW component name),
#               Memory Load address
#               Storage address of the image
# Step 3 - For each SW image (SW component) calculate its HASH and add to dictionary alopng with its size
# Step 4 - Create a record object and insert the data into it
def ImageFileAnalyzer(logFile, FileName, IsCodeEncSupported, SBU_DLLHandle, keyFileName, nonce):

    DataRecsList = list()
    
    # Open the file (that contain the files list)
    try:
        FileObj = open(FileName , "r")
        LinesDictList = list()
        FileLines = FileObj.readlines()
        # If the file is empty the data record list will be returned empty
        if len(FileLines) == 0 :
            print_and_log(logFile, "illegal number of images = 0 !")
            sys.exit(1)   
        # Get each line in the file and insert it into a list of dictionaries
        # each dictionary contain the image name (SW component name), memory load address and the Flash address
        # of the image
        for lineObj in FileLines:
            # if it is comment ignore
            if re.match(r'^#', lineObj):
                continue
            LinesDictList.append(LineAnalyzer(logFile, lineObj))
        # For each SW image (SW component) calculate its HASH and add to dictionary (hash and size)
        for ListObj in LinesDictList:
            ListObj.update(HashResOnSWComponent(logFile, ListObj.get('ImgName'),IsCodeEncSupported, SBU_DLLHandle,ListObj.get('MemLoadAddrList'), keyFileName, nonce))
            # Create a record object and insert the data into it            
            CertRecordInfoObj = CertRecordInfo(ListObj.get('FlashStoreAddr'), ListObj.get('MemLoadAddr'), ListObj.get('MemLoadAddrList'), ListObj.get('SizeOfRec'),ListObj.get('Hash'), PrjDefines)
            DataRecsList.append(CertRecordInfoObj)
            
    except IOError as Error1:
        (errno, strerror) = Error1.args
        print_and_log(logFile, "\n Error in openning file - %s" %FileName)
        sys.exit(1)
        
    return DataRecsList
# End of ImageFileAnalyzer

# The LineAnalyzer function takes each line in the files list file and separate it
# Each line is expected to be in the following structure:
# <s/w_comp_name> <mem_load_addr> <flash_store_addr>
# The function returns a dictionary with the line data. (ImgName, MemLoadAddr, FlashStoreAddr)
# In case the functions reads an illegal image address (not word alligned) an exception is raised
def LineAnalyzer(logFile, FileLine):
    try:                
        LineList = FileLine.split(" ")        
        MemLoadAddr = int(LineList[1],16)        
        MemLoadAddrList = createAddrList(LineList[1][2:])        
        FlashStoreAddr = int(LineList[2],16)                

    except NameError:
        print_and_log(logFile, "\n Illegal Address - not word alligned !!")
        sys.exit(1)
        
    return dict(ImgName = LineList[0], MemLoadAddr = MemLoadAddr, MemLoadAddrList = MemLoadAddrList, FlashStoreAddr = FlashStoreAddr)
# End of LineAnalyzer

# The HashResOnSWComponent decides on which HASH algorithm should be used and call the correct function
# Currently HASH SHA256 output & SHA 256 output trucated to 128 bits output are supported
def HashResOnSWComponent(logFile, FileName, isCodeEncrSupported, SBU_DLLHandle, memoryLoadAddr, keyFileName, nonce):
    try:        
        if isCodeEncrSupported == 1 :
            codeEncObj = CodeEncryptionData(keyFileName, memoryLoadAddr, nonce, PrjDefines)
            return codeEncObj.AESEncryptDataAndHash(FileName, SBU_DLLHandle)
        else:  
            return HashResOnSWComponent_SHA256(logFile, FileName, SBU_DLLHandle)
    except NameError:
        print_and_log(logFile, "\n HASH on SW component failed ! ")
        sys.exit(1)
        
    return     
# End of HashResOnSWComponent

# The HashResOnSWComponent_SHA256 function reads the image file (SW component) and calculate HASH on it
def HashResOnSWComponent_SHA256(logFile, FileName, SBU_Crypto):
    try:
        image_size = c_uint()
        OutputDataIntArray = create_string_buffer(SHA_256_HASH_SIZE_IN_BYTES)
            
        result = SBU_Crypto.SBU_AES_CTR_EncryptFile(str.encode(FileName), None, None, 0 ,None, OutputDataIntArray, byref(image_size))
        
    except NameError:        
        print_and_log(logFile, "Error in Hash !")
        sys.exit(1)

    return dict(Hash = OutputDataIntArray.raw, SizeOfRec = image_size.value)

def createAddrList (dataStr):
    dataList = list()

    if len(dataStr)%2 == 1: #miss a zero at the beginning 
        newStr = '0'+dataStr
    else:
        newStr = dataStr

    for i in range(int(len(newStr)/2)):
        dataList.append(newStr[i*2:i*2+2])
    return dataList

########### Data Records analyzer functions End ###########

########### RSA Public key data & signature generation #############

# The function GetRSAKeyParams reads the key file (in PEM format) parse it and gets the public and private RSA key data.
# Set the N buffer (in binary representation) and calculates the Np (Barrett n' value).
def GetRSAKeyParams(logFile, RSAKeyFileName, PassphraseFileName, SBU_Crypto):
    publicKey = create_string_buffer(RSA_SIGNATURE_SIZE_IN_BYTES + NP_SIZE_IN_BYTES)
    result = SBU_Crypto.SBU_GetNAndNpFromKeyPair(str.encode(RSAKeyFileName), str.encode(PassphraseFileName), publicKey)
    if result != 0:
        print_and_log(logFile, "Error in public key | Np")
        sys.exit(1)
      
    # Create the public key object and return it in binary format
    return CertNPublicKey(publicKey)
# End of GetRSAKeyParams

# The function GetRSAKeyHParams reads the key file (in PEM format) parse it and gets the public RSA key data.
# Set the N buffer (in binary representation) and calculates the H.
def GetRSAKeyHParams(logFile, RSAKeyFileName, PassphraseFileName, SBU_Crypto):
    publicKey = create_string_buffer(RSA_SIGNATURE_SIZE_IN_BYTES)
    hashKey = create_string_buffer(RSA_SIGNATURE_SIZE_IN_BYTES*2+2)
    result = SBU_Crypto.SBU_GetNFromKeyPairAndCalcH(str.encode(RSAKeyFileName), str.encode(PassphraseFileName), publicKey, hashKey)
    if result != 0:
        print_and_log (logFile, "Error in public key | Hash")
        sys.exit(1)
  
    # Create the public key object 
    return CertNPublicKeyHData(publicKey, hashKey)
# End of GetRSAKeyHParams

# The function GetRSASignature calculates the RSA signature
# The function calls the CRYPTO DLL to calculate the RSA signature
def GetRSASignature(logFile, Data, PrivKeyFile, PassphraseFile, CryptoDLL_handle):    
    return RSA_SIGN_DATA(logFile, Data, PrivKeyFile, PassphraseFile, CryptoDLL_handle)
# End of GetRSASignature

# The function RSA_SIGN_DATA signs on given data using the RSA PKCS#1Ver15 algorithm
def RSA_SIGN_DATA(logFile, DataIn, PrivKeyFile, PassphraseFile, CryptoDLL_handle):
    try:
        DataInSize = len(DataIn)
        Signature = create_string_buffer(RSA_SIGNATURE_SIZE_IN_BYTES)
        
        if (PrjDefines[LIST_OF_CONF_PARAMS.index("RSA_ALGORITHM")] != RSA_ALGORITHM_SUPPORT_RSA_PSS_2048_SHA256) and (PrjDefines[LIST_OF_CONF_PARAMS.index("RSA_ALGORITHM")] != RSA_ALGORITHM_SUPPORT_RSA_PKCS15_2048_SHA256):                
        #if RSA_ALGORITHM != RSA_ALGORITHM_SUPPORT_RSA_PSS_2048_SHA256 and RSA_ALGORITHM != RSA_ALGORITHM_SUPPORT_RSA_PKCS15_2048_SHA256:
            print_and_log(logFile, "\n RSA Algorithm is not supported !!")
            raise NameError
        # Do Rsa Sign and get the signature
        # N, D and DataIn are sent to the function as int arrays     

        p1=str.encode(PrivKeyFile)
        p2=str.encode(PassphraseFile)
        p3=DataIn.encode('iso-8859-1')
        result = CryptoDLL_handle.SBU_RSA_Sign(PrjDefines[LIST_OF_CONF_PARAMS.index("RSA_ALGORITHM")], p3, DataInSize, p1, p2, Signature)
        if result != 0:
            print_and_log(logFile, "\n SBU_CRYPTO_DLL.SBU_RSA_Sign returned an error !!")                
            raise NameError

    except NameError:        
        sys.exit(1)
    return CertRSASignature(ReverseBytesinBinString(Signature))
# End of RSA_SIGN_DATA

# The function GetPubKeyHash returns the hash of the (N|Np).
def GetPubKeyHash(logFile, RSAPubKeyFileName, SBU_Crypto):
    hashData = create_string_buffer(SHA_256_HASH_SIZE_IN_BYTES)
        
    result = SBU_Crypto.SBU_GetHashOfNAndNpFromPubKey(str.encode(RSAPubKeyFileName), hashData, SHA_256_HASH_SIZE_IN_BYTES)
    if result != 0:
        print_and_log (logFile, "Error in hash(public key | Np)")
        sys.exit(1)
      
    # Create the public key object and return it in binary format
    return CertPubKeyHASHData(hashData)
# End of GetPubKeyHash

########### RSA functions end ######################################

########### Certificate utility functions ##########################

# The GetSWVersion function returns CertSwVersion object with swversionId and val
def GetSWVersion(logFile, swVersionId, swVersionVal):    
    maxVerId = PrjDefines[LIST_OF_CONF_PARAMS.index("NUM_OF_REVOCATION_COUNTERS_SUPPORT")]
    if ( (swVersionId > maxVerId) or (swVersionId < 1)):
        print_and_log(logFile, "Illegal revocation counter ID !!")
        sys.exit(1)

    if swVersionId == 1:
        if swVersionVal > SW_REVOCATION_MAX_NUM_OF_BITS_CNTR1:
            print_and_log(logFile, "Illegal revocation value !!")
            sys.exit(1)
    if swVersionId == 2:
        if swVersionVal > SW_REVOCATION_MAX_NUM_OF_BITS_CNTR2:
            print_and_log(logFile, "Illegal revocation value !!")
            sys.exit(1)

    CertSwVersionObj = CertSwVersion(PrjDefines, swVersionId, swVersionVal)

    return CertSwVersionObj
# End of GetSWVersion

# The BinStrToList function takes a binary string and returns a list with HEX
# representation for the bytes
def BinStrToList(str1):    
    TempList = list()
    ConvList = list(str1.encode('iso-8859-1'))
    for i in range(len(str1)):
        TempList.append("0x%02x" % ConvList[i])
        
    return TempList
# End of BinStrToList

# The CreateCertBinFile opens a binary and text file and writes the certificate data into it 
def CreateCertBinFile(logFile, binStr, txtList, certFileName):
    try:
        # Open a binary file and write the data to it
        FileObj = open(certFileName, "wb")
        FileObj.write(bytes(binStr.encode('iso-8859-1')))
        FileObj.close()

        # Assemble the text file name (certificate + .txt)
        certFileNameTxt = certFileName[:-4] + '_' + Cert_FileName + Cert_FileExtTxt
        # Open a text file and write the data into it, in lines of 4 bytes
        FileObj = open(certFileNameTxt, "w")
        
        NumOfChars = len(txtList)
        if DEBUG_MODE == DEBUG_MODE_OFF:
            FileObj.write("char cert_bin_image[] = {\n")
            for i in range(NumOfChars):   
                FileObj.write(txtList[i])
                if i !=  NumOfChars-1:
                    FileObj.write(',')
                if (i+1) % 4 == 0:
                    FileObj.write('\n')
            FileObj.write("}")
        if DEBUG_MODE == DEBUG_MODE_ON:
            wordsList = CreateWordsListFromBytesList(txtList)
            for obj in wordsList:
                FileObj.write(obj + ",")
                FileObj.write('\n')
        FileObj.close()    
    except IOError as Error7:
        (errno, strerror) = Error7.args
        print_and_log(logFile, "Error in openning file - %s" %certFileName)
        sys.exit(1)
    return       
# End of CreateCertBinFile	

def CreateWordsListFromBytesList(BytesList):
    # Create words in reverse order
    wordsList = list()
    if PrjDefines[LIST_OF_CONF_PARAMS.index("CERT_ENDIANITY")] == CERT_IN_LITTLE_ENDIAN:
        length = len(BytesList)/4        
        for i in range(int(length)):
            tmpStr = str()
            for j in range(4):                
                byte = str()
                byte = BytesList[i*4 + 4 - j - 1]
                byte = byte[2:]
                tmpStr = tmpStr + byte
            tmpStr = '0x' + tmpStr   
            wordsList.append(tmpStr)
    else:
        for i in range(len(BytesList)/4):
            tmpStr = str()
            for j in range(4):
                tmpStr = tmpStr + BytesList[i*4 + j]
            wordsList.append(tmpStr)
    return wordsList    
            

########### certificate creation - Utility functions End ###########

# Parse given configuration file and return attributes as dictionary
def parse_config_file (config_fname, log_file):

    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        print_and_log(log_file,"Failed opening " + config_fname + " (" + e.strerror + ")\n")
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.readfp(config_file)
    config_file.close()    
    
    local_dict = dict()
    
    section_name = "CNT-CFG"
    if not config.has_section(section_name):
        log_sync(log_file, "section " + section_name + " wasn't found in cfg file\n")
        return None

    local_dict['cert_keypair'] = config.get(section_name, 'cert-keypair')                         
    log_sync(log_file,"cert-keypair: " + local_dict['cert_keypair'] + "\n")                       

    if config.has_option(section_name, 'cert-keypair-pwd'): 
        local_dict['cert_keypair_pwd'] = config.get(section_name, 'cert-keypair-pwd')                     
        log_sync(log_file,"cert-keypair-pwd: " + local_dict['cert_keypair_pwd'] + "\n")
    else:
        local_dict['cert_keypair_pwd'] = None                     

    local_dict['nvcounter_id'] = int(config.get(section_name, 'nvcounter-id'))                                         
    if (local_dict['nvcounter_id'] != int(1) and local_dict['nvcounter_id'] != int(2)):
        log_sync(log_file, "Ilegal nvcounter-id defined - exiting\n")
        return None
    log_sync(log_file,"nvcounter-id: " + str(local_dict['nvcounter_id']) + "\n") 

    local_dict['nvcounter_val'] = int(config.get(section_name, 'nvcounter-val'))                                         
    if (local_dict['nvcounter_id'] == int(1) and local_dict['nvcounter_val'] >= int(32)):
        log_sync(log_file, "Ilegal nvcounter-val for trsuted NV-Counter defined - exiting\n")
        return None
    if (local_dict['nvcounter_id'] == int(2) and local_dict['nvcounter_val'] >= int(224)):
        log_sync(log_file, "Ilegal nvcounter-val for non-trsuted NV-Counter defined - exiting\n")
        return None
    log_sync(log_file,"nvcounter-val: " + str(local_dict['nvcounter_val']) + "\n") 
               
    if config.has_option(section_name, 'aes-enc-key'): 
        local_dict['encyption_flag'] = int(1)
        local_dict['aes_enc_key'] = config.get(section_name, 'aes-enc-key')                     
        log_sync(log_file,"aes-enc-key: " + local_dict['aes_enc_key'] + "\n")
    else:
        local_dict['encyption_flag'] = int(0)
        local_dict['aes_enc_key'] = None                     

    local_dict['images_table'] = config.get(section_name, 'images-table')                         
    log_sync(log_file,"images-table: " + local_dict['images_table'] + "\n")
       
    local_dict['cert-pkg'] = config.get(section_name, 'cert-pkg')                         
    log_sync(log_file,"cert-pkg: " + local_dict['cert-pkg'] + "\n")       
        
    return local_dict

# Parse script parameters
def parse_shell_arguments ():
    len_arg =  len(sys.argv)
    if len_arg < 2:
        print("len " + str(len_arg) + " invalid. Usage:" + sys.argv[0] + "<test configuration file>\n")
        for i in range(1,len_arg):
            print("i " + str(i) + " arg " + sys.argv[i] + "\n")
        sys.exit(1)
    config_fname = sys.argv[1]
    if len_arg == 3:
        log_fname = sys.argv[2]
    else:
        log_fname = "sb_content_cert.log"
    return config_fname, log_fname

# The function analyzes the input files and creates a key certificate binary file to be used in the
# secure boot process. 
# Key certificate structure is :
#       FIELD NAME                                                  SIZE (words)
#       ----------                                                  ------------
#       Header magic num                                            1
#       cert version num                                             1
#       cert size (offset to signature)                              1
#       Flags                                                       1
#       N Pub key                                                   64
#       Np  or H                                                    5 || 64
#       revocation counter ID                                       1
#       current SW version                                          1
#       NONCE                                                       2
#       comp hash + load address * N                                N * (8+1)
#       RSA Signature                                               64
#       comp storage add + size * N                                 N * (2)                
#
# The function does the following steps:
# 1. Create the certificate header and add to list 
# 2. Create RSA public key parameters and add to list
# 3. Create SW version parameters and add to list
# 4. Add the next public key HASH
# 5. In a loop create binary string out of the certificate so far (header + public key + sw version + HASH)
# 6. Do RSA signature over the HASH value of the certificate so far
# 7. Build the end of the certificate
# 8. Write the certificate as binary and text string to file
#
# In case an error occurs the function throws exception and exits
#################################################################################

def CreateCertUtility(sysArgsList):        
    try:          
        config_fname, log_fname =  parse_shell_arguments()
             
        log_file = create_log_file(log_fname)             
        # Check the input parameters and save it to list
        ArgsDict = parse_config_file(config_fname, log_file)
        if ArgsDict == None:
               log_file.close()
               exit(1) 

                
        print_and_log(log_file, "**** Creating Content Certificate **** ")
        # Create the certificate objects and add it to a list
        CertDataList = list()
        DLLHandle = LoadDLLGetHandle()        
        
        # Create nonce if code encryption is supported
        keyNonce = KeyNonce(ArgsDict['encyption_flag'], PrjDefines, DLLHandle)                  

        # Create list out of the sw component file (create HASH & addresses), the list contains data records objects
        # In case the code encryption is supported the SW component will also be encrypted        
        print_and_log(log_file, "\n Analyze the list of s/w components and create HASH value for each component")
        
        RecList = ImageFileAnalyzer(log_file, ArgsDict['images_table'], ArgsDict['encyption_flag'], DLLHandle, ArgsDict['aes_enc_key'], keyNonce.randStr)

        print_and_log(log_file, "\n Prepare Certificate header ")
        # Create the certificate header and add to list -> header includes 
        CertDataList.append(CertHeader(log_file, int('0xff',16), CERT_TYPE_CONTENT, ArgsDict['encyption_flag'], len(RecList), PrjDefines))

        print_and_log(log_file, "\n Create RSA public key parameters to insert to the certificate")        
        # Create RSA key parameters and add to list (according to which Public key derivative is used)               
        if RSA_ALG_USE_Np == 1: # use Np            
            RSAPubKey = GetRSAKeyParams(log_file, ArgsDict['cert_keypair'], ArgsDict['cert_keypair_pwd'], DLLHandle)        
        else: # use H
            RSAPubKey = GetRSAKeyHParams(log_file, ArgsDict['cert_keypair'], ArgsDict['cert_keypair_pwd'], DLLHandle)
        CertDataList.append(RSAPubKey)

        print_and_log(log_file, "\n Get SW version parameters")
        # Create SW version parameters and add to list
        CertDataList.append(GetSWVersion(log_file, ArgsDict['nvcounter_id'], ArgsDict['nvcounter_val']))
        
        # Add the nonce in case code encryption is supported        
        CertDataList.append(keyNonce)
        
        print_and_log(log_file, "\n Create the certificate as binary string and calculate RSA signature on it")
        # In a loop create binary string out of the certificate so far (header + public key + sw version + pub key HASH)
        BinStr = str()
        for obj in CertDataList:                        
            BinStr = BinStr + obj.VarsToBinString()

        # In a loop add the component objects to the binary string (only if not empty)
        for obj in RecList:
            BinStr = BinStr + obj.VarsToBinStringHashComp()

        # Do RSA signature  
        Signature = GetRSASignature(log_file, BinStr, ArgsDict['cert_keypair'], ArgsDict['cert_keypair_pwd'], DLLHandle)
        
        print_and_log(log_file, "\n Add the signature to the certificate ")
        # Build the end of the certificate - add the signature
        
        BinStr = BinStr + Signature.VarsToBinString()
        
        # Add the additional data        
        for obj in RecList:
            BinStr = BinStr + obj.VarsToBinStringParamComp()

        print_and_log(log_file, "\n Write the certificate to file ")
        # Write binary and text string to file    
        CreateCertBinFile(log_file, BinStr, BinStrToList(BinStr), ArgsDict['cert-pkg']) 
         
        print_and_log(log_file, "\n**** Certificate file creation has been completed successfully ****")
      
        
    except IOError as Error8: 
        (errno, strerror) = Error8.args 
        print("I/O error(%s): %s" % (errno, strerror))
        raise
    except NameError:
        print("Unexpected error, exiting program")
        raise  # Debug info
    except ValueError:
        print("Illegal variable type")
        raise # Debug info


##################################
#       Main function
##################################
        
if __name__ == "__main__":

    import sys
    if sys.version_info<(3,0,0):
        print("You need python 3.0 or later to run this script")
        exit(1)

    #if "-cfg_file" in sys.argv:
    #PROJ_CONFIG = CURRENT_PATH + path_div+sys.argv[1]
    #print("Config File  - %s\n" %PROJ_CONFIG)

    # Get the project configuration values
    PrjDefines = parseConfFile(PROJ_CONFIG,LIST_OF_CONF_PARAMS)
    
    CreateCertUtility(sys.argv)




    



######################################## END OF FILE ########################################

