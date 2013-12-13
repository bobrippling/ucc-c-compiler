#!/usr/bin/perl -p
use warnings;

my %hexen;
my $last_col = 1; # 1 to 7

sub pick_col
{
	my $c = $last_col++;
	$last_col = 1 if $last_col > 7;
	return $c;
}

sub got_hex
{
	my $h = shift;
	my $found = $hexen{$h};
	return $found if $found;

	my $col = pick_col();
	return $hexen{$h} = "\033[3${col}m$h\033[m";
}

s/0x[0-9a-fA-F]+/got_hex($&)/eg;
