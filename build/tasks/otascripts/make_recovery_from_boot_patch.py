#!/usr/bin/env python
#add by spreadtrum

import sys
if sys.hexversion < 0x02070000:
  print >> sys.stderr, "Python 2.7 or newer is required."
  sys.exit(1)
import os
import imp
import tempfile
from hashlib import sha1 as sha1

sys.path.append('./build/tools/releasetools')
import common
import sign_target_files_secureboot as sign

OPTIONS = common.OPTIONS
OPTIONS.secure_boot = False
OPTIONS.secure_boot_tool = None
OPTIONS.single_key = True

def LoadInfoDict(input_dir):
  d = {}
  try:
    with open(os.path.join(input_dir, "sprd_misc_info.txt"), "rb") as f:
      data_sprd_misc = f.read()
      d = common.LoadDictionaryFromLines(data_sprd_misc.split("\n"))
  except IOError, e:
    print "can't find sprd_misc_info.txt!"
  return d

def GetBootableImage(prebuilt_name, name, input_dir):
  prebuilt_path = os.path.join(input_dir, prebuilt_name)
  if os.path.exists(prebuilt_path):
    print "find %s..." % (prebuilt_name)
    f = open(prebuilt_path, "rb")
    data = f.read()
    if OPTIONS.secure_boot:
      print"secure_boot sign"
      data = sign.DoSprdSign("vlr", name, data, "other")
    f.close()
    return common.File(prebuilt_name, data)
  return None

def main(argv):
  args = common.ParseOptions(argv, __doc__)
  input_dir, output_dir = args
  print "*********************************************************************"
  print "*********************************************************************"
  print "*********************************************************************"
  print os.path.join(input_dir, "recovery/root/etc/recovery.fstab")
  print "*********************************************************************"
  print "*********************************************************************"
  print "*********************************************************************"
  def read_helper(input_path):
    try:
      with open(os.path.join(input_dir, "recovery/root/etc/recovery.fstab")) as f:
        return f.read()
    except IOError, e:
      print"no such file!"

  OPTIONS.info_dict = LoadInfoDict(input_dir)
  OPTIONS.info_dict["fstab"] = sign.LoadRecoveryFSTab(read_helper, 2)
  OPTIONS.secure_boot = OPTIONS.info_dict.get("secure_boot", False)
  OPTIONS.secure_boot_tool = OPTIONS.info_dict.get("secure_boot_tool", None)
  OPTIONS.single_key = OPTIONS.info_dict.get("single_key", True)

  recovery_img = GetBootableImage("IMAGES/recovery.img", "recovery.img", input_dir)
  boot_img = GetBootableImage("IMAGES/boot.img", "boot.img", input_dir)

  if not recovery_img or not boot_img:
    sys.exit(0)

  def output_sink(fn, data):
    if fn == "etc/install-recovery.sh":
      fn = "bin/install-recovery.sh"
    if data:
      with open(os.path.join(output_dir, "system", *fn.split("/")), "wb") as f:
        f.write(data)

  common.MakeRecoveryPatch(input_dir, output_sink, recovery_img, boot_img, OPTIONS.info_dict)

if __name__ == '__main__':
  main(sys.argv[1:])
