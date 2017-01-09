#!/usr/bin/perl
#
# $Id: rebirth.pl 61787 2012-09-10 14:47:26Z michael.ammann $
# $HeadURL: http://svn.u-blox.ch/GPS/SOFTWARE/hmw/android/trunk/gps/rebirth.pl $
#
# This script updates the necessary files on the target.
# Use -d flag to enable debugging.

use strict;
use warnings;

use FindBin;

# enable debugging if the -d flag is specified on the command line
my $debug = ("@ARGV" =~ m/-d/ ? 1 : 0);

# find adb binary, fall back to assuming 'adb' (or 'adb.exe') is in the $PATH
# Note: using / as directory separator is fine in Windows, Perl always knows what you mean :)
my $adb = (grep { $_ && -f $_ } (
             ($ENV{'PROGRAMFILES(X86)'} ? $ENV{'PROGRAMFILES(X86)'} . '/Android/android-sdk/platform-tools/adb.exe' : undef),
             ($ENV{'PROGRAMFILES'} ? $ENV{'PROGRAMFILES'} . '/Android/android-sdk/platform-tools/adb.exe' : undef),
             'c:/flip/android-sdk/platform-tools/adb.exe',
             '/opt/android/platform-tools/adb',
           ))[0] || 'adb';

if ($^O =~ m/win32/i)
{
	# Windows shell likes "" around path names with spaces
	$adb = '"' . $adb . '"';
}

# directory where this script lives
my $dir = $FindBin::Bin;


my @etcFiles = ('gps.conf', 'u-blox.conf', 'ca-cert-google.pem');
my $lib = '../../../out/target/product/panda/system/lib/hw/gps.default.so';

print("adb=$adb\ndir=$dir\n") if ($debug);

# result (exit code) and output of external commands (adb)
my $err = 0;
my @out = ();


if (!$err)
{
    print("* checking if adb and the target are up\n");
    ($err, @out) = shellCmd("$adb shell date");
}

my $file;
foreach $file (@etcFiles)
{
	if (!$err)
	{
		print("* install /etc/$file file\n");
		($err, @out) = shellCmd("$adb push $dir/$file /etc/");
	}
}

if (!$err)
{
    print("* install /system/lib/hw/gps.default.so library\n");
    ($err, @out) = shellCmd("$adb push $dir/$lib /system/lib/hw/");
}

my $zygotePid = 0;
if (!$err)
{
    print("* find PID of the zygote task\n");
    ($err, @out) = shellCmd("$adb shell ps zygote");

    if ($out[1] && ($out[1] =~ m/^\s*root\s+(\d+).*zygote\s*$/o))
    {
        $zygotePid = $1;
    }
}
$err = 1 unless ($zygotePid);

if (!$err)
{
    print("* kill zygote\n");
    ($err, @out) = shellCmd("$adb shell kill $zygotePid");
}


# atexit() routine
END
{
    if ($err)
    {
        print("ERROR: Something went wrong above!\n");
        if ($^O =~ m/Win32/i)
        {
            print("Press any key to end.\n");
            getc();
        }
        exit($err);
    }
}

# stolen from ReleaseTest::Util
sub shellCmd
{
    my $cmd = shift;
    $cmd .= " 2>&1" unless ($cmd =~ m@\s+2>.+@g);
    my $retval;
    print("--> $cmd\n") if ($debug);
    my @cmdOut = ();
    unless (open(CMD, '-|', $cmd))
    {
        print("ERROR: Could not run command '$cmd': $!\n");
        return -1;
    }
    else
    {
        while (<CMD>)
        {
            chomp();
            push(@cmdOut, $_);
            print("<-- $_\n") if ($debug);
        }
        close(CMD);
    }
    my $ret = $? || 0;
    my $coreDumped = ($ret & 0x80);
    my $signal     = ($ret & 0x7f);
    my $exitCode   = ($ret >> 8) & 0xff;

    if ($exitCode || $coreDumped || $signal)
    {
        printf("ERROR: Command '$cmd' failed (ret=0x%04x coreDumped=%i signal=0x%02x exitCode=%i)!\n",
                $ret, $coreDumped, $signal, $exitCode);
    }

    return $exitCode, @cmdOut;
}

1;
__END__
