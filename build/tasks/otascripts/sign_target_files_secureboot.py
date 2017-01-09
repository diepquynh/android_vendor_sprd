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
import sprd_secure_boot_sign as sprd_sign
sys.path.append("build/tools/releasetools")
import common

OPTIONS = common.OPTIONS
OPTIONS.secure_boot = False
OPTIONS.secure_boot_tool = None
OPTIONS.single_key = True
OPTIONS.sprd_sign = None

def LoadRecoveryFSTab(read_helper, fstab_version, system_root_image=False):
  class Partition(object):
    def __init__(self, mount_point, fs_type, device, length, device2, context, secure_boot, pre_part):
      self.mount_point = mount_point
      self.fs_type = fs_type
      self.device = device
      self.length = length
      self.device2 = device2
      self.context = context
      # SPRD: add for secure boot
      self.secureboot = secure_boot
      self.pre_part = pre_part

  try:
    data = read_helper("RECOVERY/RAMDISK/etc/recovery.fstab")
  except KeyError:
    print "Warning: could not find RECOVERY/RAMDISK/etc/recovery.fstab"
    data = ""

  if fstab_version == 1:
    d = {}
    for line in data.split("\n"):
      line = line.strip()
      if not line or line.startswith("#"):
        continue
      pieces = line.split()
      if not 3 <= len(pieces) <= 4:
        raise ValueError("malformed recovery.fstab line: \"%s\"" % (line,))
      options = None
      if len(pieces) >= 4:
        if pieces[3].startswith("/"):
          device2 = pieces[3]
          if len(pieces) >= 5:
            options = pieces[4]
        else:
          device2 = None
          options = pieces[3]
      else:
        device2 = None

      mount_point = pieces[0]
      length = 0
      if options:
        options = options.split(",")
        for i in options:
          if i.startswith("length="):
            length = int(i[7:])
          else:
            print "%s: unknown option \"%s\"" % (mount_point, i)

      d[mount_point] = Partition(mount_point=mount_point, fs_type=pieces[1],
                                 device=pieces[2], length=length,
                                 device2=device2)

  elif fstab_version == 2:
    d = {}
    for line in data.split("\n"):
      line = line.strip()
      if not line or line.startswith("#"):
        continue
      # <src> <mnt_point> <type> <mnt_flags and options> <fs_mgr_flags>
      pieces = line.split()
      if len(pieces) != 5:
        raise ValueError("malformed recovery.fstab line: \"%s\"" % (line,))

      # Ignore entries that are managed by vold
      options = pieces[4]
      if "voldmanaged=" in options:
        continue

      # It's a good line, parse it
      length = 0
      secure_boot = None
      pre_part = None
      options = options.split(",")
      for i in options:
        if i.startswith("length="):
          length = int(i[7:])
        # SPRD: add for secure boot @{
        elif i.startswith("secureboot="):
          sec_option = i[11:]
          sec_options = sec_option.split(":")
          secure_boot = sec_options[0]
          if len(sec_options) > 1:
            pre_part = sec_options[1]
        # @}
        else:
          # Ignore all unknown options in the unified fstab
          continue

      mount_flags = pieces[3]
      # Honor the SELinux context if present.
      context = None
      for i in mount_flags.split(","):
        if i.startswith("context="):
          context = i

      mount_point = pieces[1]
      d[mount_point] = Partition(mount_point=mount_point, fs_type=pieces[2],
                                 device=pieces[0], length=length,
                                 device2=None, context=context,
                                 secure_boot=secure_boot, pre_part=pre_part)

  else:
    raise ValueError("Unknown fstab_version: \"%d\"" % (fstab_version,))

  # / is used for the system mount point when the root directory is included in
  # system. Other areas assume system is always at "/system" so point /system
  # at /.
  if system_root_image:
    assert not d.has_key("/system") and d.has_key("/")
    d["/system"] = d["/"]
  return d

def GetSprdSign():
  if OPTIONS.sprd_sign is None:
    OPTIONS.sprd_sign = sprd_sign.SprdSign(OPTIONS, OPTIONS.single_key)

  return OPTIONS.sprd_sign

def DoSprdSign(sign_type, img_name, img_data, mount_point):
  if OPTIONS.secure_boot:
    temp_dir = tempfile.mkdtemp(prefix="sprd-sign-")
    OPTIONS.tempfiles.append(temp_dir)

    img_path = os.path.join(temp_dir, img_name)
    source_img = open(img_path, "w+b")
    source_img.write(img_data)
    source_img.flush()
    source_img.close()

    secure_img = tempfile.NamedTemporaryFile()

    GetSprdSign().DoSign(sign_type, source_img.name, secure_img.name, mount_point)

    secure_img.seek(os.SEEK_SET, 0)
    data = secure_img.read()

    secure_img.close()

    return data
  else:
    return img_data

def LoadInfoDict(input_dir):
  d = {}
  try:
    with open(os.path.join(input_dir, "sprd_misc_info.txt"), "rb") as f:
      data_sprd_misc = f.read()
      d = common.LoadDictionaryFromLines(data_sprd_misc.split("\n"))
  except IOError, e:
    print "can't find sprd_misc_info.txt!"
  return d

class UpdaterConfig(object):
  def __init__(self, update_type, mount_point, file_name, mount_point2, mount_point3, verbatim, nv_merge, spl_merge):
    print "UpdaterConfig(%s,%s,%s,%s,%s,%s,%s,%s)" % (update_type, mount_point, file_name, mount_point2, mount_point3, verbatim, nv_merge, spl_merge)
    self.update_type = update_type
    self.mount_point = mount_point
    self.file_name = file_name
    if verbatim == "true":
      self.verbatim = True
    else:
      self.verbatim = False
    self.mount_point2 = mount_point2
    self.mount_point3 = mount_point3
    if nv_merge == "default":
      self.nv_merge = ""
    else:
      self.nv_merge = nv_merge
    if spl_merge == "true":
      self.spl_merge = True
    else:
      self.spl_merge = False

def GetAllUpdaterConfigs(input_dir):
  configs = []
  try:
    file = open(os.path.join(input_dir, "OTA/bin/modem.cfg"), "rb")
    config_file = file.read();
    file.close()
    contens = config_file.split('\n')
    print("for begin")
    for line in contens :
      line=line.strip()
      if not line or line.startswith("#"):
        continue
      pieces = line.split(",")
      if (len(pieces) != 8):
        print "invalid line %s" % (line,)
      else:
        print pieces[0],pieces[1],pieces[2],pieces[3],pieces[4],pieces[5],pieces[6],pieces[7]
        config = UpdaterConfig(pieces[0],pieces[1],pieces[2],pieces[3],pieces[4],pieces[5],pieces[6],pieces[7])
        configs.append(config)
    print "close()"

  except IOError, e:
    print "open file %s error!" % (config_file,), str(e)
    raise e

  return configs

class Partition(object):
  def __str__(self):
    return "Partion[" + "mount_point=" + self.mount_point + "]"

class PartitionFile(object):

  def __init__(self, partition, file_name, input_dir):
    self.partition = partition
    self.file_name = file_name
    if file_name is None or input_dir is None:
      if PartitionUpdater.IfNeedImageFile(partition.mount_point):
        raise common.ExternalError("init PartitionFile error")

      return
    self.full_name = os.path.join(input_dir, file_name)
    if os.path.exists(self.full_name):
      file_data = open(self.full_name).read()
      if OPTIONS.secure_boot:
        if partition.secureboot:
          file_data = DoSprdSign(partition.secureboot, file_name, file_data, partition.mount_point)
          print"len of file_data=%d"%len(file_data)
      self.bin = common.File(file_name, file_data)
      self.size = len(file_data)
    else:
      print ("[Warning:] no image file %s" % (self.full_name))
      self.bin = None
      self.size = 0

class PartitionUpdater(object):
  def __init__(self, mount_point, file_name=None, target_ver_dir=None, source_ver_dir=None , mount_point2=None):
    self.mount_point = mount_point
    self.file_name = file_name
    self.inc_flag = False
    self.mount_point2 = mount_point2
    fstab = OPTIONS.info_dict["fstab"]
    if fstab is None:
      raise common.ExternalError("no fstab")
    self.partition = fstab.get(mount_point, None)
    if self.partition is None and mount_point2 is not None:
      self.partition = fstab.get(mount_point2, None)
    if self.partition is None:
      print ("[Warning:] no patition in fstab for mount point %s" % mount_point)
      self.partition = Partition()
      self.partition.mount_point = mount_point
      self.partition.secureboot = None
      print ("[Warning:] auto create patition %s" % self.partition)

    if target_ver_dir is None and PartitionUpdater.IfNeedImageFile(mount_point):
      raise common.ExternalError("init PartitionUpdater error")

class PartitionFullUpdater(PartitionUpdater):
  """Class for recovery full update"""
  def __init__(self, mount_point, file_name=None, input_dir=None ,  mount_point2=None):
    print "PartitionFullUpdater %s, %s" % (mount_point, file_name)
    PartitionUpdater.__init__(self, mount_point, file_name, input_dir,  mount_point2=mount_point2)
    self.input = PartitionFile(self.partition, file_name, input_dir );
    self.target = self.input

def GetBootableImage(name, prebuilt_name, input_dir):
  prebuilt_path = os.path.join(input_dir, name)
  if os.path.exists(prebuilt_path):
    print "find %s..." % (prebuilt_path)
    f = open(prebuilt_path, "rb")
    data = f.read()
    print"secure_boot start"
    if OPTIONS.secure_boot:
      data = DoSprdSign("vlr", prebuilt_name, data, "other")
      print"secure_boot end"
    f.close()
    return common.File(prebuilt_name, data)
  return None

def main(argv):
  args = common.ParseOptions(argv, __doc__)
  input_dir, target_in_dir , target_out_dir = args
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

  def output_sink(input_dir, dir_name, partition_file):
    with open(os.path.join(target_out_dir, dir_name, partition_file.name), "w") as f:
      f.write(partition_file.data)

  OPTIONS.info_dict = LoadInfoDict(input_dir)
  OPTIONS.info_dict["fstab"] = LoadRecoveryFSTab(read_helper, 2)
  OPTIONS.secure_boot = OPTIONS.info_dict.get("secure_boot", False)
  OPTIONS.secure_boot_tool = OPTIONS.info_dict.get("secure_boot_tool", None)
  OPTIONS.single_key = OPTIONS.info_dict.get("single_key", True)

  recovery_img = GetBootableImage("IMAGES/recovery.img", "recovery.img", target_in_dir)
  boot_img = GetBootableImage("IMAGES/boot.img", "boot.img", target_in_dir)
  if boot_img:
    output_sink(target_out_dir, "IMAGES", boot_img)
  if recovery_img:
    output_sink(target_out_dir, "IMAGES", recovery_img)

  if not recovery_img or not boot_img:
    sys.exit(0)

  partitions = []
  configs = GetAllUpdaterConfigs(target_in_dir)
  radio_dir = os.path.join(target_in_dir, "RADIO")
  for config in configs:
    if config.update_type == "B" or config.update_type == "M":
      partition = PartitionFullUpdater(config.mount_point, config.file_name, radio_dir,
                                     mount_point2=config.mount_point2)
      if partition.target.bin:
        output_sink(target_out_dir, "RADIO", partition.target.bin)

if __name__ == '__main__':
  main(sys.argv[1:])
