#!/usr/bin/perl
#把excel按路径导入相应xml
#use strict;
#use warnings;  
use Spreadsheet::XLSX;
use Excel::Writer::XLSX;
use XML::Writer;
use Data::Dumper;
use XML::Simple;
use File::Basename;
#use XML::Encoding;
use Encode;
use utf8;
use IO::File;
use Cwd;
use File::Path qw(make_path remove_tree);
#解析excel
my $file = 'MUITranslatedReturn.xlsx';  
my $book = Spreadsheet::XLSX -> new($file);
my @sheets = @{$book->{Worksheet} };  
my %translation;
our $writer;
#定义全局变量
our $row;
foreach my $sheet (@sheets ){ 
    #全局变量每次清1;
    $row = 1;
   # for(my $row =1; $row < $sheet -> {MaxRow};$row++){
    while( $row <= $sheet -> {MaxRow}){
        my $sheetName = $sheet->get_name();
        my $oldPath = eval { $sheet->get_cell( $row, 1 )->value };
        my $Tag = eval { $sheet->get_cell( $row, 9 )->value };
        my $currentPath = $oldPath; 
        my $localPath = getcwd;
        #路径里Android前面的路径替换为当前路径
       if( $oldPath =~ /(.*?)(resall.*)/){
        $oldPath = $2;   
        $oldPath ="$localPath//OUT//$2";  
       }         
       #新建路径和xml文件 
        if($oldPath =~ /(.*)\\(.*\.xml)$/ ){ #split xml's path and xml's name
            my $dir = $1;   
            make_path ("$dir");  # creat xml path
            my $output = new IO::File(">$dir\\$2");
            $writer = new XML::Writer(OUTPUT => $output, DATA_INDENT => 2); #creat xml 
            $writer->xmlDecl('UTF-8');	
=pod
            $writer->comment('Copyright (C) 2006 The Android Open Source Project
     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at
          http://www.apache.org/licenses/LICENSE-2.0
     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.');
=cut
           if($Tag==1){
            $writer->startTag("resources" ,'xmlns:android'=>'http://schemas.android.com/apk/res/android', "\nxmlns:xliff"=>'urn:oasis:names:tc:xliff:document:1.2');
            }
			else{
			$writer->startTag("resources" ,'xmlns:xliff'=>'urn:oasis:names:tc:xliff:document:1.2');
			} 
            $writer->characters("\n");
            #调用子函数，对当前行的内容进行xml处理
            &writeXml($sheet,$sheetName,$row);
            #调用子函数后row行的值可能发生了改变，子函数采用的是引用方式传值，提取出此时的row行和row+1（row+1不会改变row的值）行的路径
            my $nextPath = eval { $sheet->get_cell( $row + 1, 1 )->value };
            $currentPath = eval { $sheet->get_cell( $row, 1 )->value };
            #same Path continue write; 不同路径时跳出此循环，row++,到上一个while处，重新建立路径和xml文件
            while (($currentPath eq $nextPath) && ($currentPath ne "")){
                $row ++;
            #路径相同，把row++（row的值比进来时加了1），调用子函数，对此时row行的内容做处理
               &writeXml($sheet,$sheetName,$row );     
               #调用子函数后，row行的值可能发生了改变，此时重新取row行和row+1行的路径
                $nextPath = eval { $sheet->get_cell( $row + 1, 1 )->value }; 
                $currentPath = eval { $sheet->get_cell( $row, 1 )->value };   
            }
            #跳出该循环，意味着下一个的路径不同了，该路径下的xml文件写完了，加上结束tag
            $writer->endTag("resources");
            $writer->end();
            $output->close();

        }
           $row++;
    }
}

# brief: write xml from excel;
#param0: every sheet;
#param1: sheet names;
#param2: sheet row;
#e.g.  : &writeXml($sheet,$sheetName,$row);
sub writeXml {
    $translation{$_[1]}->{'stringName'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 2 )->value } ;
    $translation{$_[1]}->{'stringValue'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 4 )->value } ;
    $translation{$_[1]}->{'englishValue'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 3 )->value } ;
    $translation{$_[1]}->{'stringType'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 6 )->value };
    $translation{$_[1]}->{'productName'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 7 )->value } ;
    $translation{$_[1]}->{'quantityName'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 8 )->value } ;

    #string simple type;
    if( $translation{$_[1]}->{'stringType'}->{$_[2]} eq "" && $translation{$_[1]}->{'productName'}->{$_[2]} eq "" ){
        $writer->startTag("string", name => $translation{$_[1]}->{'stringName'}->{$_[2]});
        $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2]});
        $writer->endTag("string");
        $writer->characters("\n");
    }

    #string add product type;
    elsif($translation{$_[1]}->{'stringType'}->{$_[2]} eq "" && $translation{$_[1]}->{'productName'}->{$_[2]} ne ""){
        $writer->startTag("string", name => $translation{$_[1]}->{'stringName'}->{$_[2]}, product => $translation{$_[1]}->{'productName'}->{$_[2]});
        $writer->characters("$translation{$_[1]}->{'stringValue'}->{$_[2]}");
        $writer->endTag("string");
        $writer->characters("\n");
    }

    #string arrays type;
    elsif ($translation{$_[1]}->{'stringType'}->{$_[2]} =~ /string-array/){
        #arrays without product type;
        if( $translation{$_[1]}->{'productName'}->{$_[2]} eq ""){
            $writer->startTag("string-array", name => $translation{$_[1]}->{'stringName'}->{$_[2]});
            $writer->characters("\n");
        }
        # arrays with product;
        else{
            $writer->startTag("string-array", name => $translation{$_[1]}->{'stringName'}->{$_[2]}, product => $translation{$_[1]}->{'productName'}->{$_[2]} );
            $writer->characters("\n");
        }
       #把当前行的翻译值写到item里
        $writer->startTag('item');
        $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2]});
        $writer->endTag("item");
        $writer->characters("\n");
        my $checkSameArray;
        my $rowFlag=0;
        $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
        $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;
       
        #判断下一行的stringName与当前行的是否相同，若相同，为同一array下的item，把下一行的翻译值写到一个item里，并把行数++；直到不同时跳出该循环 
        while ($checkSameArray eq $translation{$_[1]}->{'stringName'}->{$_[2]} && $translation{$_[1]}->{'stringType'}->{$_[2] + 1} eq 'string-array'){               $translation{$_[1]}->{'stringValue'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2]+ 1, 4 )->value };
            $writer->startTag("item" );
            $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2] + 1 });
            $writer->endTag("item");
            $writer->characters("\n");
            # 此处++，会改变主函数里的row值，目的为：既为同一array下的，每一行做处理时，row行的值++
            $_[2] ++;
           $rowFlag = 1;
            $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
            $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;
            $translation{$_[1]}->{'stringName'}->{$_[2]} =  eval { $_[0]->get_cell( $_[2], 2 )->value } ;
        }
=pod
        if($rowFlag == 1){
         $_[2] = $_[2]-1;
     }
=cut
        $writer->endTag("string-array");
        $writer->characters("\n");
        #   print " $_[2] \n"
    }
    # integer-array type
     elsif ($translation{$_[1]}->{'stringType'}->{$_[2]} =~ /integer-array/){
         # 写上integer-array的tag行
         #arrays without product type;
        if( $translation{$_[1]}->{'productName'}->{$_[2]} eq ""){
            $writer->startTag("integer-array", name => $translation{$_[1]}->{'stringName'}->{$_[2]});
            $writer->characters("\n");
        }
        # arrays with product;
        else{
            $writer->startTag("integer-array", name => $translation{$_[1]}->{'stringName'}->{$_[2]}, product => $translation{$_[1]}->{'productName'}->{$_[2]} );
            $writer->characters("\n");
        }
       #写上该上integer-array下的第一个item；只要是array的情况，至少会有一个item
        $writer->startTag('item');
        $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2]});
        $writer->endTag("item");
        $writer->characters("\n");
        my $checkSameArray;
        my $rowFlag=0;
        $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
        $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;

        # 判断下一行的stringName与当前行的stringName是否相同，若相同，判定为同一个array的item，写一个新的item进来，并把行数++；当不同时，跳出该array
        while ($checkSameArray eq $translation{$_[1]}->{'stringName'}->{$_[2]} && $translation{$_[1]}->{'stringType'}->{$_[2] + 1} eq 'integer-array'){               $translation{$_[1]}->{'stringValue'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2]+ 1, 4 )->value };
            $writer->startTag("item" );
            $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2] + 1 });
            $writer->endTag("item");
            $writer->characters("\n");
            # $row++;
            $_[2] ++;
           $rowFlag = 1;
            $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
            $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;
            $translation{$_[1]}->{'stringName'}->{$_[2]} =  eval { $_[0]->get_cell( $_[2], 2 )->value } ;
        }
=pod
        if($rowFlag == 1){
         $_[2] = $_[2]-1;
     }
=cut
        $writer->endTag("integer-array");
        $writer->characters("\n");
        #  print " $_[2] \n"
    }
       # array type
     elsif ($translation{$_[1]}->{'stringType'}->{$_[2]} =~ /^array/){
         # 写上array的tag行
         #array without product type;
        if( $translation{$_[1]}->{'productName'}->{$_[2]} eq ""){
            $writer->startTag("array", name => $translation{$_[1]}->{'stringName'}->{$_[2]});
            $writer->characters("\n");
        }
        # array with product;
        else{
            $writer->startTag("array", name => $translation{$_[1]}->{'stringName'}->{$_[2]}, product => $translation{$_[1]}->{'productName'}->{$_[2]} );
            $writer->characters("\n");
        }
       #写上该array下的第一个item；只要是array的情况，至少会有一个item
        $writer->startTag('item');
        $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2]});
        $writer->endTag("item");
        $writer->characters("\n");
        my $checkSameArray;
        my $rowFlag=0;
        $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
        $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;

        # 判断下一行的stringName与当前行的stringName是否相同，若相同，判定为同一个array的item，写一个新的item进来，并把行数++；当不同时，跳出该array
        while ($checkSameArray eq $translation{$_[1]}->{'stringName'}->{$_[2]} && $translation{$_[1]}->{'stringType'}->{$_[2] + 1} eq 'array'){               $translation{$_[1]}->{'stringValue'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2]+ 1, 4 )->value };
            $writer->startTag("item" );
            $writer->characters($translation{$_[1]}->{'stringValue'}->{$_[2] + 1 });
            $writer->endTag("item");
            $writer->characters("\n");
            # $row++;
            $_[2] ++;
           $rowFlag = 1;
            $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
            $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;
            $translation{$_[1]}->{'stringName'}->{$_[2]} =  eval { $_[0]->get_cell( $_[2], 2 )->value } ;
        }
=pod
        if($rowFlag == 1){
         $_[2] = $_[2]-1;
     }
=cut
        $writer->endTag("array");
        $writer->characters("\n");
        #  print " $_[2] \n"
    }

    #plurals type
    elsif($translation{$_[1]}->{'stringType'}->{$_[2]} =~ /plurals/){
        #plurals without product
        if( $translation{$_[1]}->{'productName'}->{$_[2]} eq ""){
            $writer->startTag("plurals", name => $translation{$_[1]}->{'stringName'}->{$_[2]});
            $writer->characters("\n");
        }
        #plurals with product
        else{
            $writer->startTag("plurals", name => $translation{$_[1]}->{'stringName'}->{$_[2]}, product => $translation{$_[1]}->{'productName'}->{$_[2]} );
            $writer->characters("\n");
        }
        #写下第一个item，item里的名字为quantity
        $writer->startTag("item", quantity => $translation{$_[1]}->{'quantityName'}->{$_[2]});
        $writer->characters("$translation{$_[1]}->{'stringValue'}->{$_[2]}");
        $writer->endTag("item");
        $writer->characters("\n");
        my $rowFlag1 =0;
        my $checkSameArray;
        $checkSameArray = eval { $_[0]->get_cell( $_[2] + 1, 2 )->value } ;
        $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;
        # 判断接下来一行的stringName和当前行是否相同，若相同，则为同一个plurals下的，再写一个item，翻译值为下一行的值；并把行数++；一直到不同时，跳出该循环
        while ($checkSameArray eq $translation{$_[1]}->{'stringName'}->{$_[2]} && $translation{$_[1]}->{'stringType'}->{$_[2] + 1} eq 'plurals'){
            $translation{$_[1]}->{'quantityName'}->{$_[2] + 1} = eval { $_[0]->get_cell( $_[2] + 1, 8 )->value } ;
            $translation{$_[1]}->{'stringValue'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2]+ 1, 4 )->value } ;
            $writer->startTag("item", quantity => $translation{$_[1]}->{'quantityName'}->{$_[2] + 1});
            $writer->characters("$translation{$_[1]}->{'stringValue'}->{$_[2] + 1}");
            $writer->endTag("item");
            $writer->characters("\n");
            # $row++;
            $_[2]++;
             $rowFlag1 = 1;
            $checkSameArray = eval {$_[0]->get_cell( $_[2] +1 , 2 )->value } ;
            $translation{$_[1]}->{'stringType'}->{$_[2] + 1} =  eval { $_[0]->get_cell( $_[2] + 1, 6 )->value } ;
            $translation{$_[1]}->{'stringName'}->{$_[2]} = eval { $_[0]->get_cell( $_[2], 2 )->value } ;
        }
=pod
        if($rowFlag1 == 1){
        $_[2] = $_[2] -1;
        }
=cut
        $writer->endTag("plurals");
        $writer->characters("\n");
    }

    else{
        print "you forget $_[2]\n";
    }   
}


