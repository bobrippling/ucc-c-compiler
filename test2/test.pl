#!/usr/bin/perl
use warnings;

sub apply_vars;
sub die2;
sub usage
{
	die "Usage: $0 [--ucc=path] file\n";
}

my $ucc = '../ucc';
my $f = undef;
my $verbose = 0;

for(@ARGV){
	if(/^--ucc=(.+)/){
		$ucc = $1;
	}elsif($_ eq '-v'){
		$verbose = 1;
	}elsif(!defined $f){
		$f = $_;
	}else{
		usage();
	}
}

(my $target = $f) =~ s/\.c$//;
$target = "./$target";

my %vars = (
	's'         => $f,
	't'         => $target,
	'ucc'       => $ucc,
	'check'     => './check.pl'
	'asmcheck'  => './asmcheck.pl'
);

open F, '<', $f or die2 "$f: $!";
while(<F>){
	chomp;

	if(my($command, $sh) = m{// *([a-z]+): *(.*)}i){
		my $subst_sh = apply_vars($sh);

		if($command eq 'RUN'){
			print "$0: run: $subst_sh\n" if $verbose;

			my $ec = system($subst_sh);

			die2 "command '$subst_sh' failed" if $ec;
		}else{
			#die2 "unrecognised command: $command";
		}
	}
}
close F;

unlink $target;

exit;

# --------

sub die2
{
	die "$0: @_\n";
}

sub apply_vars
{
	(my $f = shift) =~ s/%([a-z]+)/
	$vars{$1} ? $vars{$1} : die2 "undefined variable %$1"/ge;

	return $f;
}
