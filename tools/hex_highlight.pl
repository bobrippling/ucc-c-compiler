#!/usr/bin/perl -p
use warnings;

my %hexen;
my $last_col = 1; # 1 to 7

sub next_col
{
	my $c = $last_col++;
	return $c;
}

sub got_hex
{
	my $h = shift;
	my $found = $hexen{$h};
	return $found if $found;

	my $col = next_col();
	my $mod = int(($col - 1) / 7);
	$col = (($col - 1) % 7) + 1;

	return $hexen{$h} = "\033[3${col}m$h\{$mod}\033[m";
}

s/0x[0-9a-fA-F]+/got_hex($&)/eg;
