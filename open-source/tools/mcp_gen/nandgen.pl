#!/usr/local/bin/perl -w

use File::Basename;
#use strict;
my $LOCAL_PATH;
BEGIN
{
    $LOCAL_PATH = dirname($0);
}
#print "xxx $LOCAL_PATH\n";
use lib "$LOCAL_PATH/../Spreadsheet";
use lib "$LOCAL_PATH/../";
require 'ParseExcel.pm';

#****************************************************************************
# Customization Field
#****************************************************************************
my $DebugPrint = "no";
my $VersionAndChanglist = "2.0 support autodetect NAND ID and improvement\n";
my @MemoryDeviceList;
my $SPRD_NAND_PARAM_H = "sprd_nand_param.h";
my $MEMORY_DEVICE_LIST_XLS = "vendor/sprd/component/mcp.xls";
my $CONFIG_INI = "";

#****************************************************************************
# Main Thread Field
#****************************************************************************
if(@ARGV > 1)
{
	my $parameters="";
	my $where;
	my $value;
	my $tag;
	for($j = 0;$j<@ARGV;$j++)
	{
		$parameters.=$ARGV[$j];
		$parameters.=" ";
	}
	print "$parameters\n";
	$value = &get_parameter_str($parameters,"-h");
	if(length($value) > 0)
	{
		$SPRD_NAND_PARAM_H = $value;
		print "$SPRD_NAND_PARAM_H\n";
	}
	$value = &get_parameter_str($parameters,"-x");
	if(length($value) > 0)
	{
		$MEMORY_DEVICE_LIST_XLS = $value;
		print "$MEMORY_DEVICE_LIST_XLS\n";
	}
	$value = &get_parameter_str($parameters,"-i");
	if(length($value) > 0)
	{
		$CONFIG_INI = $value;
		print "$CONFIG_INI\n";
		if(! -e $CONFIG_INI){
			#die "$CONFIG_INI not exist\n"
			$CONFIG_INI = "";
		}
	}
}

    &ReadNANDExcelFile();
    &GenNANDHeaderFile();
    if($DebugPrint eq "yes"){
    	print "nandgen done\n";
    }
    exit 0;


#****************************************************************************
# Subfunction Field
#****************************************************************************
sub ReadNANDExcelFile
{
	my @all_column=[];
  my $SheetName = "NAND";
  my $parser = Spreadsheet::ParseExcel->new();
  my $Book = $parser->Parse($MEMORY_DEVICE_LIST_XLS);
  my $sheet = $Book->Worksheet($SheetName);
  my %COLUMN_LIST;
  my $tmp;
  my $row;
  my $col;

  for($col = 0, $row = 0,$tmp = &xls_cell_value($sheet, $row, $col); $tmp; $col++, $tmp = &xls_cell_value($sheet, $row, $col))
  {
  	$COLUMN_LIST{$tmp} = $col;
	}
	@all_column=sort (keys(%COLUMN_LIST));
	if($DebugPrint eq "yes"){
		print "@all_column\n";
	}

	for($row = 1,$tmp = &xls_cell_value($sheet, $row, $COLUMN_LIST{Part_Number});$tmp;$row++,$tmp = &xls_cell_value($sheet, $row, $COLUMN_LIST{Part_Number}))
	{
		foreach $i (@all_column){
			$MemoryDeviceList[$row-1]{$i}=&xls_cell_value($sheet, $row, $COLUMN_LIST{$i});
		}
	}

	if($DebugPrint eq "yes"){
		print "~~~~~~~~~EXCEL INFO~~~~~~~~~~~\n";
		for($index=0;$index<@MemoryDeviceList;$index++){
			print "index:$index\n";
			foreach $i (@all_column){
				printf ("%-15s:%-20s ",$i,$MemoryDeviceList[$index]->{$i});
			}
			print "\n";
		}
		print "~~~~~~~~~There are $index Nand Chips~~~~~~~~~~~\n";
	}
}

sub GenNANDHeaderFile()
{
	my %InFileChip;
	my %ChipParamters;
	my $Longest_ID=0;
	my $Chip_count=0;
	my %maker_table_hash;
	my %device_table_hash;
	my $device_table_num=0;
	my @device_table;
	my @p_list;
	my $SpareSize_B_2;
	my $Type0;
	my $Type1;
	my $timing_ace_ns;
	my $timing_rwl_ns;
	my $timing_rwh_ns;

	if(-e $SPRD_NAND_PARAM_H)
	{
		`chmod 777 $SPRD_NAND_PARAM_H`;
	}

	if(length($CONFIG_INI) > 0){
		open(FD, "<$CONFIG_INI");
		@p_list=<FD>;
		close FD;
	}

	for($iter=0;$iter<@MemoryDeviceList;$iter++){
		my $ID_length=0;
		my $ID=$MemoryDeviceList[$iter]->{Nand_ID};
		if(!exists($ChipParamters{$ID})){
			my $partnumber = $MemoryDeviceList[$iter]->{Part_Number};
			if(!&partnumber_is_exist($partnumber,@p_list)){
				print "The chip:$ID $partnumber not selected.\n";
			}
			elsif(length($ID)%2){
				print "The chip:$ID have wrong number,",length($ID),".\n";
			}
			else{
				$ID_length=length($ID)/2-1;
				if($ID_length > $Longest_ID){
					$Longest_ID = $ID_length;
				}
				#print "\$Longest_ID=$Longest_ID\n";
				$InFileChip{$iter}={'ID'=>$ID,'IDLength'=>$ID_length};
				$ChipParamters{$ID}={'BlockSize_KB'=>$MemoryDeviceList[$iter]->{BlockSize_KB},'BlockNum'=>$MemoryDeviceList[$iter]->{BlockNum},
					'PageSize_KB'=>$MemoryDeviceList[$iter]->{PageSize_KB},'SectSize_B'=>$MemoryDeviceList[$iter]->{SectSize_B},
					'BusWidth'=>$MemoryDeviceList[$iter]->{BusWidth},
					'Cycles'=>$MemoryDeviceList[$iter]->{Cycles},'spareSize_B'=>$MemoryDeviceList[$iter]->{spareSize_B},
					'oob_nEccBits_b'=>$MemoryDeviceList[$iter]->{oob_nEccBits_b},'oob_nEccPos_B'=>$MemoryDeviceList[$iter]->{oob_nEccPos_B},
					'oob_nEccSize_B'=>$MemoryDeviceList[$iter]->{oob_nEccSize_B},'oob_nInfoPos_B'=>$MemoryDeviceList[$iter]->{oob_nInfoPos_B},
					'oob_nInfoSize_B'=>$MemoryDeviceList[$iter]->{oob_nInfoSize_B}};
				$Chip_count++;
			}
		}#	if(!exists($ChipParamters{$ID}))
		else{#if ID is same,check if parameters is same.
			if($ChipParamters{$ID}->{BlockSize_KB} == $MemoryDeviceList[$iter]->{BlockSize_KB}
			&& $ChipParamters{$ID}->{BlockNum} == $MemoryDeviceList[$iter]->{BlockNum}
			&& $ChipParamters{$ID}->{PageSize_KB} == $MemoryDeviceList[$iter]->{PageSize_KB}
			&& $ChipParamters{$ID}->{SectSize_B} == $MemoryDeviceList[$iter]->{SectSize_B}
			&& $ChipParamters{$ID}->{BusWidth} eq $MemoryDeviceList[$iter]->{BusWidth}
			&& $ChipParamters{$ID}->{spareSize_B} eq $MemoryDeviceList[$iter]->{spareSize_B}
			&& $ChipParamters{$ID}->{oob_nEccBits_b} eq $MemoryDeviceList[$iter]->{oob_nEccBits_b}
			&& $ChipParamters{$ID}->{oob_nEccPos_B} eq $MemoryDeviceList[$iter]->{oob_nEccPos_B}
			&& $ChipParamters{$ID}->{oob_nEccSize_B} eq $MemoryDeviceList[$iter]->{oob_nEccSize_B}
			&& $ChipParamters{$ID}->{oob_nInfoPos_B} eq $MemoryDeviceList[$iter]->{oob_nInfoPos_B}
			&& $ChipParamters{$ID}->{oob_nInfoSize_B} eq $MemoryDeviceList[$iter]->{oob_nInfoSize_B}){
				print "There more than 1 chip have the ID:$MemoryDeviceList[$iter]->{Nand_ID},bypass it\n";
			}
			else{
				print "$ChipParamters{$ID}->{BlockSize_KB},$ChipParamters{$ID}->{BlockNum},$ChipParamters{$ID}->{PageSize_KB},$ChipParamters{$ID}->{SectSize_B},$ChipParamters{$ID}->{BusWidth}";
				print ",$ChipParamters{$ID}->{spareSize_B},$ChipParamters{$ID}->{oob_nEccBits_b},$ChipParamters{$ID}->{oob_nEccPos_B},$ChipParamters{$ID}->{oob_nEccSize_B},$ChipParamters{$ID}->{oob_nInfoPos_B},$ChipParamters{$ID}->{oob_nInfoSize_B}\n";
				print "$MemoryDeviceList[$iter]->{BlockSize_KB},$MemoryDeviceList[$iter]->{BlockNum},$MemoryDeviceList[$iter]->{PageSize_KB},$MemoryDeviceList[$iter]->{SectSize_B},$MemoryDeviceList[$iter]->{BusWidth}";
				print ",$MemoryDeviceList[$iter]->{spareSize_B},$MemoryDeviceList[$iter]->{oob_nEccBits_b},$MemoryDeviceList[$iter]->{oob_nEccPos_B},$MemoryDeviceList[$iter]->{oob_nEccSize_B},$MemoryDeviceList[$iter]->{oob_nInfoPos_B},$MemoryDeviceList[$iter]->{oob_nInfoSize_B}\n";
				die "There more than 1 chip have the ID:$MemoryDeviceList[$iter]->{Nand_ID},but paramters is not same,you should modify the excel!!!!\n";
			}
		}
	}
	print "Chip_count=$Chip_count\n";
	open(FD, ">$SPRD_NAND_PARAM_H") or &error_handler("open $SPRD_NAND_PARAM_H fail\n", __FILE__, __LINE__);
	print FD "\n#ifndef __SPRD_NAND_PARAM_H__\n#define __SPRD_NAND_PARAM_H__\n\n";
	print FD &sprd_nand_param_h_head();


	foreach $it (sort by_index (keys(%InFileChip))){
		my $ID=$InFileChip{$it}->{ID};
		#creat ID arry string
		my @ID_arry=($ID =~ m/([\dA-Fa-f]{2})/gs);
		my $arry_str="{";
		for($i=0;$i<$Longest_ID;$i++){
			if($i<@ID_arry){
				$arry_str.="0x$ID_arry[$i]";
			}else{
				$arry_str.="0x00";
			}
			if($i<$Longest_ID-1){
				$arry_str.=",";
			}
		}
		$maker_table_hash{$ID_arry[0]}=$MemoryDeviceList[$it]->{Vendor};
		$device_table_hash{$ID}={'ID_vendor'=>$ID_arry[0],'ID_type'=>$ID_arry[1],'Vendor'=>$MemoryDeviceList[$it]->{Vendor},'Part_Number'=>$MemoryDeviceList[$it]->{Part_Number},'BusWidth'=>$MemoryDeviceList[$it]->{BusWidth},'Type2'=>$MemoryDeviceList[$it]->{Type2},'BlockSize_KB'=>$MemoryDeviceList[$it]->{BlockSize_KB},'BlockNum'=>$MemoryDeviceList[$it]->{BlockNum}};
	}
	foreach $ID (keys(%device_table_hash)){
		my $find=0;
		for($i=0;$i<$device_table_num;$i++){
			#print "$device_table_hash{$ID}->{Vendor} $device_table[$i]\n";
			if($device_table_hash{$ID}->{Vendor} eq $device_table[$i]){
				$find=1;
				last;
			}
		}
		if($find == 1){
			next;
		}
		$device_table[$device_table_num]=$device_table_hash{$ID}->{Vendor};
		#print "$device_table[$device_table_num]\n";
		$device_table_num++;
	}

	for($i=0;$i<@device_table;$i++){
		my $l_maker=lc($device_table[$i]);
		print FD "struct sprd_nand_device "."$l_maker"."_device_table[] = {\n";
		foreach $ID (keys(%device_table_hash)){
			if($device_table_hash{$ID}->{Vendor} eq $device_table[$i]){
				$Type0 = $device_table_hash{$ID}->{BlockSize_KB}*$device_table_hash{$ID}->{BlockNum}*8/(1024*1024);
				$Type1 = substr($device_table_hash{$ID}->{BusWidth},3);
				$Type1 = "x".$Type1;
				print FD "\t{0x$device_table_hash{$ID}->{ID_type},\"$device_table_hash{$ID}->{Part_Number}\",\"$Type0"."Gb,$Type1,$device_table_hash{$ID}->{Type2}\"},\n";
			}
		}
		print FD "\t{0x00,NULL,NULL}\n";
		print FD "};\n\n";
	}

	print FD "struct sprd_nand_maker maker_table[] = {\n";
	foreach $ID_vendor (keys(%maker_table_hash)){
		my $l_maker=lc($maker_table_hash{$ID_vendor});
		print FD "\t{0x$ID_vendor,\"$maker_table_hash{$ID_vendor}\","."$l_maker"."_device_table},\n";
	}
	print FD "\t{0x00,NULL,NULL}\n";
	print FD "};\n\n";

	print FD "struct sprd_nand_param sprd_nand_param_table[] = {\n";
	foreach $it (sort by_index (keys(%InFileChip))){
		my $ID=$InFileChip{$it}->{ID};
		#creat ID arry string
		my @ID_arry=($ID =~ m/([\dA-Fa-f]{2})/gs);
		my $arry_str="{";
		for($i=0;$i<$Longest_ID;$i++){
			if($i<@ID_arry){
				$arry_str.="0x$ID_arry[$i]";
			}else{
				$arry_str.="0x00";
			}
			if($i<$Longest_ID-1){
				$arry_str.=",";
			}
		}
		$arry_str.="}";
		if($DebugPrint eq "yes"){
			print "ID=$arry_str\n";
		}
		#print string to file
		$SpareSize_B_2 = $MemoryDeviceList[$it]->{PageSize_KB}*1024/$MemoryDeviceList[$it]->{SectSize_B}*$MemoryDeviceList[$it]->{spareSize_B};
		$timing_ace_ns = ($MemoryDeviceList[$it]->{timing_T_clh} > $MemoryDeviceList[$it]->{timing_T_alh}) ? $MemoryDeviceList[$it]->{timing_T_clh}:$MemoryDeviceList[$it]->{timing_T_alh};
		$timing_rwl_ns = $MemoryDeviceList[$it]->{timing_T_wp};
		$timing_rwh_ns = $MemoryDeviceList[$it]->{timing_T_wh};
		print FD "\t{//$MemoryDeviceList[$it]->{Vendor}\n\t\t$arry_str,0x$ID_arry[0],0x$ID_arry[1], ";
		print FD "\n\t\tSZ_K_BLOCK($MemoryDeviceList[$it]->{BlockSize_KB}),NUM_BLOCK($MemoryDeviceList[$it]->{BlockNum}), SZ_K_PAGE($MemoryDeviceList[$it]->{PageSize_KB}),SZ_B_SECTOR($MemoryDeviceList[$it]->{SectSize_B}),SZ_B_SPARE($SpareSize_B_2),$MemoryDeviceList[$it]->{BusWidth},$MemoryDeviceList[$it]->{Cycles},";
		print FD "\n\t\t{$timing_ace_ns,$timing_rwl_ns,$timing_rwh_ns},";
		print FD "\n\t\t{SZ_B_OOB($MemoryDeviceList[$it]->{spareSize_B}),ECC_BITS($MemoryDeviceList[$it]->{oob_nEccBits_b}),POS_ECC($MemoryDeviceList[$it]->{oob_nEccPos_B}),SZ_ECC($MemoryDeviceList[$it]->{oob_nEccSize_B}),POS_INFO($MemoryDeviceList[$it]->{oob_nInfoPos_B}),SZ_INFO($MemoryDeviceList[$it]->{oob_nInfoSize_B})}";
		print FD "\n\t},\n";
	}

	print FD "\t{\n\t\t{0x00, 0x00, 0x00, 0x00, 0x00},0x00,0x00, ";
	print FD "\n\t\tSZ_K_BLOCK(256),NUM_BLOCK(2048), SZ_K_PAGE(4),SZ_B_SECTOR(512),SZ_B_SPARE(128),0x01,0x05,";
	print FD "\n\t\t{10,21,15},";
	print FD "\n\t\t{SZ_B_OOB(16),ECC_BITS(8),POS_ECC(2),SZ_ECC(14),POS_INFO(0),SZ_INFO(0)}";
	print FD "\n\t}\n";
  print FD "};\n\n";

  print FD &sprd_nand_param_h_fuction();
	print FD "#endif\n";
  close FD;
}


#****************************************************************************************
# subroutine:  xls_cell_value
# return:      Excel cell value no matter it's in merge area or not, and in windows or not
# input:       $Sheet:  Specified Excel Sheet
# input:       $row:    Specified row number
# input:       $col:    Specified column number
#****************************************************************************************
sub xls_cell_value()
{
    my($Sheet, $row, $col) = @_;
    my $cell = $Sheet->get_cell($row, $col);
    if (defined $cell)
    {
        return $cell->Value();
    } else
    {
        print "$Sheet: row=$row, col=$col undefined\n";
        return;
    }
}

sub partnumber_is_exist()
{
	my($partnumber, @p_list) = @_;
	my $where;
	#print "$partnumber\n";
	if(@p_list < 1){
		#no mcp_config.ini,so select all
		return 1;
	}
	foreach $it (@p_list){
		chomp($it);
		$it =~ s/^\s+//g;
		$where = index($it,"\#");
		if($where == 0){
			#print "bypass $it\n";
		}
		else{
			if($partnumber eq $it){
				return 1;
			}
		}
	}
	return 0;
}

sub get_parameter_str()
{
		my($parameters, $tag) = @_;
		my $where;
		my $ret;
		$where=index($parameters,$tag);
		if($where >= 0)
		{
			#bypass "-s ","-h ",and etc
			$ret=substr($parameters,$where+3);
			$where=index($ret," ");
			if($where >= 0)
			{
				$ret=substr($ret,0,$where);
			}
			return $ret;
		}
		return "";
}

#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler()
{
    my($error_msg, $file, $line_no) = @_;
    my $final_error_msg = "NandGen ERROR: $error_msg at $file line $line_no\n";
    print $final_error_msg;
    die $final_error_msg;
}

sub by_index
{$a<=>$b}

sub sprd_nand_param_h_head()
{
    my $template = <<"__TEMPLATE";
#define NAND_MAX_ID_LEN 5

#define BW_08 0x00
#define BW_16 0x01

#define CYCLES_3 0x03
#define CYCLES_4 0x04
#define CYCLES_5 0x05

#define NUM_BLOCK(n) (n)

#define SZ_K_BLOCK(n) (n * 1024)
#define SZ_K_PAGE(n) (n * 1024)

#define SZ_B_BLOCK(n) (n)
#define SZ_B_PAGE(n) (n)

#define SZ_B_SPARE(n) (n)
#define SZ_B_OOB(n) (n)
#define SZ_B_SECTOR(n) (n)

#define ECC_BITS(n) (n)

#define SZ_ECC(n) (n)
#define POS_ECC(n) (n)
#define SZ_INFO(n) (n)
#define POS_INFO(n) (n)

#define CALC_ECC_SIZE(n) ((14 * (n) + 7) / 8)


struct sprd_nand_device {
	uint8_t idDevice;
	char* pName;
	char* pType;
};

struct sprd_nand_maker {
	uint8_t idMaker;
	char* pName;
	struct sprd_nand_device* pDevTab;
};

struct sprd_nand_oob {
	uint16_t nOOBSize; //16
	uint8_t nEccBits;
	uint16_t nEccPos;
	uint16_t nEccSize;  //4
	uint16_t nInfoPos;
	uint16_t nInfoSize;
};

struct sprd_nand_timing {
	uint8_t	ace_ns;	/* ALE, CLE end of delay timing, unit: ns */
	uint8_t	rwl_ns;	/* WE, RE, IO, pulse  timing, unit: ns */
	uint8_t	rwh_ns;	/* WE, RE, IO, high hold  timing, unit: ns */
};

struct sprd_nand_param {
	uint8_t id[NAND_MAX_ID_LEN];
	uint8_t idMaker;
	uint8_t idDevice;
	uint32_t nBlkSize; //256K
	uint32_t nBlkNum; //
	uint32_t nPageSize; //4K
	uint16_t nSecSize; //512
	uint16_t nSpareSize;
	uint8_t nBusWidth;
	uint8_t nCycles;
	struct sprd_nand_timing sTiming;
	struct sprd_nand_oob sOOB;
};

__TEMPLATE

   return $template;
}

sub sprd_nand_param_h_fuction()
{
    my $template_fuction = <<"__TEMPLATE";
STATIC_FUNC struct sprd_nand_maker* __FindMaker(uint8_t idMaker) {
	struct sprd_nand_maker* pmaker = maker_table;

	while (pmaker->idMaker != 0) {
		if(pmaker->idMaker == idMaker) {
			return pmaker;
		}
		pmaker++;
	}
	return NULL;
}

STATIC_FUNC struct sprd_nand_device* __FindDevice(struct sprd_nand_maker* pmaker, uint8_t idDevice) {
	struct sprd_nand_device* pdevice = pmaker->pDevTab;

	while (pdevice->idDevice != 0) {
		if(pdevice->idDevice == idDevice) {
			return pdevice;
		}
		pdevice++;
	}
	return NULL;
}

STATIC_FUNC void __PrintNandInfo(struct sprd_nand_param* p) {
	struct sprd_nand_device* pdevice;
	struct sprd_nand_maker* pmaker;

	DPRINT("%s\\n", __func__);

	pmaker = __FindMaker(p->idMaker);
	pdevice = __FindDevice(pmaker, p->idDevice);
	
	DPRINT("device is %s:%s\\n", pmaker->pName, pdevice->pName);
	DPRINT("block size is %d\\n", p->nBlkSize);
	DPRINT("page size is %d\\n", p->nPageSize);
	DPRINT("spare size is %d\\n", p->nSpareSize);
	DPRINT("eccbits is %d\\n", p->sOOB.nEccBits);
}


STATIC_FUNC void __ParseNandParam(uint8_t* id) {
	struct sprd_nand_param* param = sprd_nand_param_table;

	while(param->idMaker != 0) {
		if ((param->id[0] == id[0]) && 
			(param->id[4] == id[1])) {
			
			__PrintNandInfo(param);
			return param;
		}
		param++;
	}
	
	return;
}

STATIC_FUNC int __SprdCheckNancParam(struct sprd_nand_param* param) {
	struct sprd_nand_device* pdevice;
	struct sprd_nand_maker* pmaker;
	struct sprd_nand_oob* poob = &param->sOOB;
	uint8_t sector_num = param->nPageSize / param->nSecSize;

	DPRINT("%s\\n", __func__);

	if (param->idMaker != param->id[0]) {
		DPRINT("%s: id is not match!!!!!!!!!!!!!!!\\n", __func__);
		SPRD_ASSERT(0);
	}
	if (param->idDevice != param->id[1]) {
		DPRINT("%s: id is not match!!!!!!!!!!!!!!!\\n", __func__);
		SPRD_ASSERT(0);
	}
	if (poob->nOOBSize * sector_num > param->nSpareSize) {
		DPRINT("%s: OOB size is not match!!!!!!!!!!!!!!!\\n", __func__);
		SPRD_ASSERT(0);
	}
	if (poob->nEccPos + poob->nEccSize > poob->nOOBSize) {
		DPRINT("%s: ECC size exceeds the oob!!!!!!!!!!!!!!!\\n", __func__);
		SPRD_ASSERT(0);
	}
	if (CALC_ECC_SIZE(poob->nEccBits) != poob->nEccSize) {
		DPRINT("%s: ECC size is wrong!!!!!!!!!!!!!!!\\n", __func__);
		SPRD_ASSERT(0);
	}
	if (poob->nInfoPos + poob->nInfoSize != poob->nEccPos) {
		DPRINT("%s: Info position is wrong!!!!!!!!!!!!!!!\\n", __func__);
		SPRD_ASSERT(0);
	}	

	return 0;	
}

struct sprd_nand_param* SprdGetNandParam(uint8_t* id) {
	struct sprd_nand_param* param = sprd_nand_param_table;
	int i;

	DPRINT("%s\\n", __func__);

	for (i = 0; i < 5; i++) {
		DPRINT("id[%d] is %x\\n", i, id[i]);
	}

	//__ParseNandParam(id);

	while(param->idMaker != 0) {
		if ((param->id[0] == id[0]) && 
			(param->id[1] == id[1]) &&
			(param->id[2] == id[2]) &&
			(param->id[3] == id[3]) &&
			(param->id[4] == id[4])) {

			__PrintNandInfo(param);
			__SprdCheckNancParam(param);
			return param;
		}
		param++;
	}
	DPRINT("Nand params unconfigured, please check it. Halt on booting!!!\\n");
	for(;;) /*relax*/;

	return NULL;
}
__TEMPLATE

   return $template_fuction;
}
