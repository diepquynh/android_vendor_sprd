#!/usr/bin/perl

#----------------------------------------------#
# Only packet for GA1 Now
#----------------------------------------------#

my $number = scalar @ARGV;
my $i=0;
my $start = time();

print "\n\n--------------------------------------\n";
for($i=0;$i<$number;$i++)
{
	print ${ARGV[$i]}." "
}
print "\n--------------------------------------\n\n";

if($number ne 25){

    die "\nInvalid parameters: $number, param number must be 25.\n\n";
}

my $pac_file       =${ARGV[0]};
my $pac_prj        =${ARGV[1]};
my $pac_ver        =${ARGV[2]};
my $config         =${ARGV[3]};

my $index = 4;
my @param = (
   #ID              , file_path          , file_flag, check_flag, omit_flag, addr1     , addr2
   ["FDL"           , ${ARGV[$index+0]}  , 0x0101   , 1,        , 0        , 0x50000000, 0xFFFFFFFF  ],
   ["FDL2"          , ${ARGV[$index+1]}  , 0x0101   , 1,        , 0        , 0x80000000, 0xFFFFFFFF  ],
   ["NV_WLTE"       , ${ARGV[$index+2]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["ProdNV"        , ${ARGV[$index+3]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["PhaseCheck"    , ""                 , 0        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["EraseUBOOT"    , ""                 , 0        , 1,        , 1        , 0x0       , 0xFFFFFFFF  ],
   ["SPLLoader"     , ${ARGV[$index+4]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["Modem_WLTE"    , ${ARGV[$index+5]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["DSP_WLTE_LTE"  , ${ARGV[$index+6]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["DSP_WLTE_GGE"  , ${ARGV[$index+7]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["WARM_WLTE"     , ${ARGV[$index+8]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["DFS"           , ${ARGV[$index+9]}  , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["FDL_WCN"       , ${ARGV[$index+10]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["Modem_WCN"     , ${ARGV[$index+11]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["BOOT"          , ${ARGV[$index+12]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["DT"            , ${ARGV[$index+13]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["Recovery"      , ${ARGV[$index+14]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["System"        , ${ARGV[$index+15]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["UserData"      , ${ARGV[$index+16]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["BootLogo"      , ${ARGV[$index+17]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["Fastboot_Logo" , ${ARGV[$index+18]} , 1        , 1,        , 1        , 0x0       , 0xFFFFFFFF  ],
   ["Cache"         , ${ARGV[$index+19]} , 1        , 1,        , 1        , 0x0       , 0xFFFFFFFF  ],
   ["FLASH_WLTE"    , ""                 , 0        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["EraseMisc"     , ""                 , 0        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   ["UBOOTLoader"   , ${ARGV[$index+20]} , 1        , 1,        , 0        , 0x0       , 0xFFFFFFFF  ],
   #config must be the last one
   [""              , $config            , 2        , 0,        , 0        , 0x0       , 0xFFFFFFFF  ],
);

my $file_count = scalar @param;

print "\n file count: $file_count\n";

my @crc16_table = (
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 );

die "parameter error" if !CheckParam();

my $fhTarget;
open($fhTarget, "+>$pac_file") or die "Can't open $pac_file";
binmode $fhTarget;


#[[ pack file header
my $szVersion; 			# packet struct version, unicode, total size is 48 bytes
my $dwSize;           		# the whole packet size, 4 bytes
my $szPrdName;   		# product name, total size is 512 bytes
my $szPrdVersion;       	# product version, total size is 512 bytes
my $nFileCount;         	# the number of files that will be downloaded, the file may be an operation, 4 bytes
my $dwFileOffset;		# the offset from the packet file header to the array of FILE_T struct buffer, 4 bytes
my $dwMode;			# 4 bytes
my $dwFlashType;		# 4 bytes
my $dwNandStrategy;		# 4 bytes
my $dwIsNvBackup;		# 4 bytes
my $dwNandPageType;		# 4 bytes
my $szPrdAlias;    		# 200 bytes
my $dwOmaDmProductFlag;		# 4 bytes
my $dwIsOmaDM;			# 4 bytes
my $dwIsPreload;		# 4 bytes
my $dwReserved;			# 800 bytes
my $dwMagic;			# 4 bytes
my $wCRC1;			# 2 bytes
my $wCRC2;			# 2 bytes
#]] total 2124 bytes

$tmp = MakeUnicode("BP_R1.0.0");
do{use bytes; $szVersion = $tmp.(pack("C",0) x (48 - length($tmp)));};

$dwSize = GetDWORD(0);

$tmp = MakeUnicode($pac_prj);
do{use bytes;$szPrdName = $tmp.(pack("C",0) x (512 - length($tmp)));};

$tmp = MakeUnicode($pac_ver);
do{use bytes;$szPrdVersion = $tmp.(pack("C",0) x (512 - length($tmp)));};

$nFileCount = GetDWORD($file_count);

$dwFileOffset= GetDWORD(2124);

$dwMode = GetDWORD(0);

$dwFlashType= GetDWORD(1);
$dwNandStrategy= GetDWORD(0);
$dwIsNvBackup= GetDWORD(1);
$dwNandPageType= GetDWORD(0);

$tmp = MakeUnicode($pac_prj);
do{use bytes;$szPrdAlias = $tmp.(pack("C",0) x (200 - length($tmp)));};

$dwOmaDmProductFlag= GetDWORD(0);
$dwIsOmaDM= GetDWORD(1);
$dwIsPreload= GetDWORD(1);
$dwReserved = pack("C",0) x 800;
$dwMagic= GetDWORD(0xFFFAFFFA);
$wCRC1= GetWORD(0);
$wCRC2= GetWORD(0);

my $offset = 2124;
$offset += ($file_count * 2580);

my $cur_offset = $offset;

do{
use bytes;
print $fhTarget $szVersion;
print $fhTarget $dwSize;
print $fhTarget $szPrdName;
print $fhTarget $szPrdVersion;
print $fhTarget $nFileCount;
print $fhTarget $dwFileOffset;
print $fhTarget $dwMode;
print $fhTarget $dwFlashType;
print $fhTarget $dwNandStrategy;
print $fhTarget $dwIsNvBackup;
print $fhTarget $dwNandPageType;
print $fhTarget $szPrdAlias;
print $fhTarget $dwOmaDmProductFlag;
print $fhTarget $dwIsOmaDM;
print $fhTarget $dwIsPreload;
print $fhTarget $dwReserved;
print $fhTarget $dwMagic;
print $fhTarget $wCRC1;
print $fhTarget $wCRC2;
};

for($i=0; $i<$file_count; $i++)
{
   WriteFileInfoHeader($fhTarget,$i);
}

for($i=0; $i<$file_count; $i++)
{
   WriteDLFile($fhTarget,$i);
}


seek $fhTarget,48,SEEK_SET;

$dwSize = GetDWORD($offset);

print $fhTarget $dwSize;

close $fhTarget;

die "\ncompute crc failed.\n" if (!CalcCrc());

my $end = time();

print "\n---------------------------\n";
print "$pac_file\n";
print "\ndo packet success\n\n";
print "total size: $offset, total time: " . ($end-$start) . "s\n";
print "---------------------------\n\n";

0;

sub MakeUnicode{
    my ($d) = @_;
    $d =~ s/(.)/$1\x{00}/g;
    return $d;
}

sub GetDWORD{
    my ($d) = @_;
    #use bytes;
    $d = pack("V",$d);
    return $d;
}

sub GetWORD{
    my ($d) = @_;
    #use bytes;
    $d = pack("v",$d);
    return $d;
}


sub WriteFileInfoHeader
{
    my $fh = shift;
	my $index = shift;
	my $id = $param[$index][0];
	my $file_path = $param[$index][1];
    my $file_flag = $param[$index][2];
	my $check_flag = $param[$index][3];
    my $omit_flag = $param[$index][4];
	my $addr = $param[$index][5];
    my $addr2 = $param[$index][6];

	my $data;
	my $temp;

	my $dwSize;		# size of this struct itself
	my $szFileID;		# file ID,such as FDL,Fdl2,NV and etc. 512 bytes
	my $szFileName;    	# file name,in the packet bin file,it only stores file name. 512 bytes
                        # but after unpacketing, it stores the full path of bin file
	my $szFileVersion;  	# Reserved now. 512 bytes
	my $nFileSize;          # file size
	my $nFileFlag;          # if "0", means that it need not a file, and
	                        # it is only an operation or a list of operations, such as file ID is "FLASH"
	                        # if "1", means that it need a file
	my $nCheckFlag;         # if "1", this file must be downloaded;
	                        # if "0", this file can not be downloaded;
	my $dwDataOffset;       # the offset from the packet file header to this file data
	my $dwCanOmitFlag;	# if "1", this file can not be downloaded and not check it as "All files"
				# in download and spupgrade tool.
	my $dwAddrNum;
	my $dwAddr;		# 4*5 bytes
	my $dwReserved;     	# Reserved for future,not used now. 249x4 bytes


	$dwSize = GetDWORD(2580);
	$data = $data.$dwSize;

	$tmp = MakeUnicode($id);
	do{use bytes;$szFileID = $tmp.(pack("C",0) x (512 - length($tmp)));};
	$data = $data.$szFileID;

	$tmp = $file_path;
	$tmp =~ s/.*\/(.*)$/$1/g;
        $tmp = MakeUnicode($tmp);
	do{use bytes;$szFileName = $tmp.(pack("C",0) x (512 - length($tmp)));};
	$data = $data.$szFileName;

	$szFileVersion = pack("C",0) x 512;
	$data = $data.$szFileVersion;

	my $file_size = 0;
	if($file_flag != 0 && ($check_flag != 0 || $file_flag == 2))
	{
		$file_size = -s $file_path;
		$nFileSize = GetDWORD($file_size);
	}
	else
	{
		$nFileSize = GetDWORD(0);
	}

	$data = $data.$nFileSize;

	$nFileFlag = GetDWORD($file_flag);
	$data = $data.$nFileFlag;

	$nCheckFlag = GetDWORD($check_flag);
	$data = $data.$nCheckFlag;

	if($file_flag != 0 && ($check_flag != 0 || $file_flag == 2))
	{
		$dwDataOffset = GetDWORD($offset);
	}
        else
	{
		$dwDataOffset = GetDWORD(0);
	}
	$data = $data.$dwDataOffset;

	$dwCanOmitFlag = GetDWORD($omit_flag);
	$data = $data.$dwCanOmitFlag;

	if($file_flag != 2)
	{
		if($addr2 != 0xFFFFFFFF)
		{
			$dwAddrNum = GetDWORD(2);
		    $data = $data.$dwAddrNum;

		    my $dwAddr=GetDWORD($addr);
			$data = $data.$dwAddr;

			my $dwAddr2=GetDWORD($addr2);
			$data = $data.$dwAddr2;

			my $dwAddr3 = pack("C",0) x 12;
			$data = $data.$dwAddr3;
		}
		else
		{
			$dwAddrNum = GetDWORD(1);
		    $data = $data.$dwAddrNum;

		    my $dwAddr=GetDWORD($addr);
			$data = $data.$dwAddr;

			my $dwAddr3 = pack("C",0) x 16;
			$data = $data.$dwAddr3;
		}
	}
	else
	{
		my $dwTmp = pack("C",0) x 24;
		$data = $data.$dwTmp;
	}

	my $dwReserved= pack("C",0) x (249*4);  #249x4 bytes
	$data = $data.$dwReserved;

	$offset += $file_size;
	#$offset = (($offset+3) & 0xFFFFFFFC);
	#print "\n--- $offset ----- \n";

	use bytes;
	print $fh $data;
}

sub WriteDLFile
{
    my $fh = shift;
    my $index = shift;
    my $file_path = $param[$index][1];

    my $len;
    $len = do{use bytes; length $file_path};
    return 0 if ($len == 0 || !(-e $file_path)) ;


    open(FILE, "<$file_path") or die "Can't open $file_path";
    binmode FILE;

    my $file_size = -s $file_path;
    print " deal with [$file_path]: file size: $file_size ...\n";
    my $max_size = 1024*1024*50;
    my $left = $file_size;
    my $buf;

    do{
	if($left > $max_size)
	{
	     $len = $max_size;
	}
	else
        {
	     $len = $left;
	}

	use bytes;
	read FILE,$buf,$len;

	#print $fh pack("C" x $len,$buf);
        print $fh $buf;

	$left -=  $len;

    }while($left>0);

    #my $align = (($file_size+3) & 0xFFFFFFFC) - $file_size ;
    #if($align>0)
    #{
    #	print $fh (pack("C",0) x $align);
    #}
}

sub CalcCrc
{
	open(TARGET, "+<$pac_file") or die "Can't open $pac_file";
	binmode TARGET;

	my $buf;

	print "\ncrc first part...\n";
	seek TARGET,0,SEEK_SET;

	my $wCRC1 = 0;
	read(TARGET,$buf,2120);
	my @part1 = unpack("C" x 2120,$buf);
	$wCRC1 = crc16($wCRC1,@part1);
	@part1=();
	$wCRC1 = GetWORD($wCRC1);
	seek TARGET,2120,SEEK_SET;
	print TARGET $wCRC1;


	print "\ncrc second part...\n";
	seek TARGET,2124,SEEK_SET;
	my $wCRC2 = 0;

	my $size = $offset-2124;
	my $max_size = 1024*1024;
	my $left = $size;

	do{
		if($left > $max_size)
		{
		     $len = $max_size;
		}
		else
		{
		     $len = $left;
		}
		read TARGET,$buf,$len;

		my @part = unpack("C" x $len,$buf);
		$wCRC2 = crc16($wCRC2,@part);
		@part = ();

		$left -=  $len;
	 }while($left>0);

	$wCRC2 = GetWORD($wCRC2);
	seek TARGET,2122,SEEK_SET;
	print TARGET $wCRC2;

	close TARGET;
	return True;
}

sub crc16
{
	my $crc    = shift;
    my @data   = @_;

	foreach $b (@data) {$crc = (($crc >> 8)^( ${ crc16_table[ ($crc^$b) & 0xff] } )) & 0xFFFF;}

    return $crc & 0xFFFF;
}

sub CheckParam
{
	my $j;
	my $error_msg = "";
	for ($j = 0; $j < $file_count; $j++)
	{
		my $id = $param[$j][0];
		my $file_path = $param[$j][1];
        my $file_flag = $param[$j][2];
		my $check_flag = $param[$j][3];
        my $omit_flag = $param[$j][4];

		if($file_flag == 0x0101)
		{
			if(!(-e $file_path))
			{
				$error_msg += "$id: [$file_path] does not exist\n";
			}
			else
			{
				$param[$j][3] = 1;
				$param[$j][4] = 0;
			}
		}
		elsif($file_flag == 1)
		{
			if(!(-e $file_path))
			{
				if($omit_flag == 0)
				{
					$error_msg += "$id: [$file_path] does not exist\n";
				}
			        else
				{
					$param[$j][3] = 0;
				}
			}
		}
		elsif($file_flag == 1)
		{
			$param[$j][1] = "";
		}
	}

	if(length($error_msg)>0)
	{
		print $error_msg;
		return False;
	}

	return True;

}



