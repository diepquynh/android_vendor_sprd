#!/usr/bin/python
# v1.0 Base function is ready
# v1.1 Push to remote function enabled
# v1.2 Added Summary function
# v1.21 Polish the clrENV function
#    merge patch_list.diff

import os
import sys
import string
import getopt
import datetime
import csv

SCRIPT_TYPE = 'Merging_Patch'
C_PATH = os.getcwd()
SCRIPT_PATH = os.path.abspath(os.path.dirname(sys.argv[0]))
DREMOTE = None
DBRANCH = None
RENAME = False
TRET = None

#Define patchInfo object array class
class patchinfo:
    def __init__(self, project, patch, status):
        self.project = project
        self.patch = patch
        self.status = status
    def __repr__(self):
        return repr((self.project, self.patch, self.status))

#Return patch object from input_file
class XXTree:
    def __init__(self):
        pass

    def createTree(self, dir, op):
        list = self.getList(dir, 0, op)
        patchList = []

        for i in range(0, len(list)):
            realdir = os.path.realpath(dir) + '/'
            fullpath = os.path.realpath(list[i])
            parpath = os.path.dirname(fullpath)
            projectpath = parpath.replace(realdir, "")
            if os.path.splitext(fullpath)[1][1:] == 'patch' and os.path.isfile(fullpath):
                patchList.append(patchinfo(projectpath, fullpath, 'N/A'))

        return patchList

    def getList(self, dir, layer, op):
        list = []
        if layer == 0: list.append(dir)
        files = os.listdir(dir)
        for file in files:
            file = os.path.join(dir, file)
            if os.path.isdir(file):
                list += self.getList(file, layer + 1, op)
            elif op == '-d':
                pass
            else:
                list.append(file)
        return list

#git push
def gitPush(project, remote, branch):
    for i in range(5):
        cmd = "cd %s;" % project
        cmd += "git push %s HEAD:refs/heads/%s;" % (remote, branch)
        status = os.system(cmd)
        if (status==0):
            print "Success: %s" %cmd
            break
        else:
            print "Retry: %s" %cmd

def clrEnv(project):
    cmd = "cd %s && " % project
    cmd += "git clean -f && "
    cmd += "git reset --hard HEAD && "
    cmd += "git am --abort;"
    status = os.system(cmd)

#summary results
def sumRet(patchList):
    sumret = a = dict()
    for i in patchList:
        if not sumret.has_key(i.project):
            if i.status == "P":
                sumret[i.project] = "1/1"
            elif i.status == "F":
                sumret[i.project] = "0/1"
        else:
            if i.status == "P":
                sumret[i.project] ="%s/%s" % (int(a[i.project].split('/')[0]) + 1, int(a[i.project].split('/')[1]) + 1)
            elif i.status == "F":
                sumret[i.project] ="%s/%s" % (int(a[i.project].split('/')[0]) + 0, int(a[i.project].split('/')[1]) + 1)
        if not sumret.has_key("SUM"):
            if i.status == "P":
                sumret["SUM"] = "1/1"
            elif i.status == "F":
                sumret["SUM"] = "0/1"
        else:
            if i.status == "P":
                sumret["SUM"] ="%s/%s" % (int(a["SUM"].split('/')[0]) + 1, int(a["SUM"].split('/')[1]) + 1)
            elif i.status == "F":
                sumret["SUM"] ="%s/%s" % (int(a["SUM"].split('/')[0]) + 0, int(a["SUM"].split('/')[1]) + 1)
    print "==== Merged & Pushed Status Summary ===="
    print "Total: %s" % sumret["SUM"]
    sumret.pop("SUM")
    for j in sumret.keys():
        print "Project: %s %s" % (j, sumret[j])

#write to csv
def patchToCsv(patchList, outputfile):
    out_csv = csv.writer(open(outputfile, 'wb'))
    head = ['project', 'patches','status']
    out_csv.writerow(head)
    for i in patchList:
        project = i.project
        patch = os.path.relpath(i.patch)
        status = i.status
        out_csv.writerow([project, patch, status])

def run(input_file, output_file):
    global TRET
    #creating patchList
    t = XXTree()
    print "[%s][%s] Start" % (SCRIPT_TYPE, str(datetime.datetime.now()))
    patchList = t.createTree(input_file, op=None)
    patchList = sorted(patchList, key=lambda self: self.patch)
    projectList = []
    #Apply patch
    for i in patchList:
        if projectList.count(i.project) == 0:
            projectList.append(i.project)
        cmd = "cd %s && " % i.project
        cmd += "git am -3 %s;" % i.patch
        print cmd
        status = os.system(cmd)
        if (status==0):
            i.status = 'P'
            l_project = i.project
            m = i.patch + '.merged'
            if RENAME == True:
                os.rename(i.patch, m)
            if DREMOTE and DBRANCH:
                print "publishing out"
                gitPush(i.project, DREMOTE, DBRANCH)
        else:
            i.status = 'F'
            TRET = True
    #clean ENV
    for i in projectList:
        print "clrENV: %s" % i
        clrEnv(i)
    print "[%s][%s] Completed" % (SCRIPT_TYPE, str(datetime.datetime.now()))
    #print patchList
    sumRet(patchList)
    if TRET:
        exit(1)
    else:
        #write to csv
        patchToCsv(patchList, output_file)
        exit(0)

#User help
def usage():
    print "\tmerge.py"
    print "\t       ===[mandatory section]==="
    print "\t      [-i] input patches from {Ref delta patch}"
    print "\t       ===[optional section]==="
    print "\t      [-o] output results file by default <out.csv>"
    print "\t       Only both remote and branch is available, the patch will be push out"
    print "\t      [--remote={remote}]"
    print "\t      [--branch={branch}]"
    print "\t      [--rename] rename the patch if it's on"
    print "\t      [-h] help"

def main(argv):
    input_file = None
    output_file = 'out.csv'
    global DREMOTE
    global DBRANCH
    global RENAME
    try:
        opts, args = getopt.getopt(argv,"i:o:h",["rename","remote=", "branch="])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h"):
            usage()
            sys.exit()
        elif opt in ("-i"):
            input_file = arg
        elif opt in ("-o"):
            output_file = arg
        elif opt in ("--remote"):
            DREMOTE = arg
        elif opt in ("--branch"):
            DBRANCH = arg
        elif opt in ("--rename"):
            RENAME = True

    if not input_file:
        usage()
        sys.exit(2)

    run(input_file, output_file)

if __name__ == "__main__":
    main(sys.argv[1:])

