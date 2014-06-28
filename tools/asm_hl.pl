#!/usr/bin/perl
use warnings;

my %c = (
	green => "\e[1;32m",
	blue => "\e[1;34m",
	red => "\e[1;31m",
	yellow => "\e[1;33m",
	black => "\e[1;30m",
	pink => "\e[1;35m",
);

$c{insn} = $c{yellow};
$c{comment} = $c{black};


my @rules = (
	{ regex => '#.*', col => $c{comment}, stop => 5 },

	# early so it doesn't affect literal escapes
	{ regex => '\$?-?[0-9]+', col => $c{blue} },

	{ regex => '\bmov[a-z]*', col => $c{insn} },
	{ regex => '\bpush[a-z]*', col => $c{insn} },
	{ regex => '\btest[a-z]*', col => $c{insn} },
	{ regex => '\bj[a-z]+', col => $c{insn} },

	{ regex => '%[a-z]{2,3}', col => $c{red} },
);

while(<>){
	for my $rule (@rules){
		my $matched = s/$rule->{regex}/$rule->{col}$&\e[m/g;

		last if $matched and $rule->{stop};
	}
	print;
}
