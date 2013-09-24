#!/usr/bin/perl
use warnings;

my %hexen;
my $last_col = 1;
my $COL_LIM = 7;
my $c256;

if(@ARGV && $ARGV[0] eq '-256'){
	shift;
	$COL_LIM = 256;
	$c256 = 1;
}

sub col_idx
{
	my $c = $last_col++;
	$last_col = 1 if $last_col > $COL_LIM;
	return $c;
}

sub got_hex
{
	my $txt = shift;
	my $found = $hexen{$txt};
	return $found if $found;

	my $icol = col_idx();
	return $hexen{$txt} =
		"\033[" .
		($c256 ? "38;5;" : "3") .
		"${icol}m$txt\033[m";
}

while(<>){
	s/0x[0-9a-fA-F]+/got_hex($&)/eg;
	print;
}
