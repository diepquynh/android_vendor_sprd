#!/usr/bin/env python
import sys
import copy
import errno
import os
import re
import sha
import subprocess
import tempfile
import time
import zipfile
import imp

sys.path.append('./build/tools/releasetools')


def main(argv):
  if len(argv) != 4:
    print __doc__
    sys.exit(1)

  in_dir = argv[0]
  glob_dict_file = argv[1]
  out_file = argv[2]
  target_out = argv[3]
  fp, pathname, desc = imp.find_module('build_image', ['./build/tools/releasetools',])
  module = imp.load_module('build_image', fp, pathname, desc)

  glob_dict = getattr(module, 'LoadGlobalDict')(glob_dict_file)
  image_properties = {}
  if "mount_point" in glob_dict:
    # The caller knows the mount point and provides a dictionay needed by
    # BuildImage().
    image_properties = glob_dict
  else:
    image_filename = os.path.basename(out_file)
    mount_point = ""
    if image_filename == "sysinfo.img":
      mount_point = "systeminfo"
      image_properties["mount_point"] = mount_point

    elif image_filename == "persist.img":
      mount_point = "persist"
    else:
      print >> sys.stderr, "error: unknown image file name ", image_filename
      exit(1)

    def copy_prop(src_p, dest_p):
      if src_p in glob_dict:
        image_properties[dest_p] = str(glob_dict[src_p])

    if mount_point == "systeminfo":
      copy_prop("sysinfo_fs_type", "fs_type")
      copy_prop("sysinfo_size", "partition_size")
      copy_prop("sysinfo_extfs_sparse_flag", "extfs_sparse_flag")

  if not getattr(module, 'BuildImage')(in_dir, image_properties, out_file, target_out):
    print >> sys.stderr, "error: failed to build %s from %s" % (out_file, in_dir)
    exit(1)

if __name__ == '__main__':
  main(sys.argv[1:])
