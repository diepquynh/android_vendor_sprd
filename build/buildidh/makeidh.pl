#!/usr/bin/perl -w
use strict;
use Cwd;
use Data::Dumper;
use Encode;
use File::Path;
use File::Copy;

use utf8;
binmode(STDIN,':encoding(utf8)');
binmode(STDOUT,':encoding(utf8)');
binmode(STDERR,':encoding(utf8)');

# check arguments
if(@ARGV lt 2) {
    die "please check your arguments:work_dir,delete_folders[,exclude_repos]\n";
}

my $filepath = $ARGV[0]; #/home/builder/repo/sprdroid4.4_3.10
my $deleted_repos = $ARGV[1];#device/sprd,vendor/sprd/proprietories-source
my $exclude_repos = $ARGV[2];#vendor/sprd/proprietories-source/rilproxy

if ($deleted_repos) {
  $deleted_repos =~ s/\s+//g;
  print "deleted_repos:$deleted_repos\n";
}

if ($exclude_repos) {
  $exclude_repos =~ s/\s+//g;
  print "exclude_repos:$exclude_repos\n";
}

my @delrepos=split(/,/,$deleted_repos);
my @excrepos = split(/,/,$exclude_repos);

my $basefile="$filepath/default.xml";
my $imgpath="$filepath/out/IDH";
my $WORKSPACE = "$imgpath/idh.code";

rmtree("$WORKSPACE");
mkpath("$WORKSPACE");

chdir($filepath);
`repo manifest -o $basefile -r`;

my $baserepo = &get_repos_and_revs($basefile);
foreach my $project (keys %$baserepo) {
  my $rev = $baserepo->{$project}->{'rev'};
  my $path = $baserepo->{$project}->{'path'};
  chdir("$filepath/$path");
  my $value = `git ls-tree $rev`;
  if ("$value") {
    `git archive --format=tar --prefix=$path/ $rev | (cd $WORKSPACE && tar xf -)`;
  }
}

######################### repack device/sprd repos ########################
chdir("$WORKSPACE");

foreach my $conf (`ls device/sprd`) {
  chomp($conf);
  `tar -czf conf-$conf.tar.gz device/sprd/$conf`;
  move("conf-$conf.tar.gz",$imgpath);
}

my $excludepath="$imgpath/exclude";
mkpath("$excludepath");
foreach my $exclude_repo(@excrepos) {
  my $exclude_repo_path=$exclude_repo;
  $exclude_repo_path=~s/\//_/g;
  `tar -czf $exclude_repo_path.tar.gz $exclude_repo`;
  move("$exclude_repo_path.tar.gz",$excludepath);
}



######################### delete unreleased repos #########################
chdir("$WORKSPACE");
foreach my $delete_repo(@delrepos) {
  print "rmtree:$delete_repo\n";
  rmtree($delete_repo);
}

#remove cmcc cucc and ctcc prebuild repos
rmtree("vendor/sprd/operator/cmcc/prebuild");
rmtree("vendor/sprd/operator/cucc/prebuild");
rmtree("vendor/sprd/operator/ctcc/prebuild");

if($exclude_repos) {
  foreach my $exclude_repo(`ls $excludepath`) {
   print "uncompress:$exclude_repo\n";
    `tar -xf $excludepath/$exclude_repo`;
  }
  rmtree($excludepath);
}


######################## copy makefile to root dir #######################
chdir("$WORKSPACE");
copy("build/core/root.mk","Makefile") || warn "could not copy files :$!";

chdir("$WORKSPACE");
######################## compress idh.code  ##############################
chdir("$imgpath");
`tar -czf idh.code.tgz idh.code`;
rmtree("$WORKSPACE");

sub get_repos_and_revs {
  my $filename = shift;
  my %filerepo=();
  open(PD,"<$filename") || die $?;
  while(defined(my $line=<PD>))
  {
    chomp($line);
    my $temp = $line;
    if ($line =~ /<project.*name="(\S+)".*revision="(\S+)".*>$/g)
    {
      my %project=();
      my $repo = $1;
      my $revision=$2;
      my $path=$repo;
      if($temp =~ /<project.*path="(\S+)".*>$/g) {
        $path = $1;
      }
      $project{'path'} = $path;
      $project{'rev'} = $revision;
      $filerepo{$repo}=\%project;
    }
  }
  close(PD);
  return \%filerepo;
}

