#!/usr/bin/perl
use warnings;

my $src = shift;

die "Usage: $0 src\n" unless @ARGV == 0 and defined($src);

(my $asm = $src) =~ s/c$/chk.s/;

my @actual = <STDIN>;

open S, '<', $asm or die;
my @asm = <STDIN>;
close S;

die "TODO";

# @asm is a list of strings:
# /type: *match/
#   type is either "absent" or "present"
#   string is: string =~ m_^/.*/$_ ? regex : &index

# TODO: make sure given lines in @asm match a line in @actual
