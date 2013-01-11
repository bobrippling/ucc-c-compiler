#!/usr/bin/perl
use warnings;

my $src = shift;

die "Usage: $0 src\n" unless @ARGV == 0 and defined($src);

(my $asm = $src) =~ s/c$/s/;

my @actual = <STDIN>;

open S, '<', $asm or die;
my @asm = <STDIN>;
close S;

# TODO: make sure given lines in @asm match a line in @actual
