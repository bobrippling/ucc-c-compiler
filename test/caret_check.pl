#!/usr/bin/perl
use warnings;
use strict;
use constant
{
	EMPTY => 0,
	GOT   => 1
};

require Parser;

my $src = shift;
die "Usage: $0 source.c [ucc-args...]\n" unless $src;

my @warns = find_warnings($src, @ARGV); # lineno, offset, msg
my @carets = find_carets($src); # lineno, offset, msg

my %warnmap;
for(@warns){
	my $a_ref = $warnmap{$_->{lineno}};

	if(!$a_ref){
		$warnmap{$_->{lineno}} = $a_ref = [];
	}

	push @{$a_ref}, $_;
}

my $ec = 0;

for my $caret (@carets){
	my $a_ref = $warnmap{$caret->{lineno}};
	my $found = 0;

	if($a_ref){
		my @line_warns = @{$a_ref};

		for my $warn (@line_warns){
			if($warn->{msg} =~ /$caret->{msg}/){
				if($warn->{offset} != $caret->{offset}){
					warn "$src:$caret->{lineno}: mismatching offset "
						. "(ucc warning=$warn->{offset}, caret=$caret->{offset})\n\t"
						. "for '$caret->{msg}'\n";

					$ec = 1;
					$found = 1;
				}else{
					$found = 1;
					last;
				}
			}
		}
	}

	if(!$found){
		warn "\"$caret->{msg}\" @ $caret->{lineno} offset $caret->{offset} not found\n";
		$ec = 1;
	}
}

exit $ec;

# ------------------

sub find_carets
{
	my $src = shift;
	open F, '<', $src or die "$0: open $src: $!\n";

	my $lineno;
	my @carets;
	my $in = 0;

	while(my $line = <F>){
		if($line =~ m;// *CARETS:$;){
			$in = 1;
			$lineno = $. - 1;
		}elsif($in){
			if($line =~ m;^//( *)\^ *(.*);){
				my($spc, $msg) = ($1, $2);

				# 2 for "//"
				my $offset = 2 + ($spc =~ s/[ \t]//g);

				push @carets, {
					lineno => $lineno,
					offset => $offset,
					msg    => $msg
				};
			}else{
				$in = 0;
			}
		}
	}

	close F or warn "close $src: $!\n";

	return @carets;
}

sub find_warnings
{
	my($src, @args) = @_;
	die "$0: no \$UCC\n" unless $ENV{UCC};

	return map {
		offset => $_->{caret_spc},
		msg    => $_->{msg},
		lineno => $_->{line},
	}, Parser::parse_warnings(Parser::chomp_all(
			`$ENV{UCC} -fsyntax-only '$src' @args 2>&1`));
}
