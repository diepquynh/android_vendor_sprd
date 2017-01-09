# Create by Spreadst

import copy
import errno
import getopt
import getpass
import imp
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
sys.path.append("build/tools/releasetools")
import common
import tempfile
import threading
import time
import zipfile

class KeyInfo(object):
  def __init__(self, product_name, password):
    self.product_name = product_name
    self.password = password

class PartitionSignInfo(object):
  def __init__(self, sign_key_name, save_key_name):
    self.sign_key_name = sign_key_name
    self.save_key_name = save_key_name

class SprdSign(object):
  """Class for Secure boot Sign"""

  def __init__(self, options, single_key):
    self.options = options
    if options.secure_boot_tool is None:
      raise common.ExternalError("no secure_boot_tool defined in \"META/misc_info.txt\" of target file")
    self.secure_boot_tool = options.secure_boot_tool
    self.key1 = None
    self.key2 = None
    self.key3 = None

    self.debug = True
    self.editor = os.getenv("EDITOR", None)
    self.cfgfile = os.getenv("SPRD_SECURE_BOOT_SIGN_CONFIG", None)
    self.single_key = single_key

    self.sign_key_config = {}

    if single_key is True:
      self.sign_key_config["/spl"] = PartitionSignInfo("key1", "key1")
      self.sign_key_config["/uboot"] = PartitionSignInfo("key1", "key1")
      self.sign_key_config["other"] = PartitionSignInfo("key1", "key1")
    else:
      self.sign_key_config["/spl"] = PartitionSignInfo("key1", "key2")
      self.sign_key_config["/uboot"] = PartitionSignInfo("key2", "key3")
      self.sign_key_config["other"] = PartitionSignInfo("key3", None)

  def UpdateAndReadConfig(self, current_config):
    if not self.editor or not self.cfgfile:
      return None

    try:
      f = open(self.cfgfile, "w")
      os.chmod(self.cfgfile, 0600)
      f.write("# Enter key passwords between the [[[ ]]] brackets.\n")
      f.write("# (Additional spaces are harmless.)\n")
      f.write("# fmt: keyN [[[ passwd ]]] product_name\n\n")
      if current_config:
        for k, v in sorted(current_config.items()):
          f.write("%s [[[  %s  ]]] %s\n" % (k, v[1], v[2]))
      f.close()
    except IOError, e:
      if e.errno != errno.ENOENT:
        print "error writing config file(%s): " % (self.cfgfile,), str(e)

    p = self.Run([self.editor, "+4", self.cfgfile])
    _, _ = p.communicate()

    return self.ReadConfig()

  def ReadConfig(self):
    if not self.cfgfile: return None

    result = {}
    try:
      f = open(self.cfgfile, "r")
      for line in f:
        line = line.strip()
        if not line or line[0] == '#': continue
        m = re.match(r"^(\S+)\s*\[\[\[\s*(.*?)\s*\]\]\]\s*(\S+)$", line)
        if not m:
          print "failed to parse config file: ", line
        else:
          result[m.group(1)] = (m.group(3), m.group(2))
      f.close()
    except IOError, e:
      if e.errno != errno.ENOENT:
        print "error reading config file(%s): " % (self.cfgfile,), str(e)

    return result

  def GetSignConfig(self, img_name, part_sign_info):
    sign_key_name = part_sign_info.sign_key_name
    save_key_name = part_sign_info.save_key_name
    if self.cfgfile:
      config = self.ReadConfig()
      if not config:
        config = self.UpdateAndReadConfig(config)
      if config:
        for k, v in sorted(config.items()):
          self.SetAttrValue(self, k, KeyInfo(v[0], v[1]))

    if getattr(self, sign_key_name) is None:
      self.GenerateKey("sign", img_name, sign_key_name)
    if save_key_name and getattr(self, save_key_name) is None:
      self.GenerateKey("save into", img_name, save_key_name)

  def GenerateKey(self, key_action, img_name, key_name):
    key = KeyInfo(None, None)
    self.GetInputOrPasswd(key, key_name, "input", "product_name", key_action, img_name)
    self.GetInputOrPasswd(key, key_name, "passwd", "password", key_action, img_name, depend_attr="product_name")
    self.SetAttrValue(self, key_name, key)

  def SetAttrValue(self, obj, attr_name, attr_val):
    if attr_val and attr_val != "":
      setattr(obj, attr_name, attr_val)

    if getattr(obj, attr_name) is None:
      raise RuntimeError("%s unavailable" % (attr_name,))

  def GetInputOrPasswd(self, obj, key_name, get_type, attr_name, key_action, img_name, depend_attr=None):
    depend_info = ""
    if depend_attr:
      depend_info = " for %s \'%s\'" % (depend_attr, getattr(obj, depend_attr))
    last_info = ""
    if get_type is "input":
      last_info = "[%s]" % (getattr(obj, attr_name),)
    notify_info = "please input %s.%s%s to %s %s. %s>" % (key_name, attr_name, depend_info, key_action, img_name, last_info)

    if get_type is "input":
      input_val = raw_input(notify_info).strip()
    elif get_type is "passwd":
      input_val = getpass.getpass(notify_info).strip()

    self.SetAttrValue(obj, attr_name, input_val)

  def DoSign(self, sign_type, source_img, secure_img, mount_point):
    part_sign_info = None
    if mount_point in self.sign_key_config.keys():
      part_sign_info = self.sign_key_config[mount_point]
    else :
      part_sign_info = self.sign_key_config["other"]

    self.GetSignConfig(os.path.basename(source_img), part_sign_info)
    cmd_opts = []
    exec_file = ""
    if sign_type == "vlr":
      exec_file = os.path.join(self.secure_boot_tool, "VLRSign")
    elif sign_type == "bsc":
      exec_file = os.path.join(self.secure_boot_tool, "BscGen")

    cmd = [exec_file, "-img", source_img, "-out", secure_img]

    if part_sign_info.sign_key_name:
      key = getattr(self, part_sign_info.sign_key_name)
      cmd.extend(["-pw", key.password, "-pn", key.product_name])

    if part_sign_info.save_key_name:
      key = getattr(self, part_sign_info.save_key_name)
      cmd.extend(["-pw2", key.password, "-pn2", key.product_name])

    cmd.extend(cmd_opts)
    print "cmd = %s" % cmd
    p = self.Run(cmd, stdout=subprocess.PIPE)
    p.communicate()
    #assert p.returncode == 0, "SprdSign DoSign(%s, %s, %s) faild" % (sign_type, source_img, secure_img)

    return

  def Run(self, args, **kwargs):
    """Create and return a subprocess.Popen object, printing the command
    line on the terminal if -v was specified."""
    if self.debug and self.options.verbose:
      print "  running: ", " ".join(args)
    return subprocess.Popen(args, **kwargs)

