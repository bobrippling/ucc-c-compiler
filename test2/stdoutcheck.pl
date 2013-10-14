#!/usr/bin/perl
use warnings;

sub usage
{
	die "Usage: $0 [-w] line1 line2... [< to_check]\n"
}

sub trim
{
	my $s = shift;
	$s =~ s/^ +//;
	$s =~ s/ +$//;
	return $s;
}

my $ign_whitespace = 0;
if($ARGV[0] eq '-w'){
	shift;
	$ign_whitespace = 1;
}

usage() unless @ARGV;

die "$0 needs STDIN from a pipe\n" if -t STDIN;

my @output = map { chomp; $_ } <STDIN>;

if($ign_whitespace){
	@output = grep { !/^[ \t]*$/ } @output;
}

if(@output != @ARGV){
	if(@output){
		print "output:\n";
		map { print "  $_\n" } @output;
	}

	die "$0: mismatching output counts "
	. scalar(@output)
	. " vs "
	. scalar(@ARGV)
	. "\n";
}

for(my $i = 0; $i < @output; ++$i){
	my($a, $b) = ($output[$i], $ARGV[$i]);

	if($ign_whitespace){
		$a = trim($a);
		$b = trim($b);
	}

	die "mismatching lines [$i]: '$a' and '$b'\n" unless $a eq $b;
}
