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

# This utility builds primary debug certifiacte package:
# the package Header format is:
#                       token
#                       version
#                       length 
#                       flags
#                       oemPubKey (pub key + Np)
#                       devPubKey (pub key + Np)
#                       debugMask
#                       rmaFlag
#                       certSign

import sys
# Definitions for paths
if sys.platform != "win32" :
    path_div = "//"    
else : #platform = win32
    path_div = "\\"


CURRENT_PATH = sys.path[0]
# In case the scripts were run from current directory
CURRENT_PATH_SCRIPTS = path_div + 'sd_scripts'
# this is the scripts local path, from where the program was called
sys.path.append(CURRENT_PATH+CURRENT_PATH_SCRIPTS)

import configparser
from cert_util_helper import *
from cert_dbg_util_gen import *
from cert_dbg_util_data import *

# Parse given test configuration file and return test attributes as dictionary
def parse_config_file (config, log_file):
    local_dict = {}
    section_name = "PRIM-DBG-CFG"
    if not config.has_section(section_name):
        log_sync(log_file, "section " + section_name + " wasn't found in cfg file\n")
        return None

    is_debug = 0
    if config.has_option(section_name, 'debug-mask'): 
        debug_mask = int(config.get(section_name, 'debug-mask'), 16)
        log_sync(log_file, "debug_mask " + str(debug_mask) + "\n")
        is_debug = 1

    is_rma = 0
    if config.has_option(section_name, 'rma-mode'): 
        rma_mode = int(config.get(section_name, 'rma-mode'))
        if rma_mode != int(0):
            log_sync(log_file, "rma_mode " + str(rma_mode) + "\n")
            is_rma = 1

    is_hbk = 0
    if config.has_option(section_name, 'hbk-id'): 
        hbk_id = int(config.get(section_name, 'hbk-id'))
        if (hbk_id == int(0) or hbk_id == int(1) or hbk_id == int(2)):
                log_sync(log_file, "hbk_id " + str(hbk_id) + "\n")
                is_hbk = 1

    is_key_pkg = 0
    if config.has_option(section_name, 'key-cert-pkg'): 
        key_pkg = config.get(section_name, 'key-cert-pkg')
        log_sync(log_file, "key_pkg " + key_pkg + "\n")
        is_key_pkg = 1

    if is_rma == 1  and is_debug == 1:
        log_sync(log_file, "Both RMA and debug mode are defined - exiting\n")
        return None
    if is_rma == 0 and is_debug == 0:
        log_sync(log_file, "RMA nor Debug mode not defined - exiting\n")
        return None

    # this condition also verifies that the chain will not have a key certificate
    if is_rma == 1 and is_hbk == 0:
        log_sync(log_file, "RMA defined without hbk-id - exiting\n")
        return None

    if is_key_pkg == 0 and is_hbk == 0:
        log_sync(log_file, "hbk-id nor key-cert-pkg not defined - exiting\n")
        return None

    if is_key_pkg == 1 and is_hbk == 1:
        log_sync(log_file, "hbk-id and key-cert-pkg defined - exiting\n")
        return None
    

    local_dict['debug_mask'] = int(0)                                         
    local_dict['rma_mode'] = int(0)
    local_dict['hbk_id'] = int(0xFF)
    local_dict['key_cert_pkg'] = ""                         

    if is_debug == 1: 
        local_dict['debug_mask'] = debug_mask
        log_sync(log_file,"debug-mask: " + str(local_dict['debug_mask']) + "\n")
        if is_hbk == 1:
            local_dict['hbk_id'] = hbk_id
            log_sync(log_file,"hbk-id: " + str(local_dict['hbk_id']) + "\n") 
        if is_key_pkg == 1:
            local_dict['key_cert_pkg'] = key_pkg
              
    if is_rma == 1:
        local_dict['rma_mode'] = int(1)
        log_sync(log_file,"rma-mode: " + str(local_dict['rma_mode']) + "\n") 
        local_dict['hbk_id'] = hbk_id
        log_sync(log_file,"hbk-id: " + str(local_dict['hbk_id']) + "\n") 

    local_dict['cert_keypair'] = config.get(section_name, 'cert-keypair')
    log_sync(log_file,"cert-keypair: " + str(local_dict['cert_keypair']) + "\n")                       

    if config.has_option(section_name, 'cert-keypair-pwd'): #used for testing
        local_dict['cert_keypair_pwd'] = config.get(section_name, 'cert-keypair-pwd')                     
        log_sync(log_file,"cert-keypair-pwd: " + str(local_dict['cert_keypair_pwd']) + "\n")
    else:
        local_dict['cert_keypair_pwd'] = int(0)                     

    local_dict['lcs'] = int(config.get(section_name, 'lcs'))                                         
    if (local_dict['lcs'] != int(1) and local_dict['lcs'] != int(2)):
        log_sync(log_file, "Ilegal lcs defined - exiting\n")
        return None
    log_sync(log_file,"lcs: " + str(local_dict['lcs']) + "\n") 
    if local_dict['lcs'] == int(1):
        local_dict['lcs'] = DX_MNG_DEVICE_MANUFACTURE_LCS
    else:
        local_dict['lcs'] = DX_MNG_SECURE_LCS

    local_dict['next_cert_pubkey'] = config.get(section_name, 'next-cert-pubkey')
    log_sync(log_file,"next-cert-pubkey: " + str(local_dict['next_cert_pubkey']) + "\n") 
      
    local_dict['cert_pkg'] = str.encode(config.get(section_name, 'cert-pkg'))
    log_sync(log_file,"cert-pkg: " + str(local_dict['cert_pkg']) + "\n")   
               
    return local_dict

# Parse script parameters
def parse_shell_arguments ():
    len_arg =  len(sys.argv)
    if len_arg < 2:
        print_sync("len " + str(len_arg) + " invalid. Usage:" + sys.argv[0] + "<test configuration file>\n")
        for i in range(1,len_arg):
            print_sync("i " + str(i) + " arg " + sys.argv[i] + "\n")
        sys.exit(1)
    config_fname = sys.argv[1]
    if len_arg == 3:
        log_fname = sys.argv[2]
    else:
        log_fname = "sb_dbg1_cert.log"
    return config_fname, log_fname


# close files and exit script
def exit_main_func(log_file, config_file, rc):
    log_file.close()
    config_file.close()
    sys.exit(rc)

def main():        

    config_fname, log_fname = parse_shell_arguments()
    log_file = create_log_file(log_fname)
    print_and_log(log_file, str(datetime.now()) + ": Primary Debug Certificate Utility started (Logging to " + log_fname + ")\n")

    DLLHandle = LoadDLLGetHandle()        
    
    try:
        config_file = open(config_fname, 'r')
    except IOError as e:
        print_and_log(log_file,"Failed opening " + config_fname + " (" + e.strerror + ")\n")
        log_file.close()
        sys.exit(e.errno)

    config = configparser.ConfigParser()
    config.read(config_fname)
    data_dict = {}

    data_dict = parse_config_file(config, log_file)

    if (data_dict != None):
        certStrBin = str()
        if data_dict['key_cert_pkg'] != '':
            isKeyExist = 1
        else:
            isKeyExist = 0
        print_and_log(log_file, "**** Generate debug certificate ****\n")        
        # create certificate header , get bin str and the header str
        certStrBin = build_certificate_header (DEBUG_PRIM_TOKEN, DEBUG_CERT_VERSION, CERT_TYPE_PRIM, isKeyExist, data_dict['hbk_id'], data_dict['lcs'], 0)
        
        # if key package exists need to insert it into the primary certificate 
        if data_dict['key_cert_pkg'] != "":
            keyStr = GetDataFromBinFile(log_file, data_dict['key_cert_pkg'])
            certStrBin = certStrBin + byte2string(keyStr)

        # get the primary certificate public key + Np
        PrimNPublicKey = GetRSAKeyParams(log_file, data_dict['cert_keypair'], data_dict['cert_keypair_pwd'], DLLHandle)
        certStrBin = certStrBin + PrimNPublicKey.VarsToBinString()        

        # get secondary certificate public key + Np (from public key)
        SecNPublicKey = GetRSAPubKeyParams(log_file, data_dict['next_cert_pubkey'], DLLHandle)
        certStrBin = certStrBin + SecNPublicKey.VarsToBinString()
        
        # add mask
        certStrBin = certStrBin + byte2string(struct.pack('<I', data_dict['debug_mask']))

        # add RMA flag
        certStrBin = certStrBin + byte2string(struct.pack('<I', data_dict['rma_mode']))

        # Sign on certificate
        Signature = GetRSASignature(log_file, certStrBin, data_dict['cert_keypair'], data_dict['cert_keypair_pwd'], DLLHandle)
        certStrBin = certStrBin + Signature.VarsToBinString()

        # add signature and write to binary file
        CreateCertBinFile(log_file, certStrBin, data_dict['cert_pkg'])

        print_and_log(log_file, "**** Generate debug certificate completed successfully ****\n")

    else:
        print_and_log(log_file, "**** Invalid config file ****\n")
        exit_main_func(log_file, config_file, 1)

    #FreeDLLGetHandle(DLLHandle)
    exit_main_func(log_file, config_file, 0)

#############################
if __name__ == "__main__":
    main()



