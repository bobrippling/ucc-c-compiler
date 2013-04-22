#!/usr/bin/perl
use warnings;

sub dirname
{
	my $a = shift;
	return $1 if $a =~ m@^(.*/)[^/]+$@;
	return './';
}

my $exp = shift;

unless(-x ($cmd = $ARGV[0])){
	# we've been passed a source file
	my $ucc = dirname($0) . "../ucc";
	my $tmp = "/tmp/$$.out";
	if(system($ucc, '-o', $tmp, $cmd)){
		die;
	}
	$ARGV[0] = $tmp;
}

my $r = system(@ARGV);

$r >>= 8;

if($exp != $r){
	die "$0: expected $exp, got $r, from @ARGV\n";
}
