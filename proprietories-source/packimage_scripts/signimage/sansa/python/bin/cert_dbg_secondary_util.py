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
#                       enabler certificate
#                       debugMask
#                       socid
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
    section_name = "SCND-DBG-CFG"
    if not config.has_section(section_name):
        log_sync(log_file, "section " + section_name + " wasn't found in cfg file\n")
        return None

    local_dict['cert_keypair'] = config.get(section_name, 'cert-keypair')
    log_sync(log_file,"cert-keypair: " + str(local_dict['cert_keypair']) + "\n")                       

    if config.has_option(section_name, 'cert-keypair-pwd'): #used for testing
        local_dict['cert_keypair_pwd'] = config.get(section_name, 'cert-keypair-pwd')
        log_sync(log_file,"cert-keypair-pwd: " + str(local_dict['cert_keypair_pwd']) + "\n")
    else:
        local_dict['cert_keypair_pwd'] = int(0)                     

    local_dict['soc_id'] = config.get(section_name, 'soc-id')
    log_sync(log_file,"soc-id: " + local_dict['soc_id'] + "\n")            
    local_dict['debug_mask'] = int(config.get(section_name, 'debug-mask'), 16) 
    log_sync(log_file,"debug-mask: " + str(local_dict['debug_mask']) + "\n")  
    if local_dict['debug_mask'] == 0:
        log_sync(log_file,"illegal empty mask \n")
        return None  

    local_dict['prim_dbg_cert_pkg'] = config.get(section_name, 'prim-dbg-cert-pkg')
    log_sync(log_file,"prim-dbg-cert-pkg: " + str(local_dict['prim_dbg_cert_pkg']) + "\n")
    local_dict['cert_pkg'] = config.get(section_name, 'cert-pkg')
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
    print_and_log(log_file, str(datetime.now()) + ": Secondary Debug Certificate Utility started (Logging to " + log_fname + ")\n")

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
                
        print_and_log(log_file, "**** Generate debug certificate ****\n")        

        # read the primary certificate from file to str to get the prim cert + get its length       
        primCertBin = GetDataFromBinFile(log_file, data_dict['prim_dbg_cert_pkg'])
        # create certificate header , get bin str and the header str
        # build header  
        certStrBin = build_certificate_header (DEBUG_SEC_TOKEN, DEBUG_CERT_VERSION, CERT_TYPE_SEC, 0, 0, 0, len(primCertBin))

        # add the prim certificate 
        certStrBin = certStrBin + byte2string(primCertBin)

        # add mask
        certStrBin = certStrBin + byte2string(struct.pack('<I', data_dict['debug_mask']))

        # add soc_id                
        socIdBin = GetDataFromBinFile(log_file, data_dict['soc_id'])
        certStrBin = certStrBin + byte2string(socIdBin)

        # Sign on certificate
        Signature = GetRSASignature(log_file, certStrBin, data_dict['cert_keypair'], data_dict['cert_keypair_pwd'], DLLHandle)
        certStrBin = certStrBin + Signature.VarsToBinString()

        # add signature and write to binary file
        CreateCertBinFile(log_file, certStrBin, data_dict['cert_pkg'])

        print_and_log(log_file, "**** Generate secondary debug certificate completed successfully ****\n")

    else:
        print_and_log(log_file, "**** Invalid config file ****\n")
        exit_main_func(log_file, config_file, 1)

    exit_main_func(log_file, config_file, 0)

#############################
if __name__ == "__main__":
    main()



