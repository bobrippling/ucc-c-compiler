#!/usr/bin/perl
use warnings;

my $verbose = 0;

sub dirname
{
	my $a = shift;
	return $1 if $a =~ m@^(.*/)[^/]+$@;
	return './';
}

sub system_v
{
	print "$0: run: @_\n" if $verbose;
	return system @_;
}

if($ARGV[0] eq '-v'){
	$verbose = 1;
	shift;
}

my $exp = shift;

if($exp !~ /^[0-9]+$/){
	die "$exp not numeric";
}

unless(-x $ARGV[0]){
	# we've been passed a source file
	my($cmd, @args) = @ARGV;
	@ARGV = ();

	my $ucc = $ENV{UCC};
	die "$0: no \$UCC" unless $ucc;

	my $tmp = "/tmp/$$.out";
	if(system_v($ucc, '-o', $tmp, $cmd, @args)){
		die;
	}

	$ARGV[0] = $tmp;
}

my $r = system_v(@ARGV);

$r >>= 8;

if($exp != $r){
	die "$0: expected $exp, got $r, from @ARGV\n";
}
