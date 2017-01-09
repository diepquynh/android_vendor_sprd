#目的：把送翻译拿回来的excel表格做补充
use strict;
#use warnings;  
use Spreadsheet::XLSX; 
use Excel::Writer::XLSX;
use Data::Dumper;
use XML::Simple;
use File::Basename;
use Encode;
use utf8;
my %translation;
#下面一行的xlsx文件即为送翻回来的excel表格的文件名，根据实际情况修改即可
my $tanslatedFile = 'D:\MUI\Test\tools\MUINeedTranslated.xlsx';  
my $workBook = Spreadsheet::XLSX -> new ($tanslatedFile);  
my @sheets = @{$workBook->{Worksheet} };  
foreach my $sheet ( @sheets ){     
    my $sheetName = $sheet->{Name};        
    $sheet -> {MaxRow} ||= $sheet -> {MinRow};  
    my %everycell; 
    print $sheet -> {MaxRow};
    print $sheet -> {MinRow};       
    foreach my $row ( 1 .. $sheet -> {MaxRow} ){          
        # 取excel第三列每行的值赋给哈希，若没有则取'0'，此列存的是english的value。此语句主要是确保取到值，防止运行时出现无初始化的警告
        $everycell{$sheetName}->{$row}->{'EnglishValue'} = eval { $sheet->get_cell( $row, 3 )->value } || '0';
        # 取excel第六列每行的值赋给哈希，若没有则取'0'，此列存的是多国语言的value.此处存在隐患，若空格里的值本来就是0，怎么处理？
        $everycell{$sheetName}->{$row}->{'MUIValue'} = eval { $sheet->get_cell( $row, 4 )->value }||0;
        $everycell{$sheetName}->{$row}->{'EnStringName'} = eval { $sheet->get_cell( $row, 2 )->value } || '0';
        # 取excel第二，三，六列每行的值赋给哈希，此时取得是该cell里真正的值，若没有则取到的是空
        $translation{$sheetName}->{'path'}->{$row} = eval { $sheet->get_cell( $row, 1 )->value };
        $translation{$sheetName}->{'stringName'}->{$row} = eval { $sheet->get_cell( $row, 2 )->value };
        $translation{$sheetName}->{'stringValue'}->{$row} = eval { $sheet->get_cell( $row, 4 )->value };
        $translation{$sheetName}->{'englishValue'}->{$row} = eval { $sheet->get_cell( $row, 3 )->value };
        $translation{$sheetName}->{'judge'}->{$row} = eval { $sheet->get_cell( $row, 5 )->value };
        $translation{$sheetName}->{'stringType'}->{$row} = eval { $sheet->get_cell( $row, 6 )->value };
        $translation{$sheetName}->{'productName'}->{$row} = eval { $sheet->get_cell( $row, 7 )->value };
        $translation{$sheetName}->{'pluralsQuantityName'}->{$row} = eval { $sheet->get_cell( $row, 8 )->value };
		$translation{$sheetName}->{'Tag'}->{$row} = eval { $sheet->get_cell( $row, 10 )->value };
        if(($translation{$sheetName}->{'judge'}->{$row} eq 'item')&&($everycell{$sheetName}->{$row}->{'MUIValue'} eq '0')){
        $translation{$sheetName}->{'stringValue'}->{$row} = $translation{$sheetName}->{'englishValue'}->{$row};
        }
        #同一excel表格查重操作
        if($translation{$sheetName}->{'judge'}->{$row} eq 'true'){
        if($translation{$sheetName}->{'stringValue'}->{$row} =~ /^".*"$/){
        }else{
        $translation{$sheetName}->{'stringValue'}->{$row} = "\"$translation{$sheetName}->{'stringValue'}->{$row}\"";
        }
        
        }
        if($translation{$sheetName}->{'judge'}->{$row} eq '*repeat*' || $everycell{$sheetName}->{$row}->{'MUIValue'} eq '0' ){
            my $k;
            for ($k = 1; $k < $row; $k++){
                #  eval { $sheet->get_cell( $k, 2 )->value }
                #取第k行的english value和mui value。此语句主要是确保取到值，防止运行时出现无初始化的警告
                $everycell{$sheetName}->{$k}->{'muiValueNext'} = eval { $sheet->get_cell( $k, 4 )->value }||0;
                $everycell{$sheetName}->{$k}->{'englishValueNext'} = eval { $sheet->get_cell( $k, 3 )->value } || '0';
=pod
if($everycell{$sheetName}->{$k}->{'muiValueNext'} =~ /^".*"$/){
        }else{
       $everycell{$sheetName}->{$k}->{'muiValueNext'} = "\"$everycell{$sheetName}->{$k}->{'muiValueNext'}\"";
        }
        $everycell{$sheetName}->{$k}->{'muiValueNext'} = eval { $sheet->get_cell( $k, 4 )->value } || '0';
=cut
                #第$row行的english value不为空，且与第$k行english value相同时
                if($everycell{$sheetName}->{$row}->{'EnglishValue'} ne '0' && ((uc $everycell{$sheetName}->{$row}->{'EnglishValue'})
                        eq (uc $everycell{$sheetName}->{$k}->{'englishValueNext'}))) { 
                # k行的mui value值不为空，把k行的mui value值给row行
                   # if($translation{$sheetName}->{'stringValue'}->{$k}) {
                        $translation{$sheetName}->{'stringValue'}->{$row} =  $translation{$sheetName}->{'stringValue'}->{$k} ;
                    #}

                }
            }     
        }

    } 

} 
#生成的填充好的excel表格的名字，可根据个人情况修改文件名
my $xlBook = Excel::Writer::XLSX->new(
    "MUITranslatedReturn.xlsx") or die "$!\n";
foreach my $everySheet ( @sheets ){     
    my $everySheetName = $everySheet->{Name};  
    my $xlSheet1 = $xlBook->add_worksheet("$everySheetName");
    my $titleFormat = $xlBook->add_format(
        color       => 'white',
        align       => 'center',
        valign      => 'vcenter',
        fg_color    => 'blue',
        font        => 'Arial',
        border    => 1,
        text_wrap   => 1
    );
    my $bodyFormat = $xlBook->add_format(
        color       => 'black',
        border    => 1,
        align       => 'right',
        font        => '宋体'
    );
    my $column = 2;
    my $mm = 1;
    my $lang = eval { $everySheet->get_cell( 0, 4 )->value };
    my $value = $translation{$everySheetName};
    for (my $i=1; $i<=$everySheet -> {MaxRow};$i++){

        my $writeRow;
        my @line;
        $line[0] = "Index";
        $line[1] = "Path";
        $line[2] = "string-name";
        $line[3] = "string-Englishname";
        $line[4] = "$lang";
        $line[5] = "mark";
        $line[6] = "stringType";
        $line[7] = "productName";
        $line[8] = "quantityName";
		$line[9] = "Tag";
        $xlSheet1->write_row($writeRow++, 0, \@line, $titleFormat);
    # 冻结首行
    $xlSheet1->freeze_panes( 1 );
    $xlSheet1->write(
        'A'.$column,
        $mm, $bodyFormat
    );
    $xlSheet1->write(
        'B'.$column,
        $value->{'path'}->{$i}, $bodyFormat
    );
    $xlSheet1->write(
        'C'.$column,
        $value->{'stringName'}->{$i}, $bodyFormat
    );
    $xlSheet1->write(
        'D'.$column,
        decode('utf8',"$value->{'englishValue'}->{$i}"), $bodyFormat
    );

    $xlSheet1->write(
        'E'.$column,
        decode('utf8',"$value->{'stringValue'}->{$i}"), $bodyFormat
    );
    $xlSheet1->write(
        'F'.$column,
        $value->{'judge'}->{$i}, $bodyFormat
    );
    $xlSheet1->write(
        'G'.$column,
        $value->{'stringType'}->{$i}, $bodyFormat
    );
    $xlSheet1->write(
        'H'.$column,
        $value->{'productName'}->{$i}, $bodyFormat
    );
    $xlSheet1->write(
        'I'.$column,
        $value->{'pluralsQuantityName'}->{$i}, $bodyFormat
    );
	 $xlSheet1->write(
        'J'.$column,
        $value->{'Tag'}->{$i}, $bodyFormat
    );
    $mm++;
    $column++;
}
}

