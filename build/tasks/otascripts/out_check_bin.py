#!/usr/bin/env python
"""
this file to collect info for each file in system dir
the output is check.bin used for root-check
"""
import os
import md5
import sys
import time
import hashlib

ISOTIMEFORMAT = '%Y-%m-%d#%X'
#f_check_name = r"/home6/neilwang/check.bin"
str_to_find = '/system/'

def calcsha1(fobj):
	sha1obj = hashlib.sha1()
	sha1obj.update(fobj.read())
	return sha1obj.hexdigest()

def sha1sum(fname):
	if fname == '-':
		ret = calcsha1(sys.stdin)
	else:
		try:
			f = file(fname,'r')
		except:
			return "0000000000000000000000000000000000000000"
		ret = calcsha1(f)
		f.close()
	return ret


def main(argv):
	if len(argv) != 2:
		print "parameter num should be 2"
		sys.exit(1)

	source_path = argv[0]
	dest_path = argv[1]
	f_check_name = dest_path + '/' + 'check.bin';

	flag = 1
	count = 0
	for root, dirs, files in os.walk(source_path):
		if flag == 1:
			flag = 0
			if os.path.exists(f_check_name):
				print "check file already exists need to be deleted"
				os.remove(f_check_name)
		try:
			fp = open(f_check_name,'a');
		except:
			print "open f_check_file failed"
			return
		compiletime=time.strftime(ISOTIMEFORMAT, time.localtime());
		fileLength = len(files)
		if fileLength != 0:
			count = count + fileLength
			for i in files:
				str_to_be_handled = root+'/'+i
				nPos = str_to_be_handled.index(str_to_find)
				if str_to_be_handled[nPos:].endswith("/Camera2.apk"):
					continue
				fp.write(sha1sum(root+'/'+i)+' '+str_to_be_handled[nPos:]+'\n');
		fp.close();
	print "The number of files under <%s> is: %d" %(source_path,count)

if __name__ == '__main__':
	main(sys.argv[1:])
