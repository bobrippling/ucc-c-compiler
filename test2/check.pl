#!/usr/bin/perl
use warnings;
use strict;

require './parser.pl';

sub die2
{
	die "$0: @_\n";
}

sub lines
{
	my $f = shift;
	open F, '<', $f or die2 "open '$f': $!";
	my @l = <F>;
	close F;
	return @l;
}

my $verbose = 0;

if(@ARGV and $ARGV[0] eq '-v'){
	$verbose = 1;
	shift;
}

die "Usage: $0 [-v] file_with_checks.c\n" unless @ARGV == 1;

my @lines;
my $line;
# (
#   [0] = { warnings = [], checks = [] },
#   [1] = { warnings = [], checks = [] },
#   ...
# )
my $nchecks = 0;

# ---------------------------
# read warnings in

for my $w (parse_warnings((<STDIN>))){
	push @{$lines[$w->{line} - 1]->{warnings}}, $w;
}

# ---------------------------
# read checks in

$line = 1;
for(chomp_all(lines(shift))){
	if(m#// *CHECK: *(\^)? *(.*)#){
		my($above, $check) = (length($1), $2);
		my $line_resolved = $line;

		if(defined $above){
			--$line_resolved
		}else{
			$above = 0
		}

		push @{$lines[$line_resolved - 1]->{checks}}, {
			check => $check,
			line => $line_resolved,
			above => $above,
		};
		$nchecks++;
	}
	$line++;
}

# ---------------------------
# util

sub iter_lines
{
	my $callback = shift;

	my $n = 1;

	for(@lines){
		my $line = $_;

		sub array_from_ref
		{
			my $r = shift;
			return $r ? @{$r} : ();
		}

		my @checks = array_from_ref($line->{checks});
		my @warns  = array_from_ref($line->{warnings});

		if(@checks || @warns){
			$callback->($n, \@checks, \@warns);
		}

		$n++;
	}
}

# ---------------------------
# dump

iter_lines(
	sub {
		my($line, $check_ref, $warn_ref) = @_;

		my @checks = @$check_ref;
		my @warns  = @$warn_ref;

		print "line $line:\n";

		sub h2s
		{
			my %h = %{$_[0]};

			return '{ ' . join(', ', map { "$_ => '$h{$_}'" } keys %h) . ' }';
		}

		print "  warnings:\n" if @warns;
		print "    " . h2s($_) . "\n" for @warns;

		print "  checks:\n" if @checks;
		print "    " . h2s($_) . "\n" for @checks;
	}
) if $verbose;

# ---------------------------
# make sure we have at least one check
if($nchecks == 0){
	die "$0: no checks";
}

# ---------------------------
# make sure all checks are fulfilled. don't check all warnings have checks

my $missing_warning = 0;

iter_lines(
	sub {
		my($line, $check_ref, $warn_ref) = @_;

		my @checks = @$check_ref;
		my @warns  = @$warn_ref;

		# check all
		for my $check (@checks){
			my $match = $check->{check}; # /regex/ or literal word(s)
			my $rev = 0;

			my($search, $is_regex);
			if($match =~ m#^(!)?/(.*)/$#){
				$rev = defined $1;
				$search = $2;
				$is_regex = 1;
			}elsif($match =~ m#^ *(.*) *$#){
				$rev = 0;
				$search = $1;
				$is_regex = 0;
			}else{
				die2 "invalid CHECK (line $check->{line}): '$match'"
			}


			my $found = 0;

			for(@warns){
				if($is_regex ? $_->{msg} =~ /$search/ : index($_->{msg}, $search) != -1){
					$found = 1;
					$_->{msg} = ''; # silence
					last;
				}
			}

			if($found == $rev){
				$missing_warning = 1;
				warn "check $match "
				. ($rev ? "" : "not ")
				. "found in warnings on line $check->{line}"
				. ($check->{above} ? " ^" : "") . "\n"
			}
		}
	}
);

exit $missing_warning;
