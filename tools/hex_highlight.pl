#!/usr/bin/perl
use warnings;

my $opt_shownth = 1;

if(@ARGV){
	if($ARGV[0] eq '-N'){
		$opt_shownth = 0;
		shift @ARGV;

	}elsif($ARGV[0] eq '--help'){
		print STDERR "Usage: $0 [-N] [files...]\n"
		. "  -N: Don't show the hex's entry number\n";
		exit 1;
	}
}

my %hexen;
my %hexcount;
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

sub count_hex
{
	return '' unless $opt_shownth;

	my $h = shift;

	my $cnt = ++$hexcount{$h};
	my $suff = 'th';

	if($cnt < 4 || $cnt > 13){
		my $end = $cnt % 10;

		if($end == 1){
			$suff = 'st';
		}elsif($end == 2){
			$suff = 'nd';
		}elsif($end == 3){
			$suff = 'rd';
		}
	}

	return "{$cnt$suff}";
}

while(<>){
	s/0x[0-9a-fA-F]+/got_hex($&) . count_hex($&)/eg;
	print;
}
