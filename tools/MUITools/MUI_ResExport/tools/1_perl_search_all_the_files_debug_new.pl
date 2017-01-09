#!/usr/bin/perl
############################################################################################
# Purpose:   查找给定文件夹(包括子文件夹)下的所有xml文件，并统计其中的字符串，分语言写入
# 到excel中，每种语言一个sheet。需要查找的文件夹路径填写在第24行相应的位置；需要翻译的语言
#填写在第63行的数组里面，该数组可以自行修改，注意，这里填写的是在安卓源代码中相应语言xml文
#件的上一级文件夹名称
############################################################################################
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

#待查找的xml路径填写到如下一行括号的单引号里面
&find_fileindir('D:\MUI\Test\resall');
my $i=0;
#print @allFileDirectoris;
my $row=2;

###########################################################################################
#写入到一个新excel的子程序，三个参数分别为（value，行数，列数）
#在写入之前需要设置$xlBook和$xlSheet1
###########################################################################################
my $xlBook0 = Excel::Writer::XLSX->new("AllTheFilesPath.xlsx") || die "$!\n";
my $xlSheet0 = $xlBook0->add_worksheet("MUI");
my $bodyFormat0 = $xlBook0->add_format(
    color       => 'black',
     border    => 1,
    align       => 'right',
    font        => '宋体'
   );
sub writetoexcel(){
 my (@canshu) = @_;
    $xlSheet0->write(
    $canshu[2].$canshu[1],
    decode('utf8',"$canshu[0]"), $bodyFormat0);
}

foreach my $path (@allFileDirectoris) {
	&writetoexcel($path,$row,'B');
	$row++;
}
$xlBook0->close();

########################################################################################################################
#实现从得到的所有xml文件路径中匹配出英文，中文及任一语言的xml路径名，保存在数组@englanguage,@zhlanguage,@samelanguage中
########################################################################################################################
my $currentpath = getcwd;
open OUT, ">$currentpath\\tmpOUT.txt"||die"error,can not open OUT.txt file\n";
open OUTSAMELANGUAGE, ">$currentpath\\tmpOUTSAMELANGUAGE.txt"||die"error,can not open OUTSAMELANGUAGE.txt file\n";
#@languages是除了中英文外的所有国家或者地区的语言列表，如果需要新增或者删除需要翻译的语言，请修改该数组
my $englang = "values";
my $zhlang = "values-zh-rCN";
my @languages = ("values-kk");
my $onelanguage;
my @englanguage;
my $tempeng = 0;
foreach my $path (@allFileDirectoris)
{
	my $text = $path;
	if($text=~/.*\/$englang\/.*/i)
	{
		$englanguage[$tempeng++]=$path;
	}
}
my @zhlanguage;
my $tempzh = 0;
foreach my $path (@allFileDirectoris)
{
	my $text = $path;
	if($text=~/.*\/$zhlang\/.*/i)
	{
		$zhlanguage[$tempzh++]=$path;
	}
}
#以下数组用于保存英文、中文及多国语言的各列
my @arrayEA;		my @arrayCA;	#	my @arrayOA;
my @arrayEB;		my @arrayCB;	#	my @arrayOB;
my @arrayEC;		my @arrayCC;	#	my @arrayOC;
my @arrayED;		my @arrayCD;	#	my @arrayOD;
my @arrayEE;		my @arrayCE;	#	my @arrayOE;
my @arrayEF;		my @arrayCF;	#	my @arrayOF;
my @arrayEG;		my @arrayCG;	#	my @arrayOG;
my @arrayEH;		my @arrayCH;	#	my @arrayOH;
my @arrayEI;		my @arrayCI;	#	my @arrayOI;
my @arrayEJ;		my @arrayCJ;	#	my @arrayOJ;
		
#保存英文的情况，到数组@arrayEA；@arrayEB；@arrayEC；@arrayED；@arrayEE；@arrayEF；@arrayEG；@arrayEH；@arrayEI
my $linerow = 0;		#用于保存数组的行号
my $temp = 0;
for ($temp=0;$temp<@englanguage;$temp++)
{		#以下对某一xml进行操作，只需要注意$allsamelanguagerow这个变量即可
		my $path = $englanguage[$temp];
		#打开该path，读取strings到excel表的某一sheet
		open(FILENAME,"$path")||die"error,can not open xmlfile\n";
		my @xmlcontent = <FILENAME>;
		#print OUT @xmlcontent;
		my $i=0;
		my $line;
		for($i=0;$i<@xmlcontent;$i++)
		{
			$line = $xmlcontent[$i];
	#		$line =~s/\n//;
	#		$line =~ s/^\s+//;
	#		$line =~ s/\s+$//;
			#删除前后空格及回车
			$line =~s/\s+/ /;
#			$line =~ s/\n\r/ /sgi;
#			$line =~ s/\r\n/ /sgi;
#			$line =~ s/\n/ /sgi;
			$line =~ s/^\s+//;
			$line =~ s/\s+$//;
			#注释不参与统计
			if ($line =~ /^<!--.*-->$/)
			{
					print OUTSAMELANGUAGE "$path ";print OUTSAMELANGUAGE "$i";print OUTSAMELANGUAGE "\n";
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
		#			$line =~ s/\n\r/ /sgi;
		#			$line =~ s/\r\n/ /sgi;
		#			$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
			}
			if ($line =~ /^<!--$/)
			{
		#			print OUTSAMELANGUAGE " \nhahaha ";print OUTSAMELANGUAGE "$path ";print OUTSAMELANGUAGE "$i";print OUTSAMELANGUAGE "\n";
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
		#			$line =~ s/\n\r/ /sgi;
		#			$line =~ s/\r\n/ /sgi;
		#			$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					my $x=0;
					while((not($line=~/^(-->\s*<skip \/>|-->)$/))&&($x<100))
					{
		#					print OUTSAMELANGUAGE " o ";
							$x++;
							$i++;
							$line = $xmlcontent[$i];
							#删除前后空格及回车
							$line =~s/\s+/ /;
		#					$line =~ s/\n\r/ /sgi;
		#					$line =~ s/\r\n/ /sgi;
		#					$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
					}
		#			print OUTSAMELANGUAGE "$i";
					if($x==100)
					{
		#					print OUTSAMELANGUAGE "plerror!!!!!!!";
							print "plerror!please contact lei.peng 589-149";
					}
			}
			
			#[A](1)以下片段处理如下情况：<string name="ringtone_summary"/>
			if ($line =~ /<string\s+name\s*="(?<string_name>.*?)"\s*\/>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			#[B](2)以下处理常规情况的strings，eg：常规情况1：<string name="name">value</string>
			#和常规情况2：<string name="searchview_description_clear" msgid="1330281990951833033">"Απαλοιφ? ερωτ?ματο?"</string>
			#和常规情况3：string name带product的：product在name后面
			#和常规情况4：<string name="web_user_agent_target_content" translatable="false">"Mobile "</string>
			elsif ($line =~ /<string\s+name\s*=\s*"(?<string_name>.*?)"\s*>\s*(?<string_value>.*?)\s*<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name ;
				my $translatable ;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayEI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				elsif ($string_name =~ /(?<string_name>.*?)"\s+translate="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			#[C](3)以下处理translatable在name前面的情况
			elsif($line =~ /<string\s+translatable="(?<translatable>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $translatable = $+{translatable};
				$translatable = "T".$translatable;
				$arrayEJ[$linerow] = $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			#[C](3-1)以下处理translate在name前面的情况
			#<string translate="false" name="sin_mathematical_value">sin</string>
			elsif($line =~ /<string\s+translate="(?<translatable>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $translatable = $+{translatable};
				$translatable = "T".$translatable;
				$arrayEJ[$linerow] = $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			#[D](4)以下片段处理如下常规情况：
			# <string product="nosdcard" name="no_storage_z788">"使用相机前请先装载USB存储设备,您可以先切换到内部存储空间保存少量照片。"</string>
			elsif ($line =~ /<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name = $+{product_name};
				my $translatable ;
				$arrayEI[$linerow] = $product_name;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				elsif ($string_name =~ /(?<string_name>.*?)"\s+translate="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			
			#[E](5)以下处理常规情况5：eg：<string msgid="1330281990951833033" name="searchview_description_clear">"Απαλοιφ? ερωτ?ματο?"</string>
			#和name后有product的情况
			elsif ($line =~ /<string\s+msgid="\d*"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name;
				if($string_name =~ /(?<string_name>.*?)"\sproduct="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayEI[$linerow] = $product_name;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			#[F](6)和常规情况7：<string msgid="4673411327373419641" product="nosdcard" name="sd_mount_summary"/>
			elsif ($line =~ /<string\s+msgid="\d*"\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)"\/>$/sgi)
			{
				my $product_name = $+{product_name};
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$arrayEI[$linerow] = $product_name;
				$linerow++;
			}
			#[G](7)和常规情况6：<string msgid="5996407024898469862" product="default" name="data_usage_limit_dialog">"达到指定的数据使用上限时，系统会停用您的<xliff:g id="NETWORKS">%1$s</xliff:g>数据连接。"\n\n"由于数据流量是由您的手机计算而得的，您的运营商对于流量的计算方法可能有所不同，因此请考虑设置一个较低的上限。"</string>
			elsif ($line =~ /<string msgid="\d*" product="(?<product_name>.*?)" name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $product_name = $+{product_name};
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$arrayEI[$linerow] = $product_name;
				$linerow++;
			}
			#[H](8)以下处理常规情况7：<string msgid="7345931695292850058" name="show_password_summary"/>
			elsif ($line =~/<string msgid="\d*" name="(?<string_name>.*?)"\/>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			
			#[I](9)###以下片段为复数1、复数2和复数3的情况
			#复数1
			# <plurals name="Nhours">
			#		 <!-- This is the label for a 1-hour reminder. -->
		  #    <item quantity="one">1 hour</item>
		  #    <!-- This is the label for a reminder of 2 or more hours. The actual number of
		  #         hours is a parameter. -->
		  #    <item quantity="other"><xliff:g id="count">%d</xliff:g> hours</item>
		  #</plurals>
			#复数2
		  # <plurals name="Nsongs">
		  #    <item quantity="other"><xliff:g id="count">%d</xliff:g> songs</item>
		  #</plurals>
		  #复数3
		  #<plurals name="listFoundAllContacts">
			#   <item quantity="one" msgid="5517063038754171134"\s*>"鬲賲 丕賱毓孬賵乇 毓賱賶 噩賴丞 丕鬲氐丕賱 賵丕丨丿丞"</item\s*>
			#   <item quantity="other" msgid="3852668542926965042"\s*>"鬲賲 丕賱毓孬賵乇 毓賱賶 <xliff:g id="COUNT">%d</xliff:g>"</item\s*>
			#</plurals>
			elsif ($line =~ /<plurals name="(?<plurals_name>.*?)">$/sgi)
			{
					my $plurals_name = $+{plurals_name};
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/plurals>/sgi))
					{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
							if($line =~ /<item quantity="(?<quantity_name>.*?)"\s*>(?<quantity_value>.*?)<\/item\s*>/sgi)
							{
								my $quantity_name = $+{quantity_name};
								my $quantity_value = $+{quantity_value};
								if($quantity_name =~ /(?<quantity_name>.*?)"\smsgid="(?<shuzi>.*?)$/sgi)
								{
									$quantity_name = $+{quantity_name};
								}
								#my $a_name;
								#my $a_value;
								#if($quantity_name =~ /(?<quantity_name>.*?)"\s(?<a_name>.*?)="(?<a_value>.*?)/sgi)		#这句怎么没有起作用呢？。。。
								#{
								#	$quantity_name = $+{quantity_name};
								#	$a_name = $+{a_name};
								#	$a_value = $+{a_value};
								#}
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $quantity_name;
								$arrayED[$linerow] = $plurals_name;
								$arrayEE[$linerow] = $quantity_value;
								$arrayEH[$linerow] = "plurals";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							#<item msgid="2048653993044269649" quantity="one">"密码中至少应包含 1 个小写字母"</item>
							elsif($line =~ /<item msgid="\d*" quantity="(?<quantity_name>.*?)"\s*>(?<quantity_value>.*?)<\/item\s*>/sgi)
							{
								my $quantity_name = $+{quantity_name};
								my $quantity_value = $+{quantity_value};
								#my $a_name;
								#my $a_value;
								#if($quantity_name =~ /(?<quantity_name>.*?)"\s(?<a_name>.*?)="(?<a_value>.*?)/sgi)		#这句怎么没有起作用呢？。。。
								#{
								#	$quantity_name = $+{quantity_name};
								#	$a_name = $+{a_name};
								#	$a_value = $+{a_value};
								#}
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $quantity_name;
								$arrayED[$linerow] = $plurals_name;
								$arrayEE[$linerow] = $quantity_value;
								$arrayEH[$linerow] = "plurals";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
					}		
			}

			#[J](10)###以下片段为数组1和数组2的情况
			#数组1
			#<string-array name="weeklist">
      #<item>"1 week"</item>
      #<item>"2 weeks"</item>
      #<item>"3 weeks"</item>
      #<item>"4 weeks"</item>
  		#</string-array>
			#数组2
	    #<string-array name="organizationTypes">
	    #<item msgid="7546335612189115615">"Εργασ?α"</item>
	    #<item msgid="4378074129049520373">"?λλο"</item>
	    #<item msgid="3455047468583965104">"Προσαρμοσμ?νο"</item>
	    #    <item>"Windows Live"</item>
	    #    <item>"Skype"</item>
	  	#</string-array>
			
			#eg: <string-array name="recent_keep_app" translatable="false">
			elsif ($line =~ /<string-array name="(?<string_array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $string_array_name = $+{string_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/string-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $string_array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "string-array";
										$arrayEJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $string_array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "string-array";
										$arrayEJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $arrayyuansu;
								$arrayED[$linerow] = $string_array_name;
								$arrayEE[$linerow] = $item_value;
								$arrayEH[$linerow] = "string-array";
								$arrayEJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/sgi)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayEA[$linerow] = $path;
							$arrayEB[$linerow] = 1+$i;
							$arrayEC[$linerow] = $arrayyuansu;
							$arrayED[$linerow] = $string_array_name;
							$arrayEE[$linerow] = $item_value;
							$arrayEH[$linerow] = "string-array";
							$arrayEJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[U]以下片段处理如下情况
			# <array name="camera_focusmode_icons" translatable="false">
      #  <item>@drawable/ic_indicators_automatic</item>
      #  <item>@drawable/ic_indicators_landscape</item>
      #  <item>@drawable/ic_indicators_macro</item>
      #  <item>@drawable/ic_indicators_focus</item>
      #</array>
			elsif ($line =~ /<array\s+name="(?<array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $array_name = $+{array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "array";
										$arrayEJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "array";
										$arrayEJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $arrayyuansu;
								$arrayED[$linerow] = $array_name;
								$arrayEE[$linerow] = $item_value;
								$arrayEH[$linerow] = "array";
								$arrayEJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/sgi)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayEA[$linerow] = $path;
							$arrayEB[$linerow] = 1+$i;
							$arrayEC[$linerow] = $arrayyuansu;
							$arrayED[$linerow] = $array_name;
							$arrayEE[$linerow] = $item_value;
							$arrayEH[$linerow] = "array";
							$arrayEJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			
			#[T]以下片段处理如下情况
			#<array name="difficultyLevel">
      #  <item>Easy</item>
      #  <item>Medium</item>
      #  <item>Hard</item>
      #</array>
      elsif ($line =~ /<array\s+name="(?<array_name>.*?)">$/sg)
			{
					my $array_name = $+{array_name};
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $arrayyuansu;
								$arrayED[$linerow] = $array_name;
								$arrayEE[$linerow] = $item_value;
								$arrayEH[$linerow] = "array";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayEA[$linerow] = $path;
							$arrayEB[$linerow] = 1+$i;
							$arrayEC[$linerow] = $arrayyuansu;
							$arrayED[$linerow] = $array_name;
							$arrayEE[$linerow] = $item_value;
							$arrayEH[$linerow] = "array";
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[K](11)以下处理如下情况：
			#<integer-array name="reminder_methods_values" translatable="false">
      #<item>1</item>  <!-- METHOD_ALERT -->
      #<item>2</item>  <!-- METHOD_EMAIL -->
      #<item>3</item>  <!-- METHOD_SMS -->
      #</integer-array>
			elsif ($line =~ /<integer-array name="(?<integer_array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $integer_array_name = $+{integer_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/integer-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $integer_array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "integer-array";
										$arrayEJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $integer_array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "integer-array";
										$arrayEJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $arrayyuansu;
								$arrayED[$linerow] = $integer_array_name;
								$arrayEE[$linerow] = $item_value;
								$arrayEH[$linerow] = "integer-array";
								$arrayEJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayEA[$linerow] = $path;
							$arrayEB[$linerow] = 1+$i;
							$arrayEC[$linerow] = $arrayyuansu;
							$arrayED[$linerow] = $integer_array_name;
							$arrayEE[$linerow] = $item_value;
							$arrayEH[$linerow] = "integer-array";
							$arrayEJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[L](11-a)以下处理如下情况：
			#<integer-array name="delete_repeating_values">
      #<item>0</item>
      #<item>1</item>
      #<item>2</item>
	    #</integer-array>
			elsif ($line =~ /<integer-array name="(?<integer_array_name>.*?)">$/sg)
			{
					my $integer_array_name = $+{integer_array_name};
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/integer-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $integer_array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "integer-array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayEA[$linerow] = $path;
										$arrayEB[$linerow] = 1+$i;
										$arrayEC[$linerow] = $arrayyuansu;
										$arrayED[$linerow] = $integer_array_name;
										$arrayEE[$linerow] = $item_value;
										$arrayEH[$linerow] = "integer-array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $arrayyuansu;
								$arrayED[$linerow] = $integer_array_name;
								$arrayEE[$linerow] = $item_value;
								$arrayEH[$linerow] = "integer-array";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayEA[$linerow] = $path;
							$arrayEB[$linerow] = 1+$i;
							$arrayEC[$linerow] = $arrayyuansu;
							$arrayED[$linerow] = $integer_array_name;
							$arrayEE[$linerow] = $item_value;
							$arrayEH[$linerow] = "integer-array";
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[M](12)以下处理translatable在name前面的情况
			#<string-array translatable="false" name="carrier_properties">
  		#	</string-array>
			elsif ($line =~ /<string-array\s+translatable="(?<trueorfalse>.*?)"\s+name="(?<string_array_name>.*?)">$/sg)
			{
					my $string_array_name = $+{string_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
						my $hang=1+$i;
				#		print OUTSAMELANGUAGE "error: ";print OUTSAMELANGUAGE "$temp ";print OUTSAMELANGUAGE "$path";print OUTSAMELANGUAGE "  ";print OUTSAMELANGUAGE "$hang";print OUTSAMELANGUAGE "\n";
					if($line =~ /<\/string-array>$/sgi)
					{
						$arrayEA[$linerow] = $path;
						$arrayEB[$linerow] = 1+$i;
						$arrayED[$linerow] = $string_array_name;
						$arrayEH[$linerow] = "string-array";
						$arrayEJ[$linerow] = $translatable;
						$linerow++;
				#		print "hahaha";
					}
					else
					{
						while(not($line =~ /<\/string-array>/sgi))
						{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
							if(not($line =~ /<!--|-->/sgi))
							{
								if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
								{
											my $item_value = $1;
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayEA[$linerow] = $path;
											$arrayEB[$linerow] = 1+$i;
											$arrayEC[$linerow] = $arrayyuansu;
											$arrayED[$linerow] = $string_array_name;
											$arrayEE[$linerow] = $item_value;
											$arrayEH[$linerow] = "string-array";
											$arrayEJ[$linerow] = $translatable;
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
								}
								elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
								{
											my $item_value = $+{item_value};
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayEA[$linerow] = $path;
											$arrayEB[$linerow] = 1+$i;
											$arrayEC[$linerow] = $arrayyuansu;
											$arrayED[$linerow] = $string_array_name;
											$arrayEE[$linerow] = $item_value;
											$arrayEH[$linerow] = "string-array";
											$arrayEJ[$linerow] = $translatable;
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
								}
								elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
								{
									my $item_value = "";
									$item++;
									my $arrayyuansu = "Element".$item;
									$arrayEA[$linerow] = $path;
									$arrayEB[$linerow] = 1+$i;
									$arrayEC[$linerow] = $arrayyuansu;
									$arrayED[$linerow] = $string_array_name;
									$arrayEE[$linerow] = $item_value;
									$arrayEH[$linerow] = "string-array";
									$arrayEJ[$linerow] = $translatable;
									$linerow++;
									$i++;
									$line = $xmlcontent[$i];
								}
								else
								{
									$i++;
									$line = $xmlcontent[$i];
								}
							}
							#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
							elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/sgi)
							{
								my $item_value = $1;
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayEA[$linerow] = $path;
								$arrayEB[$linerow] = 1+$i;
								$arrayEC[$linerow] = $arrayyuansu;
								$arrayED[$linerow] = $string_array_name;
								$arrayEE[$linerow] = $item_value;
								$arrayEH[$linerow] = "integer-array";
								$arrayEJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
					}
					
			}
			#[N](13)以下处理不带translatable的数组情况
			elsif ($line =~ /<string-array name="(?<string_array_name>.*?)"\s*>$/sgi)
			{
					my $string_array_name = $+{string_array_name};
					#my $translatable;
					#eg: <string-array name="recent_keep_app" translatable="false">
					#if ($string_array_name =~/(?<string_array_name>.*?)"\stranslatable="(?<trueorfalse>.*?)/sg)
					#{
					#	$string_array_name = $+{string_array_name};
					#	$translatable = $+{trueorfalse};
					#	$arrayEJ[$linerow] = $translatable;
					#}
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/string-array>/sgi))
					{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
								if(not($line =~ /<!--|-->/sgi))	
								{	if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
									{
											my $item_value = $1;
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayEA[$linerow] = $path;
											$arrayEB[$linerow] = 1+$i;
											$arrayEC[$linerow] = $arrayyuansu;
											$arrayED[$linerow] = $string_array_name;
											$arrayEE[$linerow] = $item_value;
											$arrayEH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
									{
											my $item_value = $+{item_value};
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayEA[$linerow] = $path;
											$arrayEB[$linerow] = 1+$i;
											$arrayEC[$linerow] = $arrayyuansu;
											$arrayED[$linerow] = $string_array_name;
											$arrayEE[$linerow] = $item_value;
											$arrayEH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
									{
											my $item_value = "";
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayEA[$linerow] = $path;
											$arrayEB[$linerow] = 1+$i;
											$arrayEC[$linerow] = $arrayyuansu;
											$arrayED[$linerow] = $string_array_name;
											$arrayEE[$linerow] = $item_value;
											$arrayEH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									else
									{
										$i++;
										$line = $xmlcontent[$i];
									}
								}
								#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
								elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/)
								{
									my $item_value = $1;
									$item++;
									my $arrayyuansu = "Element".$item;
									$arrayEA[$linerow] = $path;
									$arrayEB[$linerow] = 1+$i;
									$arrayEC[$linerow] = $arrayyuansu;
									$arrayED[$linerow] = $string_array_name;
									$arrayEE[$linerow] = $item_value;
									$arrayEH[$linerow] = "string-array";
									$linerow++;
									$i++;
									$line = $xmlcontent[$i];
								}
								else
								{
									$i++;
									$line = $xmlcontent[$i];
								}
					}
			}
			
			#[O](14)以下片段处理如下情况：
			#<string
			#name="permlab_samplesyncadapterAuthPassword">access to passwords for Sample SyncAdapter accounts</string>
			#<string
			#name="permdesc_samplesyncadapterAuth">Allows applications to see the usernames (email addresses) of
			#the Sample SyncAdapter account(s) you have configured.</string>
			elsif ($line =~ /^<string$/sgi)
			{
					$i++;
					$line = $xmlcontent[$i];
					my $string_name;
					my $part_value;
					my $string_value = "";
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					if ($line =~ /<\/string>/sgi)
					{
							if ($line =~ /name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/)
							{
									$string_name = $+{string_name};
									$string_value = $+{string_value};
								#	print "$string_name ";
								#	print "$string_value";
									$arrayEA[$linerow] = $path;
									$arrayEB[$linerow] = 1+$i;
									$arrayED[$linerow] = $string_name;
									$arrayEE[$linerow] = $string_value;
									$linerow++;
							}
					}
					else
					{
							while(not($line =~ /<\/string>/))
							{
									$line =~s/\s+/ /;
#									$line =~ s/\n\r/ /sgi;
#									$line =~ s/\r\n/ /sgi;
#									$line =~ s/\n/ /sgi;
									$line =~ s/^\s+//;
									$line =~ s/\s+$//;
									if($line =~ /\S+/)
									{
											if ($line =~ /^name="(?<string_name>.*?)">(.*)$/)
											{
												$string_name = $+{string_name};
												$string_value = $2;
											}
											else
											{
												$string_value=$string_value." ".$line;
											}
									}
									$i++;
									$line = $xmlcontent[$i];
							}
							#以下片段用于匹配</string>前面有内容的行
							if($line =~ /(\S+)<\/string>$/)
							{
									my $string_line = $xmlcontent[$i];		
									$string_line =~s/\s+/ /;
									$string_line =~ s/\n\r/ /sgi;
									$string_line =~ s/\r\n/ /sgi;
									$string_line =~ s/\n/ /sgi;
									$string_line =~s/<\/string>//;
									$string_line =~ s/^\s+//;
									$string_line =~ s/\s+$//;
									$string_value = $string_value." ".$string_line;
							}
							$arrayEA[$linerow] = $path;
							$arrayEB[$linerow] = 1+$i;
							$arrayED[$linerow] = $string_name;
							$arrayEE[$linerow] = $string_value;
							$linerow++;
					}
					
			}

			#[P](16)以下处理常规情况4：eg：<string name="name">
			#                        value1
			#                        value2
			#                value3 </string>
			#<string name="crypt_keeper_failed_summary" product="default">
			#  Encryption was interrupted and can\'t complete. As a result, the data on
			#  your phone is no longer accessible.
			#  \n\nTo resume using your phone, you must perform a factory reset.
			#  When you set up your phone after the reset, you\'ll have an opportunity
			#  to restore any data that was backed up to your Google Account.
			#</string>

			elsif ($line =~ /<string name="(?<string_name>.*?)">$/)
			{
				my $string_name = $+{string_name};
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayEI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				my $part_value = "";
				my $string_value = "".$part_value;
				$i++;
				$line = $xmlcontent[$i];
				$line =~s/^\s+/ /;
				$line =~s/\s+$/ /;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
					if($line =~ /\S+/sgi)
					{
						if (not($line =~ /^<!--.*-->$/))
						{
							my $string_line = $xmlcontent[$i];
							$string_value = $string_value.$string_line;
						}
					}
					$i++;
					$line = $xmlcontent[$i];
					$line =~s/^\s+/ /;
					$line =~s/\s+$/ /;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
				}
				if($line =~ /(\S+)<\/string>/)
				{
					my $string_line = $xmlcontent[$i];
					$string_line =~s/^\s+/ /;
					$string_line =~s/\s+$/ /;
					$string_line =~ s/^\s+//;
					$string_line =~ s/\s+$//;
					$string_line =~s/<\/string>//;
					$string_value = $string_value.$string_line;
					$string_value =~ s/\s+$//;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}

			#[V]以下处理常规情况4：eg:
			#<string product="tablet" name="lockscreen_failed_attempts_almost_glogin">
			#                        value1
			#                        value2
			#                value3 </string>
			elsif ($line =~ /<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">$/sgi)
			{
				my $string_name = $+{string_name};
				my $product_name = $+{product_name};
				my $translatable;
			#	my $part_value = $+{part_value};
				my $part_value ="";
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				my $string_value = "".$part_value;
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value." ".$string_line;
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];		
						$string_line =~s/\s+/ /;
						$string_line =~ s/\n\r/ /sgi;
						$string_line =~ s/\r\n/ /sgi;
						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value." ".$string_line;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$arrayEI[$linerow] = $product_name;
				$linerow++;
			}
			
			
			
			
			#[Q](17)以下处理常规情况5：eg：<string msgid="6766593624598183090" name="name">
			#                        value2
			#                        value3
			#                value4 </string>
			#<string msgid="6766593624598183090" name="crypt_keeper_failed_summary" product="default">
			#  Encryption was interrupted and can\'t complete. As a result, the data on
			#  your phone is no longer accessible.
			#  \n\nTo resume using your phone, you must perform a factory reset.
			#  When you set up your phone after the reset, you\'ll have an opportunity
			#  to restore any data that was backed up to your Google Account.
			#</string>
			elsif ($line =~ /<string msgid="\d*" name="(?<string_name>.*?)">$/sgi)
			{
				my $string_name = $+{string_name};
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayEI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				my $string_value = "";
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value.$string_line." ";
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];		
						$string_line =~s/\s+/ /;
						$string_line =~ s/\n\r/ /sgi;
						$string_line =~ s/\r\n/ /sgi;
						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value.$string_line;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
		#		$i++;
		#		$line = $xmlcontent[$i];
			}
			
			#[R](18)以下片段用于处理和上述情况差不多，但不带msgid的情况
			elsif ($line =~ /<string name="(?<string_name>.*?)">(.*)$/)
			{
				my $string_name = $+{string_name};
				my $part_value = $2;
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayEI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				my $string_value = "".$part_value;
				$i++;
				$line = $xmlcontent[$i];
				$line =~s/^\s+/ /;
				$line =~s/\s+$/ /;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
					if($line =~ /\S+/sgi)
					{
						if (not($line =~ /^<!--.*-->$/))
						{
							my $string_line = $xmlcontent[$i];
							$string_value = $string_value.$string_line;
						}
					}
					$i++;
					$line = $xmlcontent[$i];
					$line =~s/^\s+/ /;
					$line =~s/\s+$/ /;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
				}
				if($line =~ /(\S+)<\/string>/)
				{
					my $string_line = $xmlcontent[$i];
					$string_line =~s/^\s+/ /;
					$string_line =~s/\s+$/ /;
					$string_line =~ s/^\s+//;
					$string_line =~ s/\s+$//;
					$string_line =~s/<\/string>//;
					$string_value = $string_value.$string_line;
					$string_value =~ s/\s+$//;
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				$linerow++;
			}
			

			#[W]以下片段用于处理如下情况：
			#<string product="tablet" name="lockscreen_failed_attempts_almost_glogin">????????????????????????????????????<xliff:g id="number">%d</xliff:g>????????
      # ???? <xliff:g id="number">%d</xliff:g> ???????????????????????????? 
      # ????????????????????????????????? Google ????????????????????????????????????????????????????????????????? \n\n
      # ??????????????? <xliff:g id="number">%d</xliff:g> ?????????????????????????????????? 
      #</string>
			elsif($line =~/<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">(.*)$/g)
			{
				my $string_name = $+{string_name};
				my $part_value = $3;
				my $product_name = $+{product_name};
				my $translatable;
				$arrayEI[$linerow] = $product_name;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayEJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				my $string_value = "".$part_value." ";
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value.$string_line." ";
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];
						$string_line =~s/\s+/ /;
#						$string_line =~ s/\n\r/ /sgi;
#						$string_line =~ s/\r\n/ /sgi;
#						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value.$string_line;
						$string_value =~ s/\n$/ /sgi;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
						
				}
				$arrayEA[$linerow] = $path;
				$arrayEB[$linerow] = 1+$i;
				$arrayED[$linerow] = $string_name;
				$arrayEE[$linerow] = $string_value;
				print OUTSAMELANGUAGE " eee ";print OUTSAMELANGUAGE "$string_name";print OUTSAMELANGUAGE " : ";print OUTSAMELANGUAGE "$arrayEE[$linerow] \n";
				$linerow++;
				
		#		$i++;
		#		$line = $xmlcontent[$i];
			}
			
			
			#[S](15)以下片段处理如下情况：
			#	<string name="message_view_always_show_pictures_confirmation"
			#	>Pictures from this sender will be shown automatically.</string>
			#	<string name="account_setup_account_type_exchange_action_alternate"
			#	translatable="false">Microsoft Exchange ActiveSync</string>
			#有问题
			elsif ($line =~ /^<string\s+name="(?<string_name>.*?)"$/)
			{
					my $string_name = $+{string_name};
					my $string_value;
					my $translatable;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					if ($line =~ /<\/string>$/)
					{
							if ($line =~ /^>(?<string_value>.*?)<\/string>/)
							{
									$string_value = $+{string_value};
									$arrayEA[$linerow] = $path;
									$arrayEB[$linerow] = 1+$i;
									$arrayED[$linerow] = $string_name;
									$arrayEE[$linerow] = $string_value;
									$linerow++;
							}
							elsif ($line =~ /^translatable="(?<translatable>.*?)">(?<string_value>.*?)<\/string>/)
							{
									$translatable = $+{translatable};
									$translatable = "T".$translatable;
									$string_value = $+{string_value};
									$arrayEA[$linerow] = $path;
									$arrayEB[$linerow] = 1+$i;
									$arrayED[$linerow] = $string_name;
									$arrayEE[$linerow] = $string_value;
									$arrayEJ[$linerow] = $translatable;
									$linerow++;
							}
							else
							{
									print "(15)";print "$path";print " $i ";print "$line";
							}
							
					}
			}
			
			else
			{
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				if (not($line =~ /^<!--.*-->$/sgi))
				{
					if($line ne "")
					{
							my $hang=1+$i;
							print OUT "error: ";print OUT "$temp ";print OUT "$path";print OUT "  ";print OUT "$hang";print OUT "\n";
					}
				}
			}
		}
}
print OUT "\n\n\n\n\n\n\n\n\nbelow is chinese\n\n\n";
print "english done ";
		
#保存中文的情况，到数组@arrayCA；@arrayCB；@arrayCC；@arrayCD；@arrayCE；@arrayCF；@arrayCG；@arrayCH;@arrayCI
$linerow = 0;		#用于保存数组的行号
for ($temp=0;$temp<@zhlanguage;$temp++)
{		#以下对某一xml进行操作，只需要注意$allsamelanguagerow这个变量即可
		my $path = $zhlanguage[$temp];
		#打开该path，读取strings到excel表的某一sheet
		open(FILENAME,"$path")||die"error,can not open xmlfile\n";
		my @xmlcontent = <FILENAME>;
		#print OUT @xmlcontent;
		my $i=0;
		my $line;
		for($i=0;$i<@xmlcontent;$i++)
		{
			$line = $xmlcontent[$i];
	#		$line =~s/\n//;
	#		$line =~ s/^\s+//;
	#		$line =~ s/\s+$//;
			#删除前后空格及回车
			$line =~s/\s+/ /;
#			$line =~ s/\n\r/ /sgi;
#			$line =~ s/\r\n/ /sgi;
#			$line =~ s/\n/ /sgi;
			$line =~ s/^\s+//;
			$line =~ s/\s+$//;
			#注释不参与统计
			if ($line =~ /^<!--.*-->$/)
			{
					print OUTSAMELANGUAGE "$path ";print OUTSAMELANGUAGE "$i";print OUTSAMELANGUAGE "\n";
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
		#			$line =~ s/\n\r/ /sgi;
		#			$line =~ s/\r\n/ /sgi;
		#			$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
			}
			if ($line =~ /^<!--$/)
			{
		#			print OUTSAMELANGUAGE " \nhahaha ";print OUTSAMELANGUAGE "$path ";print OUTSAMELANGUAGE "$i";print OUTSAMELANGUAGE "\n";
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
		#			$line =~ s/\n\r/ /sgi;
		#			$line =~ s/\r\n/ /sgi;
		#			$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					my $x=0;
					while((not($line=~/^(-->\s*<skip \/>|-->)$/))&&($x<100))
					{
		#					print OUTSAMELANGUAGE " o ";
							$x++;
							$i++;
							$line = $xmlcontent[$i];
							#删除前后空格及回车
							$line =~s/\s+/ /;
		#					$line =~ s/\n\r/ /sgi;
		#					$line =~ s/\r\n/ /sgi;
		#					$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
					}
		#			print OUTSAMELANGUAGE "$i";
					if($x==100)
					{
		#					print OUTSAMELANGUAGE "plerror!!!!!!!";
							print "plerror!please contact lei.peng 589-149";
					}
			}
			
			#[A](1)以下片段处理如下情况：<string name="ringtone_summary"/>
			if ($line =~ /<string\s+name\s*="(?<string_name>.*?)"\s*\/>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			#[B](2)以下处理常规情况的strings，eg：常规情况1：<string name="name">value</string>
			#和常规情况2：<string name="searchview_description_clear" msgid="1330281990951833033">"Απαλοιφ? ερωτ?ματο?"</string>
			#和常规情况3：string name带product的：product在name后面
			#和常规情况4：<string name="web_user_agent_target_content" translatable="false">"Mobile "</string>
			elsif ($line =~ /<string\s+name\s*=\s*"(?<string_name>.*?)"\s*>\s*(?<string_value>.*?)\s*<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name ;
				my $translatable ;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayCI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				elsif ($string_name =~ /(?<string_name>.*?)"\s+translate="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			#[C](3)以下处理translatable在name前面的情况
			elsif($line =~ /<string\s+translatable="(?<translatable>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $translatable = $+{translatable};
				$translatable = "T".$translatable;
				$arrayCJ[$linerow] = $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			#[C](3-1)以下处理translate在name前面的情况
			#<string translate="false" name="sin_mathematical_value">sin</string>
			elsif($line =~ /<string\s+translate="(?<translatable>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $translatable = $+{translatable};
				$translatable = "T".$translatable;
				$arrayCJ[$linerow] = $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			#[D](4)以下片段处理如下常规情况：
			# <string product="nosdcard" name="no_storage_z788">"使用相机前请先装载USB存储设备,您可以先切换到内部存储空间保存少量照片。"</string>
			elsif ($line =~ /<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name = $+{product_name};
				my $translatable ;
				$arrayCI[$linerow] = $product_name;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				elsif ($string_name =~ /(?<string_name>.*?)"\s+translate="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			
			#[E](5)以下处理常规情况5：eg：<string msgid="1330281990951833033" name="searchview_description_clear">"Απαλοιφ? ερωτ?ματο?"</string>
			#和name后有product的情况
			elsif ($line =~ /<string\s+msgid="\d*"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name;
				if($string_name =~ /(?<string_name>.*?)"\sproduct="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayCI[$linerow] = $product_name;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			#[F](6)和常规情况7：<string msgid="4673411327373419641" product="nosdcard" name="sd_mount_summary"/>
			elsif ($line =~ /<string\s+msgid="\d*"\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)"\/>$/sgi)
			{
				my $product_name = $+{product_name};
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$arrayCI[$linerow] = $product_name;
				$linerow++;
			}
			#[G](7)和常规情况6：<string msgid="5996407024898469862" product="default" name="data_usage_limit_dialog">"达到指定的数据使用上限时，系统会停用您的<xliff:g id="NETWORKS">%1$s</xliff:g>数据连接。"\n\n"由于数据流量是由您的手机计算而得的，您的运营商对于流量的计算方法可能有所不同，因此请考虑设置一个较低的上限。"</string>
			elsif ($line =~ /<string msgid="\d*" product="(?<product_name>.*?)" name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $product_name = $+{product_name};
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$arrayCI[$linerow] = $product_name;
				$linerow++;
			}
			#[H](8)以下处理常规情况7：<string msgid="7345931695292850058" name="show_password_summary"/>
			elsif ($line =~/<string msgid="\d*" name="(?<string_name>.*?)"\/>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			
			#[I](9)###以下片段为复数1、复数2和复数3的情况
			#复数1
			# <plurals name="Nhours">
			#		 <!-- This is the label for a 1-hour reminder. -->
		  #    <item quantity="one">1 hour</item>
		  #    <!-- This is the label for a reminder of 2 or more hours. The actual number of
		  #         hours is a parameter. -->
		  #    <item quantity="other"><xliff:g id="count">%d</xliff:g> hours</item>
		  #</plurals>
			#复数2
		  # <plurals name="Nsongs">
		  #    <item quantity="other"><xliff:g id="count">%d</xliff:g> songs</item>
		  #</plurals>
		  #复数3
		  #<plurals name="listFoundAllContacts">
			#   <item quantity="one" msgid="5517063038754171134">"鬲賲 丕賱毓孬賵乇 毓賱賶 噩賴丞 丕鬲氐丕賱 賵丕丨丿丞"</item>
			#   <item quantity="other" msgid="3852668542926965042">"鬲賲 丕賱毓孬賵乇 毓賱賶 <xliff:g id="COUNT">%d</xliff:g>"</item>
			#</plurals>
			elsif ($line =~ /<plurals name="(?<plurals_name>.*?)">$/sgi)
			{
					my $plurals_name = $+{plurals_name};
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/plurals>/sgi))
					{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
							if($line =~ /<item quantity="(?<quantity_name>.*?)"\s*>(?<quantity_value>.*?)<\/item\s*>/sgi)
							{
								my $quantity_name = $+{quantity_name};
								my $quantity_value = $+{quantity_value};
								if($quantity_name =~ /(?<quantity_name>.*?)"\smsgid="(?<shuzi>.*?)$/sgi)
								{
									$quantity_name = $+{quantity_name};
								}
								#my $a_name;
								#my $a_value;
								#if($quantity_name =~ /(?<quantity_name>.*?)"\s(?<a_name>.*?)="(?<a_value>.*?)/sgi)		#这句怎么没有起作用呢？。。。
								#{
								#	$quantity_name = $+{quantity_name};
								#	$a_name = $+{a_name};
								#	$a_value = $+{a_value};
								#}
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $quantity_name;
								$arrayCD[$linerow] = $plurals_name;
								$arrayCF[$linerow] = $quantity_value;
								$arrayCH[$linerow] = "plurals";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							#<item msgid="2048653993044269649" quantity="one">"密码中至少应包含 1 个小写字母"</item>
							elsif($line =~ /<item msgid="\d*" quantity="(?<quantity_name>.*?)"\s*>(?<quantity_value>.*?)<\/item\s*>/sgi)
							{
								my $quantity_name = $+{quantity_name};
								my $quantity_value = $+{quantity_value};
								#my $a_name;
								#my $a_value;
								#if($quantity_name =~ /(?<quantity_name>.*?)"\s(?<a_name>.*?)="(?<a_value>.*?)/sgi)		#这句怎么没有起作用呢？。。。
								#{
								#	$quantity_name = $+{quantity_name};
								#	$a_name = $+{a_name};
								#	$a_value = $+{a_value};
								#}
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $quantity_name;
								$arrayCD[$linerow] = $plurals_name;
								$arrayCF[$linerow] = $quantity_value;
								$arrayCH[$linerow] = "plurals";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
					}		
			}

			#[J](10)###以下片段为数组1和数组2的情况
			#数组1
			#<string-array name="weeklist">
      #<item>"1 week"</item>
      #<item>"2 weeks"</item>
      #<item>"3 weeks"</item>
      #<item>"4 weeks"</item>
  		#</string-array>
			#数组2
	    #<string-array name="organizationTypes">
	    #<item msgid="7546335612189115615">"Εργασ?α"</item>
	    #<item msgid="4378074129049520373">"?λλο"</item>
	    #<item msgid="3455047468583965104">"Προσαρμοσμ?νο"</item>
	    #    <item>"Windows Live"</item>
	    #    <item>"Skype"</item>
	  	#</string-array>
			
			#eg: <string-array name="recent_keep_app" translatable="false">
			elsif ($line =~ /<string-array name="(?<string_array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $string_array_name = $+{string_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/string-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $string_array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "string-array";
										$arrayCJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $string_array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "string-array";
										$arrayCJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $arrayyuansu;
								$arrayCD[$linerow] = $string_array_name;
								$arrayCF[$linerow] = $item_value;
								$arrayCH[$linerow] = "string-array";
								$arrayCJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/sgi)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayCA[$linerow] = $path;
							$arrayCB[$linerow] = 1+$i;
							$arrayCC[$linerow] = $arrayyuansu;
							$arrayCD[$linerow] = $string_array_name;
							$arrayCF[$linerow] = $item_value;
							$arrayCH[$linerow] = "string-array";
							$arrayCJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[U]以下片段处理如下情况
			# <array name="camera_focusmode_icons" translatable="false">
      #  <item>@drawable/ic_indicators_automatic</item>
      #  <item>@drawable/ic_indicators_landscape</item>
      #  <item>@drawable/ic_indicators_macro</item>
      #  <item>@drawable/ic_indicators_focus</item>
      #</array>
			elsif ($line =~ /<array\s+name="(?<array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $array_name = $+{array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "array";
										$arrayCJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "array";
										$arrayCJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $arrayyuansu;
								$arrayCD[$linerow] = $array_name;
								$arrayCF[$linerow] = $item_value;
								$arrayCH[$linerow] = "array";
								$arrayCJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/sgi)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayCA[$linerow] = $path;
							$arrayCB[$linerow] = 1+$i;
							$arrayCC[$linerow] = $arrayyuansu;
							$arrayCD[$linerow] = $array_name;
							$arrayCF[$linerow] = $item_value;
							$arrayCH[$linerow] = "array";
							$arrayCJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			
			#[T]以下片段处理如下情况
			#<array name="difficultyLevel">
      #  <item>Easy</item>
      #  <item>Medium</item>
      #  <item>Hard</item>
      #</array>
      elsif ($line =~ /<array\s+name="(?<array_name>.*?)">$/sg)
			{
					my $array_name = $+{array_name};
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $arrayyuansu;
								$arrayCD[$linerow] = $array_name;
								$arrayCF[$linerow] = $item_value;
								$arrayCH[$linerow] = "array";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayCA[$linerow] = $path;
							$arrayCB[$linerow] = 1+$i;
							$arrayCC[$linerow] = $arrayyuansu;
							$arrayCD[$linerow] = $array_name;
							$arrayCF[$linerow] = $item_value;
							$arrayCH[$linerow] = "array";
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[K](11)以下处理如下情况：
			#<integer-array name="reminder_methods_values" translatable="false">
      #<item>1</item>  <!-- METHOD_ALERT -->
      #<item>2</item>  <!-- METHOD_EMAIL -->
      #<item>3</item>  <!-- METHOD_SMS -->
      #</integer-array>
			elsif ($line =~ /<integer-array name="(?<integer_array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $integer_array_name = $+{integer_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/integer-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $integer_array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "integer-array";
										$arrayCJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $integer_array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "integer-array";
										$arrayCJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $arrayyuansu;
								$arrayCD[$linerow] = $integer_array_name;
								$arrayCF[$linerow] = $item_value;
								$arrayCH[$linerow] = "integer-array";
								$arrayCJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayCA[$linerow] = $path;
							$arrayCB[$linerow] = 1+$i;
							$arrayCC[$linerow] = $arrayyuansu;
							$arrayCD[$linerow] = $integer_array_name;
							$arrayCF[$linerow] = $item_value;
							$arrayCH[$linerow] = "integer-array";
							$arrayCJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[L](11-a)以下处理如下情况：
			#<integer-array name="delete_repeating_values">
      #<item>0</item>
      #<item>1</item>
      #<item>2</item>
	    #</integer-array>
			elsif ($line =~ /<integer-array name="(?<integer_array_name>.*?)">$/sg)
			{
					my $integer_array_name = $+{integer_array_name};
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/integer-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $integer_array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "integer-array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayCA[$linerow] = $path;
										$arrayCB[$linerow] = 1+$i;
										$arrayCC[$linerow] = $arrayyuansu;
										$arrayCD[$linerow] = $integer_array_name;
										$arrayCF[$linerow] = $item_value;
										$arrayCH[$linerow] = "integer-array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $arrayyuansu;
								$arrayCD[$linerow] = $integer_array_name;
								$arrayCF[$linerow] = $item_value;
								$arrayCH[$linerow] = "integer-array";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayCA[$linerow] = $path;
							$arrayCB[$linerow] = 1+$i;
							$arrayCC[$linerow] = $arrayyuansu;
							$arrayCD[$linerow] = $integer_array_name;
							$arrayCF[$linerow] = $item_value;
							$arrayCH[$linerow] = "integer-array";
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[M](12)以下处理translatable在name前面的情况
			#<string-array translatable="false" name="carrier_properties">
  		#	</string-array>
			elsif ($line =~ /<string-array\s+translatable="(?<trueorfalse>.*?)"\s+name="(?<string_array_name>.*?)">$/sg)
			{
					my $string_array_name = $+{string_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
						my $hang=1+$i;
				#		print OUTSAMELANGUAGE "error: ";print OUTSAMELANGUAGE "$temp ";print OUTSAMELANGUAGE "$path";print OUTSAMELANGUAGE "  ";print OUTSAMELANGUAGE "$hang";print OUTSAMELANGUAGE "\n";
					if($line =~ /<\/string-array>$/sgi)
					{
						$arrayCA[$linerow] = $path;
						$arrayCB[$linerow] = 1+$i;
						$arrayCD[$linerow] = $string_array_name;
						$arrayCH[$linerow] = "string-array";
						$arrayCJ[$linerow] = $translatable;
						$linerow++;
				#		print "hahaha";
					}
					else
					{
						while(not($line =~ /<\/string-array>/sgi))
						{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
							if(not($line =~ /<!--|-->/sgi))
							{
								if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
								{
											my $item_value = $1;
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayCA[$linerow] = $path;
											$arrayCB[$linerow] = 1+$i;
											$arrayCC[$linerow] = $arrayyuansu;
											$arrayCD[$linerow] = $string_array_name;
											$arrayCF[$linerow] = $item_value;
											$arrayCH[$linerow] = "string-array";
											$arrayCJ[$linerow] = $translatable;
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
								}
								elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
								{
											my $item_value = $+{item_value};
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayCA[$linerow] = $path;
											$arrayCB[$linerow] = 1+$i;
											$arrayCC[$linerow] = $arrayyuansu;
											$arrayCD[$linerow] = $string_array_name;
											$arrayCF[$linerow] = $item_value;
											$arrayCH[$linerow] = "string-array";
											$arrayCJ[$linerow] = $translatable;
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
								}
								elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
								{
									my $item_value = "";
									$item++;
									my $arrayyuansu = "Element".$item;
									$arrayCA[$linerow] = $path;
									$arrayCB[$linerow] = 1+$i;
									$arrayCC[$linerow] = $arrayyuansu;
									$arrayCD[$linerow] = $string_array_name;
									$arrayCF[$linerow] = $item_value;
									$arrayCH[$linerow] = "string-array";
									$arrayCJ[$linerow] = $translatable;
									$linerow++;
									$i++;
									$line = $xmlcontent[$i];
								}
								else
								{
									$i++;
									$line = $xmlcontent[$i];
								}
							}
							#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
							elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/sgi)
							{
								my $item_value = $1;
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayCA[$linerow] = $path;
								$arrayCB[$linerow] = 1+$i;
								$arrayCC[$linerow] = $arrayyuansu;
								$arrayCD[$linerow] = $string_array_name;
								$arrayCF[$linerow] = $item_value;
								$arrayCH[$linerow] = "integer-array";
								$arrayCJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
					}
					
			}
			#[N](13)以下处理不带translatable的数组情况
			elsif ($line =~ /<string-array name="(?<string_array_name>.*?)"\s*>$/sgi)
			{
					my $string_array_name = $+{string_array_name};
					#my $translatable;
					#eg: <string-array name="recent_keep_app" translatable="false">
					#if ($string_array_name =~/(?<string_array_name>.*?)"\stranslatable="(?<trueorfalse>.*?)/sg)
					#{
					#	$string_array_name = $+{string_array_name};
					#	$translatable = $+{trueorfalse};
					#	$arrayCJ[$linerow] = $translatable;
					#}
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/string-array>/sgi))
					{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
								if(not($line =~ /<!--|-->/sgi))	
								{	if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
									{
											my $item_value = $1;
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayCA[$linerow] = $path;
											$arrayCB[$linerow] = 1+$i;
											$arrayCC[$linerow] = $arrayyuansu;
											$arrayCD[$linerow] = $string_array_name;
											$arrayCF[$linerow] = $item_value;
											$arrayCH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
									{
											my $item_value = $+{item_value};
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayCA[$linerow] = $path;
											$arrayCB[$linerow] = 1+$i;
											$arrayCC[$linerow] = $arrayyuansu;
											$arrayCD[$linerow] = $string_array_name;
											$arrayCF[$linerow] = $item_value;
											$arrayCH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
									{
											my $item_value = "";
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayCA[$linerow] = $path;
											$arrayCB[$linerow] = 1+$i;
											$arrayCC[$linerow] = $arrayyuansu;
											$arrayCD[$linerow] = $string_array_name;
											$arrayCF[$linerow] = $item_value;
											$arrayCH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									else
									{
										$i++;
										$line = $xmlcontent[$i];
									}
								}
								#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
								elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/)
								{
									my $item_value = $1;
									$item++;
									my $arrayyuansu = "Element".$item;
									$arrayCA[$linerow] = $path;
									$arrayCB[$linerow] = 1+$i;
									$arrayCC[$linerow] = $arrayyuansu;
									$arrayCD[$linerow] = $string_array_name;
									$arrayCF[$linerow] = $item_value;
									$arrayCH[$linerow] = "string-array";
									$linerow++;
									$i++;
									$line = $xmlcontent[$i];
								}
								else
								{
									$i++;
									$line = $xmlcontent[$i];
								}
					}
			}
			
			#[O](14)以下片段处理如下情况：
			#<string
			#name="permlab_samplesyncadapterAuthPassword">access to passwords for Sample SyncAdapter accounts</string>
			#<string
			#name="permdesc_samplesyncadapterAuth">Allows applications to see the usernames (email addresses) of
			#the Sample SyncAdapter account(s) you have configured.</string>
			elsif ($line =~ /^<string$/sgi)
			{
					$i++;
					$line = $xmlcontent[$i];
					my $string_name;
					my $part_value;
					my $string_value = "";
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					if ($line =~ /<\/string>/sgi)
					{
							if ($line =~ /name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/)
							{
									$string_name = $+{string_name};
									$string_value = $+{string_value};
								#	print "$string_name ";
								#	print "$string_value";
									$arrayCA[$linerow] = $path;
									$arrayCB[$linerow] = 1+$i;
									$arrayCD[$linerow] = $string_name;
									$arrayCF[$linerow] = $string_value;
									$linerow++;
							}
					}
					else
					{
							while(not($line =~ /<\/string>/))
							{
									$line =~s/\s+/ /;
#									$line =~ s/\n\r/ /sgi;
#									$line =~ s/\r\n/ /sgi;
#									$line =~ s/\n/ /sgi;
									$line =~ s/^\s+//;
									$line =~ s/\s+$//;
									if($line =~ /\S+/)
									{
											if ($line =~ /^name="(?<string_name>.*?)">(.*)$/)
											{
												$string_name = $+{string_name};
												$string_value = $2;
											}
											else
											{
												$string_value=$string_value." ".$line;
											}
									}
									$i++;
									$line = $xmlcontent[$i];
							}
							#以下片段用于匹配</string>前面有内容的行
							if($line =~ /(\S+)<\/string>$/)
							{
									my $string_line = $xmlcontent[$i];		
									$string_line =~s/\s+/ /;
									$string_line =~ s/\n\r/ /sgi;
									$string_line =~ s/\r\n/ /sgi;
									$string_line =~ s/\n/ /sgi;
									$string_line =~s/<\/string>//;
									$string_line =~ s/^\s+//;
									$string_line =~ s/\s+$//;
									$string_value = $string_value." ".$string_line;
							}
							$arrayCA[$linerow] = $path;
							$arrayCB[$linerow] = 1+$i;
							$arrayCD[$linerow] = $string_name;
							$arrayCF[$linerow] = $string_value;
							$linerow++;
					}
					
			}

			#[P](16)以下处理常规情况4：eg：<string name="name">
			#                        value1
			#                        value2
			#                value3 </string>
			#<string name="crypt_keeper_failed_summary" product="default">
			#  Encryption was interrupted and can\'t complete. As a result, the data on
			#  your phone is no longer accessible.
			#  \n\nTo resume using your phone, you must perform a factory reset.
			#  When you set up your phone after the reset, you\'ll have an opportunity
			#  to restore any data that was backed up to your Google Account.
			#</string>

			elsif ($line =~ /<string name="(?<string_name>.*?)">$/)
			{
				my $string_name = $+{string_name};
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayCI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				my $part_value = "";
				my $string_value = "".$part_value;
				$i++;
				$line = $xmlcontent[$i];
				$line =~s/^\s+/ /;
				$line =~s/\s+$/ /;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
					if($line =~ /\S+/sgi)
					{
						if (not($line =~ /^<!--.*-->$/))
						{
							my $string_line = $xmlcontent[$i];
							$string_value = $string_value.$string_line;
						}
					}
					$i++;
					$line = $xmlcontent[$i];
					$line =~s/^\s+/ /;
					$line =~s/\s+$/ /;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
				}
				if($line =~ /(\S+)<\/string>/)
				{
					my $string_line = $xmlcontent[$i];
					$string_line =~s/^\s+/ /;
					$string_line =~s/\s+$/ /;
					$string_line =~ s/^\s+//;
					$string_line =~ s/\s+$//;
					$string_line =~s/<\/string>//;
					$string_value = $string_value.$string_line;
					$string_value =~ s/\s+$//;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}

			#[V]以下处理常规情况4：eg:
			#<string product="tablet" name="lockscreen_failed_attempts_almost_glogin">
			#                        value1
			#                        value2
			#                value3 </string>
			elsif ($line =~ /<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">$/sgi)
			{
				my $string_name = $+{string_name};
				my $product_name = $+{product_name};
				my $translatable;
			#	my $part_value = $+{part_value};
				my $part_value ="";
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				my $string_value = "".$part_value;
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value." ".$string_line;
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];		
						$string_line =~s/\s+/ /;
						$string_line =~ s/\n\r/ /sgi;
						$string_line =~ s/\r\n/ /sgi;
						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value." ".$string_line;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$arrayCI[$linerow] = $product_name;
				$linerow++;
			}
			
			
			
			
			#[Q](17)以下处理常规情况5：eg：<string msgid="6766593624598183090" name="name">
			#                        value2
			#                        value3
			#                value4 </string>
			#<string msgid="6766593624598183090" name="crypt_keeper_failed_summary" product="default">
			#  Encryption was interrupted and can\'t complete. As a result, the data on
			#  your phone is no longer accessible.
			#  \n\nTo resume using your phone, you must perform a factory reset.
			#  When you set up your phone after the reset, you\'ll have an opportunity
			#  to restore any data that was backed up to your Google Account.
			#</string>
			elsif ($line =~ /<string msgid="\d*" name="(?<string_name>.*?)">$/sgi)
			{
				my $string_name = $+{string_name};
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayCI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				my $string_value = "";
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value.$string_line." ";
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];		
						$string_line =~s/\s+/ /;
						$string_line =~ s/\n\r/ /sgi;
						$string_line =~ s/\r\n/ /sgi;
						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value.$string_line;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
		#		$i++;
		#		$line = $xmlcontent[$i];
			}
			
			#[R](18)以下片段用于处理和上述情况差不多，但不带msgid的情况
			elsif ($line =~ /<string name="(?<string_name>.*?)">(.*)$/)
			{
				my $string_name = $+{string_name};
				my $part_value = $2;
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayCI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				my $string_value = "".$part_value;
				$i++;
				$line = $xmlcontent[$i];
				$line =~s/^\s+/ /;
				$line =~s/\s+$/ /;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
					if($line =~ /\S+/sgi)
					{
						if (not($line =~ /^<!--.*-->$/))
						{
							my $string_line = $xmlcontent[$i];
							$string_value = $string_value.$string_line;
						}
					}
					$i++;
					$line = $xmlcontent[$i];
					$line =~s/^\s+/ /;
					$line =~s/\s+$/ /;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
				}
				if($line =~ /(\S+)<\/string>/)
				{
					my $string_line = $xmlcontent[$i];
					$string_line =~s/^\s+/ /;
					$string_line =~s/\s+$/ /;
					$string_line =~ s/^\s+//;
					$string_line =~ s/\s+$//;
					$string_line =~s/<\/string>//;
					$string_value = $string_value.$string_line;
					$string_value =~ s/\s+$//;
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				$linerow++;
			}
			

			#[W]以下片段用于处理如下情况：
			#<string product="tablet" name="lockscreen_failed_attempts_almost_glogin">????????????????????????????????????<xliff:g id="number">%d</xliff:g>????????
      # ???? <xliff:g id="number">%d</xliff:g> ???????????????????????????? 
      # ????????????????????????????????? Google ????????????????????????????????????????????????????????????????? \n\n
      # ??????????????? <xliff:g id="number">%d</xliff:g> ?????????????????????????????????? 
      #</string>
			elsif($line =~/<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">(.*)$/g)
			{
				my $string_name = $+{string_name};
				my $part_value = $3;
				my $product_name = $+{product_name};
				my $translatable;
				$arrayCI[$linerow] = $product_name;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayCJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				my $string_value = "".$part_value." ";
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value.$string_line." ";
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];
						$string_line =~s/\s+/ /;
#						$string_line =~ s/\n\r/ /sgi;
#						$string_line =~ s/\r\n/ /sgi;
#						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value.$string_line;
						$string_value =~ s/\n$/ /sgi;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
						
				}
				$arrayCA[$linerow] = $path;
				$arrayCB[$linerow] = 1+$i;
				$arrayCD[$linerow] = $string_name;
				$arrayCF[$linerow] = $string_value;
				print OUTSAMELANGUAGE " eee ";print OUTSAMELANGUAGE "$string_name";print OUTSAMELANGUAGE " : ";print OUTSAMELANGUAGE "$arrayCF[$linerow] \n";
				$linerow++;
				
		#		$i++;
		#		$line = $xmlcontent[$i];
			}
			
			
			#[S](15)以下片段处理如下情况：
			#	<string name="message_view_always_show_pictures_confirmation"
			#	>Pictures from this sender will be shown automatically.</string>
			#	<string name="account_setup_account_type_exchange_action_alternate"
			#	translatable="false">Microsoft Exchange ActiveSync</string>
			#有问题
			elsif ($line =~ /^<string\s+name="(?<string_name>.*?)"$/)
			{
					my $string_name = $+{string_name};
					my $string_value;
					my $translatable;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					if ($line =~ /<\/string>$/)
					{
							if ($line =~ /^>(?<string_value>.*?)<\/string>/)
							{
									$string_value = $+{string_value};
									$arrayCA[$linerow] = $path;
									$arrayCB[$linerow] = 1+$i;
									$arrayCD[$linerow] = $string_name;
									$arrayCF[$linerow] = $string_value;
									$linerow++;
							}
							elsif ($line =~ /^translatable="(?<translatable>.*?)">(?<string_value>.*?)<\/string>/)
							{
									$translatable = $+{translatable};
									$translatable = "T".$translatable;
									$string_value = $+{string_value};
									$arrayCA[$linerow] = $path;
									$arrayCB[$linerow] = 1+$i;
									$arrayCD[$linerow] = $string_name;
									$arrayCF[$linerow] = $string_value;
									$arrayCJ[$linerow] = $translatable;
									$linerow++;
							}
							else
							{
									print "(15)";print "$path";print " $i ";print "$line";
							}
							
					}
			}
			
			else
			{
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				if (not($line =~ /^<!--.*-->$/sgi))
				{
					if($line ne "")
					{
							my $hang=1+$i;
							print OUT "error: ";print OUT "$temp ";print OUT "$path";print OUT "  ";print OUT "$hang";print OUT "\n";
					}
				}
			}
		}
}
print OUT "\n\n\n\n\n\n\n\n\nbelow is other language\n\n\n";
print "chinese done ";

###############################################################################################################################
#以下片段实现把中文的string填到英文string相应的位置
###############################################################################################################################
my $tempC = 0;
my $temp = 0;
for ($tempC = 0;$tempC<@arrayCD;$tempC++)
{
		for ($temp = 0;$temp<@arrayED;$temp++)
		{
				if($arrayCI[$tempC])																						#若中文string存在product
				{
						if(
						  ($arrayEI[$temp])																						#若英文string存在product
						  &&   ($arrayEI[$temp] eq $arrayCI[$tempC])									#中英文product是否相等
						  &&   ($arrayED[$temp] eq $arrayCD[$tempC])									#和英文string name比较，看是否相等
						  &&   (&pathmatchornot($arrayEA[$temp],$arrayCA[$tempC]))		#看中文的xml文件path是否和英文xml文件的path匹配
						  )
						{
							$arrayEF[$temp] = $arrayCF[$tempC];
						}
				}
				
				if($arrayCH[$tempC])																						#若中文string存在Array or Plurals
				{
						if(
						  ($arrayEH[$temp])																					#若英文string存在Array or Plurals
						  &&   ($arrayEH[$temp] eq $arrayCH[$tempC])									#中英文Array or Plurals是否相等
						  &&   ($arrayEC[$temp] eq $arrayCC[$tempC])									#中英文的index是否相等
						  &&   ($arrayED[$temp] eq $arrayCD[$tempC])									#和英文string name比较，看是否相等
						  &&   (&pathmatchornot($arrayEA[$temp],$arrayCA[$tempC]))				#看中文的xml文件path是否和英文xml文件的path匹配
						  )
						{
							$arrayEF[$temp] = $arrayCF[$tempC];
						}
				}
				
				if(not(($arrayCH[$tempC])||($arrayCI[$tempC])||($arrayCC[$tempC])))		#若不存在Array or Plurals、produc、index
				{
						if(
						  ($arrayED[$temp] eq $arrayCD[$tempC])														#和英文string name比较，看是否相等
						  &&   (&pathmatchornot($arrayEA[$temp],$arrayCA[$tempC]))				#看中文的xml文件path是否和英文xml文件的path匹配
						  )
						{
							$arrayEF[$temp] = $arrayCF[$tempC];
						}
				}
		}
}


my $xlBook = Excel::Writer::XLSX->new("Alllanguagesstrings".".xlsx") || die "$!\n";
my $bodyFormat = $xlBook->add_format(
		color       => 'black',
		 border    => 1,
		align       => 'right',
		font        => '宋体'
		);
my @xlSheet;
for(my $langindex = 0;$langindex<@languages;$langindex++)
{		$onelanguage = $languages[$langindex];
		$xlSheet[$langindex] = $xlBook->add_worksheet("$onelanguage");
		sub toexcel()
		{
		 my(@canshu) = @_;
		    $xlSheet[$langindex]->write(
		    $canshu[2].$canshu[1],
		    decode('utf8',"$canshu[0]"), $bodyFormat0);
		}
		my @samelanguage;
		#以下片段搜索出其他任一种语言的所有xml，并把路径保存在数组：@samelanguage。
		my $temp = 0;
		foreach my $path (@allFileDirectoris)
		{
			my $text = $path;
			if($text=~/.*\/$onelanguage\/.*/i)
			{
				$samelanguage[$temp++]=$path;
			}
		}
	
		my $tiaoshitemp = @samelanguage;
		print "$onelanguage";print "  ";print "xml number: ";print "$tiaoshitemp";print "\n";
		
	
		###############################################################################################################################
		my @arrayOA;		my @arrayTA = @arrayEA;
		my @arrayOB;		my @arrayTB = @arrayEB;
		my @arrayOC;		my @arrayTC = @arrayEC;
		my @arrayOD;		my @arrayTD = @arrayED;
		my @arrayOE;		my @arrayTE = @arrayEE;
		my @arrayOF;		my @arrayTF = @arrayEF;
		my @arrayOG;		my @arrayTG = @arrayEG;
		my @arrayOH;		my @arrayTH = @arrayEH;
		my @arrayOI;		my @arrayTI = @arrayEI;
		my @arrayOJ;		my @arrayTJ = @arrayEJ;
		
		#保存其他任一语言的情况，到数组@arrayOA；@arrayOB；@arrayOC；@arrayOD；@arrayOE；@arrayOF；@arrayOG；@arrayOH;@arrayOI
		$linerow = 0;		#用于保存数组的行号
for ($temp=0;$temp<@samelanguage;$temp++)
{		#以下对某一xml进行操作，只需要注意$allsamelanguagerow这个变量即可
		my $path = $samelanguage[$temp];
		#打开该path，读取strings到excel表的某一sheet
		open(FILENAME,"$path")||die"error,can not open xmlfile\n";
		my @xmlcontent = <FILENAME>;
		#print OUT @xmlcontent;
		my $i=0;
		my $line;
		for($i=0;$i<@xmlcontent;$i++)
		{
			$line = $xmlcontent[$i];
	#		$line =~s/\n//;
	#		$line =~ s/^\s+//;
	#		$line =~ s/\s+$//;
			#删除前后空格及回车
			$line =~s/\s+/ /;
#			$line =~ s/\n\r/ /sgi;
#			$line =~ s/\r\n/ /sgi;
#			$line =~ s/\n/ /sgi;
			$line =~ s/^\s+//;
			$line =~ s/\s+$//;
			#注释不参与统计
			if ($line =~ /^<!--.*-->$/)
			{
					print OUTSAMELANGUAGE "$path ";print OUTSAMELANGUAGE "$i";print OUTSAMELANGUAGE "\n";
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
		#			$line =~ s/\n\r/ /sgi;
		#			$line =~ s/\r\n/ /sgi;
		#			$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
			}
			if ($line =~ /^<!--$/)
			{
		#			print OUTSAMELANGUAGE " \nhahaha ";print OUTSAMELANGUAGE "$path ";print OUTSAMELANGUAGE "$i";print OUTSAMELANGUAGE "\n";
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
		#			$line =~ s/\n\r/ /sgi;
		#			$line =~ s/\r\n/ /sgi;
		#			$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					my $x=0;
					while((not($line=~/^(-->\s*<skip \/>|-->)$/))&&($x<100))
					{
		#					print OUTSAMELANGUAGE " o ";
							$x++;
							$i++;
							$line = $xmlcontent[$i];
							#删除前后空格及回车
							$line =~s/\s+/ /;
		#					$line =~ s/\n\r/ /sgi;
		#					$line =~ s/\r\n/ /sgi;
		#					$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
					}
		#			print OUTSAMELANGUAGE "$i";
					if($x==100)
					{
		#					print OUTSAMELANGUAGE "plerror!!!!!!!";
							print "plerror!please contact lei.peng 589-149";
					}
			}
			
			#[A](1)以下片段处理如下情况：<string name="ringtone_summary"/>
			if ($line =~ /<string\s+name\s*="(?<string_name>.*?)"\s*\/>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			#[B](2)以下处理常规情况的strings，eg：常规情况1：<string name="name">value</string>
			#和常规情况2：<string name="searchview_description_clear" msgid="1330281990951833033">"Απαλοιφ? ερωτ?ματο?"</string>
			#和常规情况3：string name带product的：product在name后面
			#和常规情况4：<string name="web_user_agent_target_content" translatable="false">"Mobile "</string>
			elsif ($line =~ /<string\s+name\s*=\s*"(?<string_name>.*?)"\s*>\s*(?<string_value>.*?)\s*<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name ;
				my $translatable ;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayOI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				elsif ($string_name =~ /(?<string_name>.*?)"\s+translate="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			#[C](3)以下处理translatable在name前面的情况
			elsif($line =~ /<string\s+translatable="(?<translatable>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $translatable = $+{translatable};
				$translatable = "T".$translatable;
				$arrayOJ[$linerow] = $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			#[C](3-1)以下处理translate在name前面的情况
			#<string translate="false" name="sin_mathematical_value">sin</string>
			elsif($line =~ /<string\s+translate="(?<translatable>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $translatable = $+{translatable};
				$translatable = "T".$translatable;
				$arrayOJ[$linerow] = $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			#[D](4)以下片段处理如下常规情况：
			# <string product="nosdcard" name="no_storage_z788">"使用相机前请先装载USB存储设备,您可以先切换到内部存储空间保存少量照片。"</string>
			elsif ($line =~ /<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name = $+{product_name};
				my $translatable ;
				$arrayOI[$linerow] = $product_name;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				elsif ($string_name =~ /(?<string_name>.*?)"\s+translate="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			
			#[E](5)以下处理常规情况5：eg：<string msgid="1330281990951833033" name="searchview_description_clear">"Απαλοιφ? ερωτ?ματο?"</string>
			#和name后有product的情况
			elsif ($line =~ /<string\s+msgid="\d*"\s+name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				my $product_name;
				if($string_name =~ /(?<string_name>.*?)"\sproduct="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayOI[$linerow] = $product_name;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			#[F](6)和常规情况7：<string msgid="4673411327373419641" product="nosdcard" name="sd_mount_summary"/>
			elsif ($line =~ /<string\s+msgid="\d*"\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)"\/>$/sgi)
			{
				my $product_name = $+{product_name};
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$arrayOI[$linerow] = $product_name;
				$linerow++;
			}
			#[G](7)和常规情况6：<string msgid="5996407024898469862" product="default" name="data_usage_limit_dialog">"达到指定的数据使用上限时，系统会停用您的<xliff:g id="NETWORKS">%1$s</xliff:g>数据连接。"\n\n"由于数据流量是由您的手机计算而得的，您的运营商对于流量的计算方法可能有所不同，因此请考虑设置一个较低的上限。"</string>
			elsif ($line =~ /<string msgid="\d*" product="(?<product_name>.*?)" name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/sgi)
			{
				my $product_name = $+{product_name};
				my $string_name = $+{string_name};
				my $string_value = $+{string_value};
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$arrayOI[$linerow] = $product_name;
				$linerow++;
			}
			#[H](8)以下处理常规情况7：<string msgid="7345931695292850058" name="show_password_summary"/>
			elsif ($line =~/<string msgid="\d*" name="(?<string_name>.*?)"\/>/sgi)
			{
				my $string_name = $+{string_name};
				my $string_value = "";
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			
			#[I](9)###以下片段为复数1、复数2和复数3的情况
			#复数1
			# <plurals name="Nhours">
			#		 <!-- This is the label for a 1-hour reminder. -->
		  #    <item quantity="one">1 hour</item>
		  #    <!-- This is the label for a reminder of 2 or more hours. The actual number of
		  #         hours is a parameter. -->
		  #    <item quantity="other"><xliff:g id="count">%d</xliff:g> hours</item>
		  #</plurals>
			#复数2
		  # <plurals name="Nsongs">
		  #    <item quantity="other"><xliff:g id="count">%d</xliff:g> songs</item>
		  #</plurals>
		  #复数3
		  #<plurals name="listFoundAllContacts">
			#   <item quantity="one" msgid="5517063038754171134">"鬲賲 丕賱毓孬賵乇 毓賱賶 噩賴丞 丕鬲氐丕賱 賵丕丨丿丞"</item>
			#   <item quantity="other" msgid="3852668542926965042">"鬲賲 丕賱毓孬賵乇 毓賱賶 <xliff:g id="COUNT">%d</xliff:g>"</item>
			#</plurals>
			elsif ($line =~ /<plurals name="(?<plurals_name>.*?)">$/sgi)
			{
					my $plurals_name = $+{plurals_name};
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/plurals>/sgi))
					{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
							if($line =~ /<item quantity="(?<quantity_name>.*?)"\s*>(?<quantity_value>.*?)<\/item\s*>/sgi)
							{
								my $quantity_name = $+{quantity_name};
								my $quantity_value = $+{quantity_value};
								if($quantity_name =~ /(?<quantity_name>.*?)"\smsgid="(?<shuzi>.*?)$/sgi)
								{
									$quantity_name = $+{quantity_name};
								}
								#my $a_name;
								#my $a_value;
								#if($quantity_name =~ /(?<quantity_name>.*?)"\s(?<a_name>.*?)="(?<a_value>.*?)/sgi)		#这句怎么没有起作用呢？。。。
								#{
								#	$quantity_name = $+{quantity_name};
								#	$a_name = $+{a_name};
								#	$a_value = $+{a_value};
								#}
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $quantity_name;
								$arrayOD[$linerow] = $plurals_name;
								$arrayOG[$linerow] = $quantity_value;
								$arrayOH[$linerow] = "plurals";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							#<item msgid="2048653993044269649" quantity="one">"密码中至少应包含 1 个小写字母"</item>
							elsif($line =~ /<item msgid="\d*" quantity="(?<quantity_name>.*?)"\s*>(?<quantity_value>.*?)<\/item\s*>/sgi)
							{
								my $quantity_name = $+{quantity_name};
								my $quantity_value = $+{quantity_value};
								#my $a_name;
								#my $a_value;
								#if($quantity_name =~ /(?<quantity_name>.*?)"\s(?<a_name>.*?)="(?<a_value>.*?)/sgi)		#这句怎么没有起作用呢？。。。
								#{
								#	$quantity_name = $+{quantity_name};
								#	$a_name = $+{a_name};
								#	$a_value = $+{a_value};
								#}
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $quantity_name;
								$arrayOD[$linerow] = $plurals_name;
								$arrayOG[$linerow] = $quantity_value;
								$arrayOH[$linerow] = "plurals";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
					}		
			}

			#[J](10)###以下片段为数组1和数组2的情况
			#数组1
			#<string-array name="weeklist">
      #<item>"1 week"</item>
      #<item>"2 weeks"</item>
      #<item>"3 weeks"</item>
      #<item>"4 weeks"</item>
  		#</string-array>
			#数组2
	    #<string-array name="organizationTypes">
	    #<item msgid="7546335612189115615">"Εργασ?α"</item>
	    #<item msgid="4378074129049520373">"?λλο"</item>
	    #<item msgid="3455047468583965104">"Προσαρμοσμ?νο"</item>
	    #    <item>"Windows Live"</item>
	    #    <item>"Skype"</item>
	  	#</string-array>
			
			#eg: <string-array name="recent_keep_app" translatable="false">
			elsif ($line =~ /<string-array name="(?<string_array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $string_array_name = $+{string_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/string-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $string_array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "string-array";
										$arrayOJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $string_array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "string-array";
										$arrayOJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $arrayyuansu;
								$arrayOD[$linerow] = $string_array_name;
								$arrayOG[$linerow] = $item_value;
								$arrayOH[$linerow] = "string-array";
								$arrayOJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/sgi)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayOA[$linerow] = $path;
							$arrayOB[$linerow] = 1+$i;
							$arrayOC[$linerow] = $arrayyuansu;
							$arrayOD[$linerow] = $string_array_name;
							$arrayOG[$linerow] = $item_value;
							$arrayOH[$linerow] = "string-array";
							$arrayOJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[U]以下片段处理如下情况
			# <array name="camera_focusmode_icons" translatable="false">
      #  <item>@drawable/ic_indicators_automatic</item>
      #  <item>@drawable/ic_indicators_landscape</item>
      #  <item>@drawable/ic_indicators_macro</item>
      #  <item>@drawable/ic_indicators_focus</item>
      #</array>
			elsif ($line =~ /<array\s+name="(?<array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $array_name = $+{array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "array";
										$arrayOJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "array";
										$arrayOJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $arrayyuansu;
								$arrayOD[$linerow] = $array_name;
								$arrayOG[$linerow] = $item_value;
								$arrayOH[$linerow] = "array";
								$arrayOJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/sgi)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayOA[$linerow] = $path;
							$arrayOB[$linerow] = 1+$i;
							$arrayOC[$linerow] = $arrayyuansu;
							$arrayOD[$linerow] = $array_name;
							$arrayOG[$linerow] = $item_value;
							$arrayOH[$linerow] = "array";
							$arrayOJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			
			#[T]以下片段处理如下情况
			#<array name="difficultyLevel">
      #  <item>Easy</item>
      #  <item>Medium</item>
      #  <item>Hard</item>
      #</array>
      elsif ($line =~ /<array\s+name="(?<array_name>.*?)">$/sg)
			{
					my $array_name = $+{array_name};
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $arrayyuansu;
								$arrayOD[$linerow] = $array_name;
								$arrayOG[$linerow] = $item_value;
								$arrayOH[$linerow] = "array";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayOA[$linerow] = $path;
							$arrayOB[$linerow] = 1+$i;
							$arrayOC[$linerow] = $arrayyuansu;
							$arrayOD[$linerow] = $array_name;
							$arrayOG[$linerow] = $item_value;
							$arrayOH[$linerow] = "array";
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[K](11)以下处理如下情况：
			#<integer-array name="reminder_methods_values" translatable="false">
      #<item>1</item>  <!-- METHOD_ALERT -->
      #<item>2</item>  <!-- METHOD_EMAIL -->
      #<item>3</item>  <!-- METHOD_SMS -->
      #</integer-array>
			elsif ($line =~ /<integer-array name="(?<integer_array_name>.*?)"\s+translatable="(?<trueorfalse>.*?)">$/sg)
			{
					my $integer_array_name = $+{integer_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/integer-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $integer_array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "integer-array";
										$arrayOJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $integer_array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "integer-array";
										$arrayOJ[$linerow] = $translatable;
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $arrayyuansu;
								$arrayOD[$linerow] = $integer_array_name;
								$arrayOG[$linerow] = $item_value;
								$arrayOH[$linerow] = "integer-array";
								$arrayOJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayOA[$linerow] = $path;
							$arrayOB[$linerow] = 1+$i;
							$arrayOC[$linerow] = $arrayyuansu;
							$arrayOD[$linerow] = $integer_array_name;
							$arrayOG[$linerow] = $item_value;
							$arrayOH[$linerow] = "integer-array";
							$arrayOJ[$linerow] = $translatable;
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[L](11-a)以下处理如下情况：
			#<integer-array name="delete_repeating_values">
      #<item>0</item>
      #<item>1</item>
      #<item>2</item>
	    #</integer-array>
			elsif ($line =~ /<integer-array name="(?<integer_array_name>.*?)">$/sg)
			{
					my $integer_array_name = $+{integer_array_name};
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/integer-array>/sgi))
					{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if(not($line =~ /<!--|-->/sgi))
						{
							if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
							{
										my $item_value = $1;
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $integer_array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "integer-array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
							{
										my $item_value = $+{item_value};
										$item++;
										my $arrayyuansu = "Element".$item;
										$arrayOA[$linerow] = $path;
										$arrayOB[$linerow] = 1+$i;
										$arrayOC[$linerow] = $arrayyuansu;
										$arrayOD[$linerow] = $integer_array_name;
										$arrayOG[$linerow] = $item_value;
										$arrayOH[$linerow] = "integer-array";
										$linerow++;
										$i++;
										$line = $xmlcontent[$i];
							}
							elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
							{
								my $item_value = "";
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $arrayyuansu;
								$arrayOD[$linerow] = $integer_array_name;
								$arrayOG[$linerow] = $item_value;
								$arrayOH[$linerow] = "integer-array";
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
						#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
						elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/)
						{
							my $item_value = $1;
							$item++;
							my $arrayyuansu = "Element".$item;
							$arrayOA[$linerow] = $path;
							$arrayOB[$linerow] = 1+$i;
							$arrayOC[$linerow] = $arrayyuansu;
							$arrayOD[$linerow] = $integer_array_name;
							$arrayOG[$linerow] = $item_value;
							$arrayOH[$linerow] = "integer-array";
							$linerow++;
							$i++;
							$line = $xmlcontent[$i];
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
					}
			}
			#[M](12)以下处理translatable在name前面的情况
			#<string-array translatable="false" name="carrier_properties">
  		#	</string-array>
			elsif ($line =~ /<string-array\s+translatable="(?<trueorfalse>.*?)"\s+name="(?<string_array_name>.*?)">$/sg)
			{
					my $string_array_name = $+{string_array_name};
					my $translatable = $+{trueorfalse};
					$translatable = "T".$translatable;
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
						my $hang=1+$i;
				#		print OUTSAMELANGUAGE "error: ";print OUTSAMELANGUAGE "$temp ";print OUTSAMELANGUAGE "$path";print OUTSAMELANGUAGE "  ";print OUTSAMELANGUAGE "$hang";print OUTSAMELANGUAGE "\n";
					if($line =~ /<\/string-array>$/sgi)
					{
						$arrayOA[$linerow] = $path;
						$arrayOB[$linerow] = 1+$i;
						$arrayOD[$linerow] = $string_array_name;
						$arrayOH[$linerow] = "string-array";
						$arrayOJ[$linerow] = $translatable;
						$linerow++;
				#		print "hahaha";
					}
					else
					{
						while(not($line =~ /<\/string-array>/sgi))
						{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
							if(not($line =~ /<!--|-->/sgi))
							{
								if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
								{
											my $item_value = $1;
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayOA[$linerow] = $path;
											$arrayOB[$linerow] = 1+$i;
											$arrayOC[$linerow] = $arrayyuansu;
											$arrayOD[$linerow] = $string_array_name;
											$arrayOG[$linerow] = $item_value;
											$arrayOH[$linerow] = "string-array";
											$arrayOJ[$linerow] = $translatable;
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
								}
								elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
								{
											my $item_value = $+{item_value};
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayOA[$linerow] = $path;
											$arrayOB[$linerow] = 1+$i;
											$arrayOC[$linerow] = $arrayyuansu;
											$arrayOD[$linerow] = $string_array_name;
											$arrayOG[$linerow] = $item_value;
											$arrayOH[$linerow] = "string-array";
											$arrayOJ[$linerow] = $translatable;
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
								}
								elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
								{
									my $item_value = "";
									$item++;
									my $arrayyuansu = "Element".$item;
									$arrayOA[$linerow] = $path;
									$arrayOB[$linerow] = 1+$i;
									$arrayOC[$linerow] = $arrayyuansu;
									$arrayOD[$linerow] = $string_array_name;
									$arrayOG[$linerow] = $item_value;
									$arrayOH[$linerow] = "string-array";
									$arrayOJ[$linerow] = $translatable;
									$linerow++;
									$i++;
									$line = $xmlcontent[$i];
								}
								else
								{
									$i++;
									$line = $xmlcontent[$i];
								}
							}
							#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
							elsif($line =~ /^<item\s*>(.*)<\/item\s*>\s+<!--(.*)-->$/sgi)
							{
								my $item_value = $1;
								$item++;
								my $arrayyuansu = "Element".$item;
								$arrayOA[$linerow] = $path;
								$arrayOB[$linerow] = 1+$i;
								$arrayOC[$linerow] = $arrayyuansu;
								$arrayOD[$linerow] = $string_array_name;
								$arrayOG[$linerow] = $item_value;
								$arrayOH[$linerow] = "integer-array";
								$arrayOJ[$linerow] = $translatable;
								$linerow++;
								$i++;
								$line = $xmlcontent[$i];
							}
							else
							{
								$i++;
								$line = $xmlcontent[$i];
							}
						}
					}
					
			}
			#[N](13)以下处理不带translatable的数组情况
			elsif ($line =~ /<string-array name="(?<string_array_name>.*?)"\s*>$/sgi)
			{
					my $string_array_name = $+{string_array_name};
					#my $translatable;
					#eg: <string-array name="recent_keep_app" translatable="false">
					#if ($string_array_name =~/(?<string_array_name>.*?)"\stranslatable="(?<trueorfalse>.*?)/sg)
					#{
					#	$string_array_name = $+{string_array_name};
					#	$translatable = $+{trueorfalse};
					#	$arrayOJ[$linerow] = $translatable;
					#}
					my $item = 0;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					while(not($line =~ /<\/string-array>/sgi))
					{
							#删除前后空格及回车
							$line =~s/\s+/ /;
#							$line =~ s/\n\r/ /sgi;
#							$line =~ s/\r\n/ /sgi;
#							$line =~ s/\n/ /sgi;
							$line =~ s/^\s+//;
							$line =~ s/\s+$//;
								if(not($line =~ /<!--|-->/sgi))	
								{	if($line =~ /<item msgid="\d*"\s*>(.*)<\/item\s*>/sgi)
									{
											my $item_value = $1;
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayOA[$linerow] = $path;
											$arrayOB[$linerow] = 1+$i;
											$arrayOC[$linerow] = $arrayyuansu;
											$arrayOD[$linerow] = $string_array_name;
											$arrayOG[$linerow] = $item_value;
											$arrayOH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									elsif($line =~ /<item\s*>(?<item_value>.*?)<\/item\s*>/sgi)
									{
											my $item_value = $+{item_value};
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayOA[$linerow] = $path;
											$arrayOB[$linerow] = 1+$i;
											$arrayOC[$linerow] = $arrayyuansu;
											$arrayOD[$linerow] = $string_array_name;
											$arrayOG[$linerow] = $item_value;
											$arrayOH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									elsif($line =~ /<item msgid="\d*"\s*\/>$/sgi)
									{
											my $item_value = "";
											$item++;
											my $arrayyuansu = "Element".$item;
											$arrayOA[$linerow] = $path;
											$arrayOB[$linerow] = 1+$i;
											$arrayOC[$linerow] = $arrayyuansu;
											$arrayOD[$linerow] = $string_array_name;
											$arrayOG[$linerow] = $item_value;
											$arrayOH[$linerow] = "string-array";
											$linerow++;
											$i++;
											$line = $xmlcontent[$i];
									}
									else
									{
										$i++;
										$line = $xmlcontent[$i];
									}
								}
								#以下处理：<item>1</item>  <!-- METHOD_ALERT -->的情况
								elsif($line =~ /<item\s*>(.*)<\/item\s*>\s*<!--(.*)-->/)
								{
									my $item_value = $1;
									$item++;
									my $arrayyuansu = "Element".$item;
									$arrayOA[$linerow] = $path;
									$arrayOB[$linerow] = 1+$i;
									$arrayOC[$linerow] = $arrayyuansu;
									$arrayOD[$linerow] = $string_array_name;
									$arrayOG[$linerow] = $item_value;
									$arrayOH[$linerow] = "string-array";
									$linerow++;
									$i++;
									$line = $xmlcontent[$i];
								}
								else
								{
									$i++;
									$line = $xmlcontent[$i];
								}
					}
			}
			
			#[O](14)以下片段处理如下情况：
			#<string
			#name="permlab_samplesyncadapterAuthPassword">access to passwords for Sample SyncAdapter accounts</string>
			#<string
			#name="permdesc_samplesyncadapterAuth">Allows applications to see the usernames (email addresses) of
			#the Sample SyncAdapter account(s) you have configured.</string>
			elsif ($line =~ /^<string$/sgi)
			{
					$i++;
					$line = $xmlcontent[$i];
					my $string_name;
					my $part_value;
					my $string_value = "";
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					if ($line =~ /<\/string>/sgi)
					{
							if ($line =~ /name="(?<string_name>.*?)">(?<string_value>.*?)<\/string>/)
							{
									$string_name = $+{string_name};
									$string_value = $+{string_value};
								#	print "$string_name ";
								#	print "$string_value";
									$arrayOA[$linerow] = $path;
									$arrayOB[$linerow] = 1+$i;
									$arrayOD[$linerow] = $string_name;
									$arrayOG[$linerow] = $string_value;
									$linerow++;
							}
					}
					else
					{
							while(not($line =~ /<\/string>/))
							{
									$line =~s/\s+/ /;
#									$line =~ s/\n\r/ /sgi;
#									$line =~ s/\r\n/ /sgi;
#									$line =~ s/\n/ /sgi;
									$line =~ s/^\s+//;
									$line =~ s/\s+$//;
									if($line =~ /\S+/)
									{
											if ($line =~ /^name="(?<string_name>.*?)">(.*)$/)
											{
												$string_name = $+{string_name};
												$string_value = $2;
											}
											else
											{
												$string_value=$string_value." ".$line;
											}
									}
									$i++;
									$line = $xmlcontent[$i];
							}
							#以下片段用于匹配</string>前面有内容的行
							if($line =~ /(\S+)<\/string>$/)
							{
									my $string_line = $xmlcontent[$i];		
									$string_line =~s/\s+/ /;
									$string_line =~ s/\n\r/ /sgi;
									$string_line =~ s/\r\n/ /sgi;
									$string_line =~ s/\n/ /sgi;
									$string_line =~s/<\/string>//;
									$string_line =~ s/^\s+//;
									$string_line =~ s/\s+$//;
									$string_value = $string_value." ".$string_line;
							}
							$arrayOA[$linerow] = $path;
							$arrayOB[$linerow] = 1+$i;
							$arrayOD[$linerow] = $string_name;
							$arrayOG[$linerow] = $string_value;
							$linerow++;
					}
					
			}

			#[P](16)以下处理常规情况4：eg：<string name="name">
			#                        value1
			#                        value2
			#                value3 </string>
			#<string name="crypt_keeper_failed_summary" product="default">
			#  Encryption was interrupted and can\'t complete. As a result, the data on
			#  your phone is no longer accessible.
			#  \n\nTo resume using your phone, you must perform a factory reset.
			#  When you set up your phone after the reset, you\'ll have an opportunity
			#  to restore any data that was backed up to your Google Account.
			#</string>

			elsif ($line =~ /<string name="(?<string_name>.*?)">$/)
			{
				my $string_name = $+{string_name};
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayOI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				my $part_value = "";
				my $string_value = "".$part_value;
				$i++;
				$line = $xmlcontent[$i];
				$line =~s/^\s+/ /;
				$line =~s/\s+$/ /;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
					if($line =~ /\S+/sgi)
					{
						if (not($line =~ /^<!--.*-->$/))
						{
							my $string_line = $xmlcontent[$i];
							$string_value = $string_value.$string_line;
						}
					}
					$i++;
					$line = $xmlcontent[$i];
					$line =~s/^\s+/ /;
					$line =~s/\s+$/ /;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
				}
				if($line =~ /(\S+)<\/string>/)
				{
					my $string_line = $xmlcontent[$i];
					$string_line =~s/^\s+/ /;
					$string_line =~s/\s+$/ /;
					$string_line =~ s/^\s+//;
					$string_line =~ s/\s+$//;
					$string_line =~s/<\/string>//;
					$string_value = $string_value.$string_line;
					$string_value =~ s/\s+$//;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}

			#[V]以下处理常规情况4：eg:
			#<string product="tablet" name="lockscreen_failed_attempts_almost_glogin">
			#                        value1
			#                        value2
			#                value3 </string>
			elsif ($line =~ /<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">$/sgi)
			{
				my $string_name = $+{string_name};
				my $product_name = $+{product_name};
				my $translatable;
			#	my $part_value = $+{part_value};
				my $part_value ="";
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				my $string_value = "".$part_value;
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value." ".$string_line;
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];		
						$string_line =~s/\s+/ /;
						$string_line =~ s/\n\r/ /sgi;
						$string_line =~ s/\r\n/ /sgi;
						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value." ".$string_line;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$arrayOI[$linerow] = $product_name;
				$linerow++;
			}
			
			
			
			
			#[Q](17)以下处理常规情况5：eg：<string msgid="6766593624598183090" name="name">
			#                        value2
			#                        value3
			#                value4 </string>
			#<string msgid="6766593624598183090" name="crypt_keeper_failed_summary" product="default">
			#  Encryption was interrupted and can\'t complete. As a result, the data on
			#  your phone is no longer accessible.
			#  \n\nTo resume using your phone, you must perform a factory reset.
			#  When you set up your phone after the reset, you\'ll have an opportunity
			#  to restore any data that was backed up to your Google Account.
			#</string>
			elsif ($line =~ /<string msgid="\d*" name="(?<string_name>.*?)">$/sgi)
			{
				my $string_name = $+{string_name};
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayOI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				my $string_value = "";
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value.$string_line." ";
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];		
						$string_line =~s/\s+/ /;
						$string_line =~ s/\n\r/ /sgi;
						$string_line =~ s/\r\n/ /sgi;
						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value.$string_line;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
		#		$i++;
		#		$line = $xmlcontent[$i];
			}
			
			#[R](18)以下片段用于处理和上述情况差不多，但不带msgid的情况
			elsif ($line =~ /<string name="(?<string_name>.*?)">(.*)$/)
			{
				my $string_name = $+{string_name};
				my $part_value = $2;
				my $product_name;
				my $translatable;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+product="(?<product_name>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$product_name = $+{product_name};
					$arrayOI[$linerow] = $product_name;
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				my $string_value = "".$part_value;
				$i++;
				$line = $xmlcontent[$i];
				$line =~s/^\s+/ /;
				$line =~s/\s+$/ /;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				while(not($line =~ /<\/string>/sgi))
				{
					if($line =~ /\S+/sgi)
					{
						if (not($line =~ /^<!--.*-->$/))
						{
							my $string_line = $xmlcontent[$i];
							$string_value = $string_value.$string_line;
						}
					}
					$i++;
					$line = $xmlcontent[$i];
					$line =~s/^\s+/ /;
					$line =~s/\s+$/ /;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
				}
				if($line =~ /(\S+)<\/string>/)
				{
					my $string_line = $xmlcontent[$i];
					$string_line =~s/^\s+/ /;
					$string_line =~s/\s+$/ /;
					$string_line =~ s/^\s+//;
					$string_line =~ s/\s+$//;
					$string_line =~s/<\/string>//;
					$string_value = $string_value.$string_line;
					$string_value =~ s/\s+$//;
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				$linerow++;
			}
			
			#[W]以下片段用于处理如下情况：
			#<string product="tablet" name="lockscreen_failed_attempts_almost_glogin">????????????????????????????????????<xliff:g id="number">%d</xliff:g>????????
      # ???? <xliff:g id="number">%d</xliff:g> ???????????????????????????? 
      # ????????????????????????????????? Google ????????????????????????????????????????????????????????????????? \n\n
      # ??????????????? <xliff:g id="number">%d</xliff:g> ?????????????????????????????????? 
      #</string>
			elsif($line =~/<string\s+product="(?<product_name>.*?)"\s+name="(?<string_name>.*?)">(.*)$/g)
			{
				my $string_name = $+{string_name};
				my $part_value = $3;
				my $product_name = $+{product_name};
				my $translatable;
				$arrayOI[$linerow] = $product_name;
				if($string_name =~ /(?<string_name>.*?)"\s+msgid="(?<shuzi>.*?)$/sgi)
				{
					$string_name = $+{string_name};
				}
				if($string_name =~ /(?<string_name>.*?)"\s+translatable="(?<translatable>.*?)$/sgi)
				{
					$string_name = $+{string_name};
					$translatable = $+{translatable};
					$translatable = "T".$translatable;
					$arrayOJ[$linerow] = $translatable;
				}
				$i++;
				$line = $xmlcontent[$i];
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				my $string_value = "".$part_value." ";
				while(not($line =~ /<\/string>/sgi))
				{
						#删除前后空格及回车
						$line =~s/\s+/ /;
#						$line =~ s/\n\r/ /sgi;
#						$line =~ s/\r\n/ /sgi;
#						$line =~ s/\n/ /sgi;
						$line =~ s/^\s+//;
						$line =~ s/\s+$//;
						if (not(($line =~ /^<!--.*/sgi)||($line =~ /.*-->$/sgi)))
						{
								if($line =~ /\S+/sgi)
								{
										my $string_line = $xmlcontent[$i];
										$string_line =~ s/^\s*(\S.*\S)\s*$/$1/;
								#		$string_line =~ s/\n\r/\s/sgi;
								#		$string_line =~ s/\r\n/\s/sgi;
								#		$string_line =~ s/\n/\s/sgi;
										$string_value = $string_value.$string_line." ";
								#		print OUT "hahaha ";print OUT "$path";print OUT "$i";print OUT "string_line:";print OUT "$string_value";print OUT "\n";
										$i++;
										$line = $xmlcontent[$i];
								}
						}
						else
						{
							$i++;
							$line = $xmlcontent[$i];
						}
				}
				#以下片段用于匹配</string>前面有内容的行
				if($line =~ /(\S+)<\/string>/)
				{
						my $string_line = $xmlcontent[$i];
						$string_line =~s/\s+/ /;
#						$string_line =~ s/\n\r/ /sgi;
#						$string_line =~ s/\r\n/ /sgi;
#						$string_line =~ s/\n/ /sgi;
						$string_line =~s/<\/string>//;
						$string_line =~ s/^\s+//;
						$string_line =~ s/\s+$//;
						$string_value = $string_value.$string_line;
						$string_value =~ s/\n$/ /sgi;
						$string_value =~ s/^\s+//;
						$string_value =~ s/\s+$//;
						
				}
				$arrayOA[$linerow] = $path;
				$arrayOB[$linerow] = 1+$i;
				$arrayOD[$linerow] = $string_name;
				$arrayOG[$linerow] = $string_value;
				print OUTSAMELANGUAGE " eee ";print OUTSAMELANGUAGE "$string_name";print OUTSAMELANGUAGE " : ";print OUTSAMELANGUAGE "$arrayOG[$linerow] \n";
				$linerow++;
				
		#		$i++;
		#		$line = $xmlcontent[$i];
			}
			
			
			#[S](15)以下片段处理如下情况：
			#	<string name="message_view_always_show_pictures_confirmation"
			#	>Pictures from this sender will be shown automatically.</string>
			#	<string name="account_setup_account_type_exchange_action_alternate"
			#	translatable="false">Microsoft Exchange ActiveSync</string>
			#有问题
			elsif ($line =~ /^<string\s+name="(?<string_name>.*?)"$/)
			{
					my $string_name = $+{string_name};
					my $string_value;
					my $translatable;
					$i++;
					$line = $xmlcontent[$i];
					#删除前后空格及回车
					$line =~s/\s+/ /;
#					$line =~ s/\n\r/ /sgi;
#					$line =~ s/\r\n/ /sgi;
#					$line =~ s/\n/ /sgi;
					$line =~ s/^\s+//;
					$line =~ s/\s+$//;
					if ($line =~ /<\/string>$/)
					{
							if ($line =~ /^>(?<string_value>.*?)<\/string>/)
							{
									$string_value = $+{string_value};
									$arrayOA[$linerow] = $path;
									$arrayOB[$linerow] = 1+$i;
									$arrayOD[$linerow] = $string_name;
									$arrayOG[$linerow] = $string_value;
									$linerow++;
							}
							elsif ($line =~ /^translatable="(?<translatable>.*?)">(?<string_value>.*?)<\/string>/)
							{
									$translatable = $+{translatable};
									$translatable = "T".$translatable;
									$string_value = $+{string_value};
									$arrayOA[$linerow] = $path;
									$arrayOB[$linerow] = 1+$i;
									$arrayOD[$linerow] = $string_name;
									$arrayOG[$linerow] = $string_value;
									$arrayOJ[$linerow] = $translatable;
									$linerow++;
							}
							else
							{
									print "(15)";print "$path";print " $i ";print "$line";
							}
							
					}
			}
			
			else
			{
				#删除前后空格及回车
				$line =~s/\s+/ /;
#				$line =~ s/\n\r/ /sgi;
#				$line =~ s/\r\n/ /sgi;
#				$line =~ s/\n/ /sgi;
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				if (not($line =~ /^<!--.*-->$/sgi))
				{
					if($line ne "")
					{
							my $hang=1+$i;
							print OUT "error: ";print OUT "$temp ";print OUT "$path";print OUT "  ";print OUT "$hang";print OUT "\n";
					}
				}
			}
		}
}
	
	###############################################################################################################################
	#以下片段实现把其他语言的string填到英文string相应的位置
	###############################################################################################################################
	my $tempO = 0;
	my $temp = 0;
	for ($tempO = 0;$tempO<@arrayOD;$tempO++)
	{
			for ($temp = 0;$temp<@arrayTD;$temp++)
			{
					if($arrayOI[$tempO])																						#若其他语言string存在product
					{
							if(
							  ($arrayEI[$temp])																					#若英文string存在product
							  &&   ($arrayTI[$temp] eq $arrayOI[$tempO])									#其他语言和英文product是否相等
							  &&   ($arrayTD[$temp] eq $arrayOD[$tempO])									#和英文string name比较，看是否相等
							  &&   (&pathmatchornot($arrayTA[$temp],$arrayOA[$tempO]))				#看其他语言的xml文件path是否和英文xml文件的path匹配
							  )
							{
								$arrayTG[$temp] = $arrayOG[$tempO];
							}
					}
					
					if($arrayOH[$tempO])																							#若其他语言string存在Array or Plurals
					{
							if(
							  ($arrayTH[$temp])																						#若英文string存在Array or Plurals
							  &&   ($arrayTH[$temp] eq $arrayOH[$tempO])									#其他语言和英文Array or Plurals是否相等
							  &&   ($arrayTC[$temp] eq $arrayOC[$tempO])									#其他语言和英文的index是否相等
							  &&   ($arrayTD[$temp] eq $arrayOD[$tempO])									#和英文string name比较，看是否相等
							  &&   (&pathmatchornot($arrayTA[$temp],$arrayOA[$tempO]))				#看其他语言的xml文件path是否和英文xml文件的path匹配
							  )
							{
								$arrayTG[$temp] = $arrayOG[$tempO];
							}
					}
					
					if(not(($arrayOH[$tempO])||($arrayOI[$tempO])||($arrayOC[$tempO])))		#若不存在Array or Plurals、produc、index
					{
						if(not(($arrayEH[$temp])||($arrayEI[$temp])||($arrayEC[$temp])))	#判断英文是否也不存在Array or Plurals、produc、index
						{
							if(
							  ($arrayTD[$temp] eq $arrayOD[$tempO])														#和英文string name比较，看是否相等
							  &&   (&pathmatchornot($arrayTA[$temp],$arrayOA[$tempO]))				#看中文的xml文件path是否和英文xml文件的path匹配
							  )
							{
								$arrayTG[$temp] = $arrayOG[$tempO];
							}
						}
					}
					
			}
	}
	
	###############################################################################################################################

	for($temp=0;$temp<@arrayED;$temp++)
	{
			#以下片段实现改路径名，把英文xml路径改为当前所处理的语言xml路径名
			my $pathT = $arrayTA[$temp];
			$pathT =~ s#\/#\\#sg;
			my @arraypathT = split /\\/,$pathT;
			my $countT = @arraypathT;
			$arraypathT[$countT-2]=$onelanguage;
			my $result = join "\\",@arraypathT;
			$arrayTA[$temp] = $result;
			#end改路径名
			&toexcel($arrayTA[$temp],2+$temp,'A');
			&toexcel($arrayTB[$temp],2+$temp,'B');
			&toexcel($arrayTC[$temp],2+$temp,'C');
			&toexcel($arrayTD[$temp],2+$temp,'D');
			$arrayTE[$temp] =~ s/\s+/ /g;
			#$arrayTE[$temp]=~s/\&amp\;/\&/g;
			#$arrayTE[$temp]=~s/\&lt\;/</g;
			#$arrayTE[$temp]=~s/\&gt\;/>/g;
			if("=-O" eq $arrayTE[$temp])
			{
				$arrayTE[$temp]="\"=-O\"";
			}
			#加入引号
			if(not($arrayTE[$temp]=~/^\".*\"$/sgi))
			{
				if(0==$arrayTE[$temp])
				{
					$arrayTE[$temp]="\"".$arrayTE[$temp]."\"";
				}
				elsif($arrayTE[$temp])
				{
					$arrayTE[$temp]="\"".$arrayTE[$temp]."\"";
				}
			}
			if ($arrayTE[$temp] eq "\"\"")
			{
				$arrayTE[$temp]="";
			}
			&toexcel($arrayTE[$temp],2+$temp,'E');
			$arrayTF[$temp] =~ s/\s+/ /g;
			#$arrayTF[$temp]=~s/\&amp\;/\&/g;
			#$arrayTF[$temp]=~s/\&lt\;/</g;
			#$arrayTF[$temp]=~s/\&gt\;/>/g;
			if(not($arrayTF[$temp]=~/^\".*\"$/sgi))
			{
				if(0==$arrayTF[$temp])
				{
					$arrayTF[$temp]="\"".$arrayTF[$temp]."\"";
				}
				elsif($arrayTF[$temp])
				{
					$arrayTF[$temp]="\"".$arrayTF[$temp]."\"";
				}
			}
			if ($arrayTF[$temp] eq "\"\"")
			{
				$arrayTF[$temp]="";
			}
			&toexcel($arrayTF[$temp],2+$temp,'F');
			$arrayTG[$temp] =~ s/\s+/ /g;
			#$arrayTG[$temp]=~s/\&amp\;/\&/g;
			#$arrayTG[$temp]=~s/\&lt\;/</g;
			#$arrayTG[$temp]=~s/\&gt\;/>/g;
			if(not($arrayTG[$temp]=~/^\".*\"$/sgi))
			{
				if(0==$arrayTG[$temp])
				{
					$arrayTG[$temp]="\"".$arrayTG[$temp]."\"";
				}
				elsif($arrayTG[$temp])
				{
					$arrayTG[$temp]="\"".$arrayTG[$temp]."\"";
				}
			}
			if ($arrayTG[$temp] eq "\"\"")
			{
				$arrayTG[$temp]="";
			}
			&toexcel($arrayTG[$temp],2+$temp,'G');
			&toexcel($arrayTH[$temp],2+$temp,'H');
			&toexcel($arrayTI[$temp],2+$temp,'I');
			&toexcel($arrayTJ[$temp],2+$temp,'J');
	}
	&toexcel("XML Path",1,'A');
	&toexcel("XML Line",1,'B');
	&toexcel("Index",1,'C');
	&toexcel("StringOrArrayOrPluralsName",1,'D');
	&toexcel("Value(EN)",1,'E');
	&toexcel("Value(ZH)",1,'F');
	&toexcel($onelanguage,1,'G');
	&toexcel("ArrayOrPlurals",1,'H');
	&toexcel("ProductName",1,'I');
	&toexcel("translate",1,'J');
}
$xlBook->close();


############################################################################################
#以下函数实现判断两个path是不是匹配，即是不是只有.xml所在的目录不一样
#调用方法为：&pathmatchornot($path1,$path2);
#若匹配则返回数字1，不匹配的话返回数字0
############################################################################################
sub pathmatchornot()
{
my (@canshu) = @_;
my $path1 = $canshu[0];
my $path2 = $canshu[1];
$path1 =~ s#\/#\\#sg;
$path2 =~ s#\/#\\#sg;
my @arraypath1 = split /\\/,$path1;
my @arraypath2 = split /\\/,$path2;
my $count1 = @arraypath1;
my $count2 = @arraypath2;
if ($count1 == $count2)
{
	my $temp1 = 0;
	while($arraypath1[$temp1] eq $arraypath2[$temp1])
	{
			$temp1++;
	}
	my $temp2 = $count1 - 1;
	if(($arraypath1[$temp2] eq $arraypath2[$temp2])&&($temp2==(1+$temp1)))
	{
		return 1;
	}
}
return 0;
}
############################################################################################




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
#       if($file=~/strings\.(xml)$/i){
#          print "$dir\/$file \n";
					$dirandfile=$dir."\/".$file;
					$allFileDirectoris[$xuhao++]=$dirandfile;
#      }
   }
   elsif(-d"$dir/$file")
   {
           find_fileindir("$dir/$file" );
   }
 }
}
#########################################################################################################
