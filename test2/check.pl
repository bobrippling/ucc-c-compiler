#!/usr/bin/perl
use warnings;
use strict;

sub die2
{
	die "$0: @_\n";
}

sub chomp_all
{
	return map { chomp; $_ } @_;
}

sub lines
{
	my $f = shift;
	open F, '<', $f or die2 "open '$f': $!";
	my @l = <F>;
	close F;
	return @l;
}

die "Usage: $0 file_with_checks.c\n" unless @ARGV == 1;

my @lines;
my $line;
# (
#   [0] = { warnings = [], checks = [] },
#   [1] = { warnings = [], checks = [] },
#   ...
# )

# ---------------------------
# read warnings in

for(chomp_all(<STDIN>)){
	if(/^([^:]+):([0-9]+):(([0-9]+):)? *(.*)/){
		my $line = $2;

		my $warn = { file => $1, line => $line, col => $4 || 0, msg => $5 };

		push @{$lines[$line - 1]->{warnings}}, $warn;
	}
}

# ---------------------------
# read checks in

$line = 1;
for(chomp_all(lines(shift))){
	if(m#// *CHECK: *(.*)#){
		push @{$lines[$line]->{checks}}, { check => $1, line => $line };
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
);

# ---------------------------
# compare

iter_lines(
	sub {
		my($line, $check_ref, $warn_ref) = @_;

		my @checks = @$check_ref;
		my @warns  = @$warn_ref;

		my $nchecks = @checks;
		my $nwarns  = @warns;

		if($nchecks != $nwarns){
			warn "line: $line, warnings ($nwarns) != checks ($nchecks)\n";
		}elsif($nchecks){
			# make sure they're equal using $check
			my @copy = @warns;

			for(@checks){
				my $check = $_;
				my $match = $check->{check}; # /regex/
				die2 "invalid CHECK: '$match'" unless $match =~ m#^/(.*)/$#;

				my $regex = $1;
				my $found = 0;

				for(@copy){
					if($_ =~ /$regex/){
						$found = 1;
						$_ = ''; # silence
						last;
					}
				}

				warn "check $match not found in warnings, line $check->{line}\n" unless $found;
			}
		}
	}
);
