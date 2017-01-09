#!/usr/bin/perl
######################################################################################################################
# Purpose:   ²éÕÒžø¶šÎÄŒþŒÐ(°üÀš×ÓÎÄŒþŒÐ)ÏÂµÄËùÓÐxmlÎÄŒþ£¬×ª»»ÆäÖÐµÄ&lt;&gt;Îª<>£¬²¢Îªxliff:g ºóÃæµÄ×Ö¶ÎÌíŒÓÒýºÅ
#Òª²éÕÒµÄÎÄŒþŒÐÇëÌîÐŽÔÚµÚ19ÐÐµÄµ¥ÒýºÅÄÚ
#author£ºlei.peng(lei.peng@spreadtrum.com)
######################################################################################################################
use Time::Piece;
use IO::Handle;
use strict;
use warnings;
use utf8;
use Cwd;
use Excel::Writer::XLSX;
use Spreadsheet::XLSX;  
use Spreadsheet::ParseExcel;
use Encode;
use Dumper;
$|=1;
my $PATH ='D:\MUI\Test\tools\MUI\Test\resall';
our @allFileDirectoris;
our $xuhao=0;
#open OUT, ">OUTtmp.txt"||die"error,can not open OUT.txt file\n";

&find_fileindir($PATH);
our @allFileDirectoriesNeedProcess;
my $ind=0;


#ÕÒ³ö·ÇÓ¢ÎÄµÄxml
for(my $t=0;$t<@allFileDirectoris;$t++)
{ 
    
	if (!($allFileDirectoris[$t] =~/.*values\//g))
	{
		$allFileDirectoriesNeedProcess[$ind++]=$allFileDirectoris[$t];
	}
}
foreach my $path (@allFileDirectoriesNeedProcess)
{
#	print "$path\n";
	open SOURCE, '<', "$path" || die "cannot open report file:$!\n";
	my @XmlFileContent = <SOURCE>;
	close SOURCE;
	open TARGET, ">$path"||die"error,can not open OUT.txt file\n";
	for (my $t=0;$t<@XmlFileContent;$t++)
	{
		if (($XmlFileContent[$t]=~s/&(amp;)+lt;/</g) || ($XmlFileContent[$t]=~s/&lt;/</g))
		{
			if (($XmlFileContent[$t]=~s/&(amp;)+gt;/>/g) ||($XmlFileContent[$t]=~s/&gt;/>/g))
			{
				print TARGET "$XmlFileContent[$t]";
				#print "replace\n";
			}
			else
			{
				print "error at $path line:$t\n";
			}
		}
		elsif ($XmlFileContent[$t]=~s/(amp;)+/amp;/g)
		{
				print TARGET "$XmlFileContent[$t]";
				#print "$path\n$t\n";
				#print "multi amp replace!\n";
		}
		else
		{
		  	print TARGET "$XmlFileContent[$t]";
		}
	}
	close TARGET;
	
#	open TARGET2, ">$path"||die"error,can not open OUT.txt file\n";
=pod
	for (my $t=0;$t<@XmlFileContent;$t++)
	{
		if ($XmlFileContent[$t]=~s/<xliff:g id=(.*?) example=(.*?)>(.*?)<\/xliff:g>/<xliff:g id=\"$1\" example=\"$2\">$3<\/xliff:g>/g)
		{
			  print TARGET2 "$XmlFileContent[$t]";
		}
		elsif ($XmlFileContent[$t]=~s/<xliff:g example=(.*?) id=(.*?)>(.*?)<\/xliff:g>/<xliff:g example=\"$1\" id=\"$2\">$3<\/xliff:g>/g)
		{
			  print TARGET2 "$XmlFileContent[$t]";
		}
		elsif ($XmlFileContent[$t]=~s/<xliff:g id=(.*?)>(.*?)<\/xliff:g>/<xliff:g id=\"$1\">$2<\/xliff:g>/g)
		{
			  print TARGET2 "$XmlFileContent[$t]";
		}
		else
		{
		  	print TARGET2 "$XmlFileContent[$t]";
		}
	}
	close TARGET2;
=cut
=pod
  for (my $t=0;$t<@XmlFileContent;$t++)
	{
		if ($XmlFileContent[$t]=~s/<xliff:g id=(.*?)>(.*?)<\/xliff:g>/<xliff:g id=\"$1\">$2<\/xliff:g>/g)
		{
				if ($XmlFileContent[$t]=~s/<xliff:g id=\"(.*?) example=(.*?)\">(.*?)<\/xliff:g>/<xliff:g id=\"$1\" example=\"$2\">$3<\/xliff:g>/g)
				{
						print TARGET2 "$XmlFileContent[$t]";
				}
				else
				{
				  	print TARGET2 "$XmlFileContent[$t]";
				}
		}
		else
		{
		  	print TARGET2 "$XmlFileContent[$t]";
		}
	}
	close TARGET2;
=cut
#convert
	open TARGET3, ">$path"||die"error,can not open OUT.txt file\n";
	for (my $t=0;$t<@XmlFileContent;$t++)
	{
		if ($XmlFileContent[$t]=~/\\\'/g)
		{
				print TARGET3 "$XmlFileContent[$t]";
				print "hahaha  at $path  $t\n";
		}
		elsif ($XmlFileContent[$t]=~s/\'/\\\'/g)
		{
				print TARGET3 "$XmlFileContent[$t]";
				print "heihei  at $path $t\n";
		}
		else
		{
				print TARGET3 "$XmlFileContent[$t]";
		}
	}
	close TARGET3;
#end convert

}


#########################################################################################################
#ÒÔÏÂº¯ÊýÊµÏÖ±éÀúÄ¿ÂŒ²éÕÒÌØ¶šÎÄŒþ ±éÀúÌØ¶šµÄÄ¿ÂŒÏÂËùÓÐµÄÎÄŒþ£š°üÀš×ÓÄ¿ÂŒ£©£¬²¢ÕÒ³öÒÔxmlœáÎ²µÄÎÄŒþÃû³Æ
#ÒÔÏÂ²¿·ÖÇë²»Òª¶¯
#########################################################################################################
sub find_fileindir()
{
  my($dir) = @_;
  my $dirandfile;
  opendir(DIR,"$dir"|| die "can't open this$dir");
  my @files = readdir(DIR);
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
