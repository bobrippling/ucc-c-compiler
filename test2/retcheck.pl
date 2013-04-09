#!/usr/bin/perl
use warnings;

my $exp = shift;

my $r = system(@ARGV);

$r >>= 8;

if($exp != $r){
	die "$0: expected $exp, got $r, from @ARGV\n";
}
