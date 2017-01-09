#!/bin/bash

if [ $# = 0 ];then
	echo "for example"
	echo "./xls_diff.sh mcp.xls"
	echo "./xls_diff.sh mcp.xls commit_id"
	echo "./xls_diff.sh mcp.xls commit_id1 commit_id2"
	exit 0
fi
FILE=$1
if [ -d $FILE ];then
	echo $FILE" is not exist"
	exit 0
fi

FILE_1="excel_f_t_1"
FILE_2="excel_f_t_2"
FILE_b="excel_f_t_b"

FILE_COVERTED_1="excel_f_t_c_1"
FILE_COVERTED_2="excel_f_t_c_2"

str=`git log -1 | grep commit`
commit_id_head=${str:7}
#echo $commit_id_head

if [ $# -ge 3 ];then
#./xls_diff.sh mcp.xls commit_id1 commit_id2
mv $FILE $FILE_b
cp $FILE_b $FILE
commit_id_1=$2
commit_id_2=$3
git checkout $commit_id_1 $FILE
cp $FILE $FILE_1
git checkout $commit_id_2 $FILE
cp $FILE $FILE_2
else
if [ $# -ge 2 ];then
#./xls_diff.sh mcp.xls commit_id
mv $FILE $FILE_b
cp $FILE_b $FILE
FILE1=$FILE
cp $FILE $FILE_1
commit_id_1=$2
git checkout $commit_id_1 $FILE
cp $FILE $FILE_2
else
#./xls_diff.sh mcp.xls
mv $FILE $FILE_b
cp $FILE_b $FILE
FILE1=$FILE
commit_id_1=$commit_id_head
cp $FILE $FILE_1
git checkout $commit_id_1 $FILE
cp $FILE $FILE_2
fi
fi

#perl ../open-source/tools/mcp_gen/nandgen.pl $FILE_COVERTED_1 $FILE_1 >/dev/null
#perl ../open-source/tools/mcp_gen/nandgen.pl $FILE_COVERTED_2 $FILE_2 >/dev/null
perl xls_diff.pl $FILE_COVERTED_1 $FILE_1 >/dev/null
perl xls_diff.pl $FILE_COVERTED_2 $FILE_2 >/dev/null
diff -u $FILE_COVERTED_2 $FILE_COVERTED_1
#还原到head状态
git checkout $commit_id_head $FILE
#文件还原,避免日期刷新
mv $FILE_b $FILE
#临时文件都删掉
rm excel_f_t*