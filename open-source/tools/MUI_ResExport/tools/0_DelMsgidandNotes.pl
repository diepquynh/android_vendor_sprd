#!/usr/bin/perl
######################################################################################################################
# Purpose:   查找给定文件夹(包括子文件夹)下的所有xml文件，删除所有的xml文件中的msgid和注释
#需要处理的文件夹请填写在第23行的单引号内。
######################################################################################################################
use Time::Piece;
use IO::Handle;
use strict;
#use warnings;
use utf8;
use Cwd;
use Excel::Writer::XLSX;
use Spreadsheet::XLSX;  
use Spreadsheet::ParseExcel;
use Encode;
$|=1;

our @allFileDirectoris;
our $xuhao=0;

open OUT, ">DeletedNotes.xml"||die"error,can not open DeletedNotes.xml file\n";

&find_fileindir('D:\MUI\Test\resall');
our @allFileDirectoriesNeedProcess;
=pod
my $ind=0;
#找出英文的xml
for(my $t=0;$t<@allFileDirectoris;$t++)
{
	if ($allFileDirectoris[$t] =~/.*values\//g)
	{
		$allFileDirectoriesNeedProcess[$ind++]=$allFileDirectoris[$t];
		#print "$allFileDirectoris[$t]\n";
	}
}
=cut
#foreach my $path (@allFileDirectoriesNeedProcess)
foreach my $path (@allFileDirectoris)
{
	open SOURCE, '<', "$path" || die "cannot open report file:$!\n";
	my @XmlFileContent = <SOURCE>;
	close SOURCE;
	open TARGET, ">$path"||die"error,can not open OUT.txt file\n";
	for (my $t=0;$t<@XmlFileContent;$t++)
	{
		#删除字符串中的msgid
		$XmlFileContent[$t]=~s/ msgid="\d*"//g;
		#删除单行类似<!-- Sound settings screen -->的注释
		if ($XmlFileContent[$t]=~/^\s*<!--(.*)-->\s*$/)
	  {
		  	my $a = $1;
		  	if (!($a =~/-->/))
		  	{
		  	 	 #print "$XmlFileContent[$t]\n";
		  	 	 $XmlFileContent[$t]=~s/^\s*<!--.*-->\s*//g;
		  	}
		}
		#删除单行类似<!-- About --> <skip />的注释
		if ($XmlFileContent[$t]=~/^\s*<!--(.*)-->\s*<skip\s*\/>\s*$/)
	  {
		  	my $a = $1;
		  	if (!($a =~/-->/))
		  	{
		  	 	 #print "$XmlFileContent[$t]\n";
		  	 	 $XmlFileContent[$t]=~s/^\s*<!--(.*)-->\s*<skip\s*\/>\s*$//g;
		  	}
		}
		#删除多行类似<!-- The option in the date-format picker for using the normal format
    #     called for by the user's locale. -->的注释
    if (($XmlFileContent[$t]=~/^\s*<!--(.*)$/)&&(!($XmlFileContent[$t]=~/-->/)))
    {
    	 print OUT $XmlFileContent[$t];
    	 $XmlFileContent[$t]="";
    	 my $s = $t+1;
    	 while(!($XmlFileContent[$s]=~/.*-->\s*$/))
    	 {
    	 	  print OUT $XmlFileContent[$s];
    	 	  $XmlFileContent[$s]="";
    	 	  $s++;
    	 }
    	 print OUT $XmlFileContent[$s];
    	 $XmlFileContent[$s]="";
    	 $t = $s;
    }
    
		print TARGET "$XmlFileContent[$t]";
	}
	close TARGET;
}

#########################################################################################################
#以下函数实现遍历目录查找特定文件 遍历特定的目录下所有的文件（包括子目录），并找出以xml结尾的文件名称
#以下部分请不要动
#########################################################################################################
sub find_fileindir()
{
  my($dir) = @_;
  my $dirandfile;
  opendir(DIR,"$dir"|| die "can't open this$dir");
  my @files =readdir(DIR);
  closedir(DIR);
  for my $file (@files)
  {
    next if($file=~m/\.$/ || $file =~m/\.\.$/);
    if ($file=~/\.(xml)$/i)
    {
					 $dirandfile=$dir."\/".$file;
					 $allFileDirectoris[$xuhao++]=$dirandfile;
    }
    elsif(-d"$dir/$file")
    {
            &find_fileindir("$dir/$file" );
    }
  }
}
#########################################################################################################
