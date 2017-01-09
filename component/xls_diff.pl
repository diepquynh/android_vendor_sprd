#!/usr/local/bin/perl -w

use File::Basename;
#use strict;
my $LOCAL_PATH;
BEGIN
{
    $LOCAL_PATH = dirname($0);
}
#print "xxx $LOCAL_PATH\n";
use lib "$LOCAL_PATH/../open-source/tools/Spreadsheet";
use lib "$LOCAL_PATH/../open-source/tools/";
require 'ParseExcel.pm';

#****************************************************************************
# Customization Field
#****************************************************************************
my $DebugPrint = "no";
my $xls_txt = "xls_coverted_f";
my $MEMORY_DEVICE_LIST_XLS = "mcp.xls";
#****************************************************************************
# Main Thread Field
#****************************************************************************
if(@ARGV > 0)
{
	$xls_txt=$ARGV[0];
	if($DebugPrint eq "yes"){
		print "ARGV = @ARGV\n";
		print "xls_txt is $xls_txt\n"
	}
}

if(@ARGV > 1)
{
	$MEMORY_DEVICE_LIST_XLS=$ARGV[1];
	if($DebugPrint eq "yes"){
		print "ARGV = @ARGV\n";
		print "xls is $MEMORY_DEVICE_LIST_XLS\n"
	}
}
    &CovertExcelFile_Txt();
    exit 0;


#****************************************************************************
# Subfunction Field
#****************************************************************************
sub CovertExcelFile_Txt
{
	my @all_column=[];
  my $SheetName = "NAND";
  my $parser = Spreadsheet::ParseExcel->new();
  my $Book = $parser->Parse($MEMORY_DEVICE_LIST_XLS);
  my $sheet = $Book->Worksheet($SheetName);
  my $str_line;
  my $tmp;
  my $row;
  my $col;
  my $cell;
  
	$cell=&xls_cell_value($sheet, 0, 1);
	open(FD, ">$xls_txt") or &error_handler("open $xls_txt fail\n", __FILE__, __LINE__);
	for($row = 0,$tmp = &xls_cell_value($sheet, $row, 0);$tmp;$row++,$tmp = &xls_cell_value($sheet, $row, 0)){
		$str_line="";
		for($col = 0;$tmp;$col++,$tmp = &xls_cell_value($sheet, $row, $col)){
			$str_line.="$tmp,";
		}
		print FD "$str_line"."\n\n\n";
	}
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