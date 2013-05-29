#!/usr/bin/perl
use warnings;

sub usage
{
	die "Usage: $0 line1 line2... [< to_check]\n"
}

usage() unless @ARGV;

die "$0 needs STDIN from a pipe\n" if -t STDIN;

my @output = map { chomp; $_ } <STDIN>;

die "mismatching output counts" unless @output == @ARGV;

for(my $i = 0; $i < @output; ++$i){
	my($a, $b) = ($output[$i], $ARGV[$i]);
	die "mismatching lines [$i]: '$a' and '$b'\n" unless $a eq $b;
}
