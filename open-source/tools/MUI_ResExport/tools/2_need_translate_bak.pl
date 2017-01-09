#######################################################################################
#Purpose:提取需要翻译的MUI，并生成excel表格，此表格送翻译的时候用，只翻译excel中标记为true的MUI项
#Author：xiuli.fang 
#######################################################################################
use strict;
#use warnings;  
use Locale::Language; 
use Locale::Country;
use Spreadsheet::XLSX; 
use Excel::Writer::XLSX;
use Data::Dumper;
use XML::Simple;
use File::Basename;
use Time::Piece;
use Encode;
use utf8;
my %translation; 
my $hms = qx(time /T);
chomp($hms);
print "begin time:$hms\n";
#@@@@@@@@@@@@@此处的xlsx文件是待筛选的excel表格的文件名
my $file = 'Alllanguagesstrings.xlsx';  
my $book = Spreadsheet::XLSX -> new ($file);  
my @sheets = @{$book->{Worksheet} };  
my $excel;
#@@@@@@@@@@@@@@此处修改总表路径
my $dir = 'D:\MUI\test\zong';
my $dirandfile;
opendir(DIR,"$dir"|| die "can't open this$dir");
my @files =readdir(DIR);
closedir(DIR);
splice (@files,0,2);
my @filepath;
foreach my $file(@files){
    print "$dir\\$file\n";
    push @filepath, "$dir\\$file";
}

foreach my $sheet ( @sheets ){     
    my $sheetName = $sheet->{Name};     
    print "$sheetName\n";    
    $sheet -> {MaxRow} ||= $sheet -> {MinRow};   
    print $sheet -> {MaxRow};
    print $sheet -> {MinRow}; 
    my %everycell;
    my @dealitem;
    my @item;
    my @everyName;
    my %stringXml;
    #存取
    my @same;
    my @dealItem;
    my %repeatDownNull;
    my %languageMatch;
    foreach my $row ( 1 .. $sheet -> {MaxRow} ){
        # 取每行的EnglishValue
        $everycell{$sheetName}->{$row}->{'4'} = eval { $sheet->get_cell( $row, 4 )->value } || '0zero_0';
		# 取每行的ChinesValue
		$everycell{$sheetName}->{$row}->{'5'} = eval { $sheet->get_cell( $row, 5 )->value } || '0zero_0';
        # 取每行的多国语言value
        $everycell{$sheetName}->{$row}->{'6'} = eval { $sheet->get_cell( $row, 6 )->value } || '0zero_0';
        $everycell{$sheetName}->{$row}->{'chinese'} = eval { $sheet->get_cell( $row, 5 )->value } || '0zero_0';
        $everycell{$sheetName}->{$row}->{'judgeArrays'} = eval { $sheet->get_cell( $row, 7 )->value } || '0zero_0';
        $everycell{$sheetName}->{$row}->{'EnStringName'} = eval { $sheet->get_cell( $row, 3 )->value } || '0zero_0';
        # 取相应行的值，在写excel表格时会用到
        $translation{$sheetName}->{'chineseName'}->{$row} = eval { $sheet->get_cell( $row, 5 )->value };
        $translation{$sheetName}->{'stringName'}->{$row} = eval { $sheet->get_cell( $row, 3 )->value };
        $translation{$sheetName}->{'stringValue'}->{$row} = eval { $sheet->get_cell( $row, 6 )->value };
        $translation{$sheetName}->{'englishValue'}->{$row} = eval { $sheet->get_cell( $row, 4 )->value };
        $translation{$sheetName}->{'stringType'}->{$row} = eval { $sheet->get_cell( $row, 7 )->value };
        $translation{$sheetName}->{'productName'}->{$row} = eval { $sheet->get_cell( $row, 8 )->value };
        $translation{$sheetName}->{'path'}->{$row} = eval { $sheet->get_cell( $row, 0 )->value };
        $translation{$sheetName}->{'pluralsQuantityName'}->{$row} = eval { $sheet->get_cell( $row, 2 )->value };
        $translation{$sheetName}->{'noTranslated'}->{$row} = eval { $sheet->get_cell( $row, 9 )->value} || '0zero_0';

        #判断当english value有值，而MUI翻译没有值时，把true赋给$translation{$sheetName}->{'judge'}->{$row}做标记

        if ( $everycell{$sheetName}->{$row}->{'4'} ne '0zero_0' && $everycell{$sheetName}->{$row}->{'5'} ne '0zero_0'){
		    print 
            # $celljudge->value = 'true';

            $translation{$sheetName}->{'judge'}->{$row} = 'true';
        }

        #否则赋予false
        else {
            $translation{$sheetName}->{'judge'}->{$row} = 'false';
            print ">";
        } 
        #对标记为true的情况做进一步处理：看中英文翻译是否相同，是否有'Tfalse'
        if($translation{$sheetName}->{'judge'}->{$row} eq 'true'){

            #英文和中文相等时，不论多国语言是否为空，均不需要翻译，标记为'false'
            if ($everycell{$sheetName}->{$row}->{'4'} ne '0zero_0' && ((uc $everycell{$sheetName}->{$row}->{'chinese'}) eq (uc $everycell{$sheetName}->{$row}->{'4'}))){
                $translation{$sheetName}->{'judge'}->{$row} = 'false';
                print "***";
            }
            #判断是否有'Tfalse'
            if ( $translation{$sheetName}->{'noTranslated'}->{$row} eq 'Tfalse'){
                $translation{$sheetName}->{'judge'}->{$row} = 'false';
                #   print "#";
            }
       }

        #item的情况
        #item有三种情况：string-array, integer-array, plurals. 当'judeg'不为false,且'judgeArrays'为三种情况之一
        if(($translation{$sheetName}->{'judge'}->{$row} ne 'false')&& ($everycell{$sheetName}->{$row}->{'judgeArrays'} ne '0zero_0')){
            #往下查:取下一行的judgeArray，6item(为mui value)，stringName做比较
            my $j = $row + 1;
            $everycell{$sheetName}->{$j}->{'nextJudgeArrays'} = eval { $sheet->get_cell( $j, 7 )->value };
            $everycell{$sheetName}->{$j}->{'6item'} = eval { $sheet->get_cell( $j, 6 )->value } || '0zero_0';
            $everycell{$sheetName}->{$j}->{'itemEnStringName'} =  eval { $sheet->get_cell( $j, 3 )->value } ;
            #如果当前行与下一行的stringName相同, 且'judegArray'为三种情况之一，做下面对处理，如果不满足条件，跳出循环
            while(($everycell{$sheetName}->{$row}->{'EnStringName'} eq $everycell{$sheetName}->{$j}->{'itemEnStringName'}) && ($everycell{$sheetName}->{$j}->{'nextJudgeArrays'}  eq $everycell{$sheetName}->{$row}->{'judgeArrays'})){
                #如果j行的mui value值不为空，先检测@item是否为空，若为空直接把j行push进去，若不为空，检测j行是否已在数组@item内,如果不在，把该行push进去
                if($everycell{$sheetName}->{$j}->{'6item'} ne '0zero_0'){
                    if (@item){
                        if (grep {$_ eq $j} @item) {       
                            print "..-";   
                        }
                        else{
                            push @item, $j;

                        }

                    }
                    else{
                        push @item, $j;
                    }

                }
                #如果j行的mui value值为空，把该j行push进@dealitem，因为为空，也可能是不需要翻译的，比如后面有Tfalse或中英文翻译相同这种情况会被标记为false，在写excel表格时就会被忽略；解决：把mui value为空的行，都放在@dealitem，然后再对@dealitem处理
                else {push @dealitem, $j;
                }
                $j++;
                $everycell{$sheetName}->{$j}->{'nextJudgeArrays'} = eval { $sheet->get_cell( $j, 7 )->value } ;
                $everycell{$sheetName}->{$j}->{'6item'} = eval { $sheet->get_cell( $j, 6 )->value } || '0zero_0';
                $everycell{$sheetName}->{$j}->{'itemEnStringName'} =  eval { $sheet->get_cell( $j, 3 )->value } ;  
            }
            #往上查：取上一行的值做比较，思路和往下查一样
            my $m = $row - 1;
            $everycell{$sheetName}->{$m}->{'nextJudgeArrays'} = eval { $sheet->get_cell( $m, 7 )->value } ;
            $everycell{$sheetName}->{$m}->{'6item'} = eval { $sheet->get_cell( $m, 6 )->value } || '0zero_0';
            $everycell{$sheetName}->{$m}->{'itemEnStringName'} =  eval { $sheet->get_cell( $m, 3 )->value } ;
            while($m >= 0 && ($everycell{$sheetName}->{$row}->{'EnStringName'} eq $everycell{$sheetName}->{$m}->{'itemEnStringName'}) && ($everycell{$sheetName}->{$m}->{'nextJudgeArrays'}  eq $everycell{$sheetName}->{$row}->{'judgeArrays'} )){
                if($everycell{$sheetName}->{$m}->{'6item'} ne '0zero_0'){
                    if (@item){
                        if (grep {$_ eq $m} @item) {       
                            print "-";   
                        }
                        else{
                            push @item, $m;

                        }

                    }
                    else{
                        push @item, $m;
                    }

                }
                else {push @dealitem, $m;}
                $m--;
                $everycell{$sheetName}->{$m}->{'nextJudgeArrays'} = eval { $sheet->get_cell( $m, 7 )->value } ;
                $everycell{$sheetName}->{$m}->{'6item'} = eval { $sheet->get_cell( $m, 6 )->value } || '0zero_0';
                $everycell{$sheetName}->{$m}->{'itemEnStringName'} =  eval { $sheet->get_cell( $m, 3 )->value } ;  
            }
        }

        #同一excel表格查重操作，只对标记为true的row行查重
        if( $translation{$sheetName}->{'judge'}->{$row} eq 'true'){
            my $k;
            for ($k = 1; $k <= $sheet -> {MaxRow}; $k++){
                #  eval { $sheet->get_cell( $k, 2 )->value }
                #取第k行的englishvalue和MUI value。此语句主要是确保取到值，防止运行时出现无初始化的警告
                $everycell{$sheetName}->{$k}->{'sixthNext'} = eval { $sheet->get_cell( $k, 6 )->value } || '0zero_0';
                $everycell{$sheetName}->{$k}->{'forthNext'} = eval { $sheet->get_cell( $k, 4 )->value } || '0zero_0';
                #第$row行有english value值(不为空)，且与第$k行的english value 值相同时，不区分大小写
                if($everycell{$sheetName}->{$row}->{'4'} ne '0zero_0') { 
                    if((uc $everycell{$sheetName}->{$k}->{'forthNext'}) eq (uc $everycell{$sheetName}->{$row}->{'4'})){
                        #第$row行与第$k行的翻译值均为空(row行和k行的多国语言翻译均为空)时，把$k push到@same数组里，@same记录的是有重复的行数
                        if( $everycell{$sheetName}->{$row}->{'6'} eq '0zero_0' &&  $everycell{$sheetName}->{$k}->{'sixthNext'} eq '0zero_0'){
                            #k值大于row时，把k值push到数组里；当k<row时，如果再push到数组里，会导致两个true的情况都被push到数组，然后在下面全标为repeat
                            if($k > $row){
                                push @same, $k;}
                        }
                        #第$row行翻译值为空，第$k行不为空时，把第k行里的值赋给第row行 
                        if( $everycell{$sheetName}->{$row}->{'6'} eq '0zero_0' && ($everycell{$sheetName}->{$k}->{'sixthNext'} ne '0zero_0'))
                        {
                            $translation{$sheetName}->{'stringValue'}->{$row} = eval { $sheet->get_cell( $k, 6 )->value };
                            #  print  $translation{$sheetName}->{'stringValue'}->{$row};
                            push @same, $row;
                        }
                    }
                }
            }   
        }  
    }         

    #把@same里的值分别取出(即重复的值)，通过哈希标记为重复
    foreach my $samerow (@same) {
        if( $translation{$sheetName}->{'judge'}->{$samerow} eq 'true'){

            $translation{$sheetName}->{'judge'}->{$samerow} = '*repeat*';
        }
    }
    foreach my $itemNum (@item) {

        $translation{$sheetName}->{'judge'}->{$itemNum} = 'item';
        #print $itemNum;

    }
    #@dealitem处理：mui值不为空的列，如果该列的'judge'标记为false，在这改写为item，否则在写excel表格时会写不上，致使array的情况写不全（因为写的时候不写false的情况）
    foreach my $dealitemCase (@dealitem){
        if ($translation{$sheetName}->{'judge'}->{$dealitemCase} eq 'false'){
            # $celljudge->value = 'true';
            $translation{$sheetName}->{'judge'}->{$dealitemCase} = 'item';
            print "***++";
        }
    }
} 

my $xlBook = Excel::Writer::XLSX->new(
    "MUIStringsNeedTranslated.xlsx") or die "$!\n";
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
foreach my $everySheet ( @sheets ){     
    my $everySheetName = $everySheet->{Name};  
    my $xlSheet1 = $xlBook->add_worksheet("$everySheetName");

    my $mm = 1;
    my $column = 2;
    my $abbLanguage;
    my $lang;
    my $abbcountry;
    #根据sheet名，匹配出语言缩写
    if($everySheetName =~ /values-(\w\w)$/){
        $abbLanguage = $1; 
        #根据缩写得到全称
        if($abbLanguage eq "iw"){
            $abbLanguage = "he";
            $lang = code2language($abbLanguage);
        }
        elsif($abbLanguage eq "in"){
            $abbLanguage = "id";
            $lang = code2language($abbLanguage);
        }
        elsif($abbLanguage eq "ce"){
            $abbLanguage = "ceb";
            $lang = "Cebuano";
        }else{
            #根据缩写得到全称
            $lang = code2language($abbLanguage);
        }
        print "\n$lang\n";
    }
    #根据sheet名匹配出语言缩写和地区缩写
    if($everySheetName =~ /values-(\w\w)-(\w\w\w)-?(.*)/){
        my $abbLanguage2 = code2language($1);
        if($2 =~ /r(\w\w)/){
            $abbcountry = code2country($1);
            #根据缩写得到语言全称和地区全称，组合成语言名
            $lang = "$abbLanguage2-$abbcountry"; }
        else{
            $lang = "$abbLanguage2";
        }
        print "\n$lang\n";
    }

    my $mui = "$lang\.xlsx";
    my $muipath = "$dir\\$mui";
    my $value = $translation{$everySheetName};
    for (my $i=1; $i<= $everySheet -> {MaxRow};$i++){
        #只提取标记为'*repeat*'和'true'的值
        if ($value->{'judge'}->{$i} eq '*repeat*' || $value->{'judge'}->{$i} eq 'true' || $value->{'judge'}->{$i} eq 'item' ) {
            if($value->{'judge'}->{$i} eq '*repeat*' || $value->{'judge'}->{$i} eq 'true'){

                #与另一个excel表格中的值做查重处理，取出表格中第0行的值（包含各国语言），去匹配，当值和上面的语言名相同时，做查重
				if ($muipath ~~ @filepath){
                $excel = Spreadsheet::XLSX -> new ($muipath);
                foreach my $sheet2 ( @{$excel -> {Worksheet}} ){     
                    #my $sheetName2 = $sheet2->{Name};   
                    # for(my $n = 0; $n <= 31; $n++) {
                    #    my $languageName = eval { $sheet2->get_cell( 0, $n )->value}||'0zero_0';
                    #  if ((uc $lang) eq (uc $sheetName2)) {
                    #  if( $sheetName2 eq $languageName)){
                    #  print "*$n\n";
                    $sheet2 -> {MaxRow} ||= $sheet2 -> {MinRow};      
                    foreach my $row2 ( 1 .. $sheet2 -> {MaxRow} ){   
                        my $compareEnglishValue = eval { $sheet2->get_cell( $row2, 2 )->value } ||'0zero_0';
                        my $compareMuiValue = eval { $sheet2->get_cell( $row2, 4 )->value } || '0zero_0';
                        #第一个表格中的值都有双引号，此对比表格中的值没有双引号时要加上
                        if($compareEnglishValue =~ /^"(.*)"$/){

                        }else{
                            $compareEnglishValue ="\"$compareEnglishValue\"";
                        }
						 $compareEnglishValue =~s/&amp\;/&/g;
                        #如果第一个表格中某个sheet中的english value和对比表格中相同语言列下的english value相同(不区分大小写)，把该值赋给第一个表                                  格中的相应mui
                        if ((uc $compareEnglishValue) eq (uc $value->{'englishValue'}->{$i})) {
                            if($compareMuiValue ne '0zero_0'){
                                $value->{'stringValue'}->{$i} = $compareMuiValue;
                                $value->{'judge'}->{$i} = 'translated';
                            }
                        }
                    }
                }
			}
        }
            my $writeRow;
            my @line;
            $line[0] = "Index";
            $line[1] = "Path";
            $line[2] = "string-name";
            $line[3] = "string-Englishname";
            $line[4] = "$lang" || "$everySheetName";
            $line[5] = "mark";
            $line[6] = "stringType";
            $line[7] = "productName";
            $line[8] = "quantityName";
            $line[9] = "ChineseName";
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
        if($value->{'pluralsQuantityName'}->{$i} =~ /Element\d+/){
            $xlSheet1->write(
                'I'.$column,
                "", $bodyFormat
            );
        }
        else{
            $xlSheet1->write(
                'I'.$column,
                $value->{'pluralsQuantityName'}->{$i}, $bodyFormat
            );
        }
        $xlSheet1->write(
            'J'.$column,
            decode('utf8',"$value->{'chineseName'}->{$i}"), $bodyFormat
        );
        $mm++;
        $column++;
    }
}
}
$hms = qx(time /T);
chomp($hms);
print "finish time:$hms\n";
