#!/usr/bin/perl
use warnings;
use strict;

my $ran = 0;
my $want_check = 0;
my $had_check = 0;
my $file = undef;
my $log = undef;
my $verbose = 0;

my %vars = (
	'check'     => './bin/check.sh',
	'asmcheck'  => './asmcheck.pl',
	'stdoutcheck' => './bin/stdoutcheck',
	'ocheck' => './bin/ocheck',
	'layout_check' => './bin/layout_check.sh',
	'caret_check' => './caret_check.pl',
	'debug_check' => './bin/debug-check',
	'debug_scope' => './debug_scope.sh',
	'jmpcheck' => './jmpcheck.sh',
	'archgen' => './archgen.pl',
	'gdbcheck' => './gdb.sh',
);

sub require_env
{
	my $name = shift;
	die "no \$$name" unless exists $ENV{$name};
	return $ENV{$name};
}

sub apply_vars
{
	my $f = shift;

	for my $regex ("^()", "([^%])"){
		$f =~ s/$regex%([a-z_]+)/
		$vars{$2} ? $1 . $vars{$2} : die "undefined variable %$2 in $file"/ge;

		$had_check++ if defined $2 and $2 eq 'check';
	}

	$f =~ s/%%/%/g;

	return $f;
}

sub system_loud_on_failure
{
	my $pid = fork();
	if($pid < 0){
		die "fork(): $!";
	}

	if($pid == 0){
		# $args < /dev/null > "$2" 2>&1
		open STDIN, '</dev/null' or die;
		open STDOUT, '>', $log or die;
		open STDERR, '>&STDOUT' or die;

		exec @_;
		die "exec: $!";
	}

	if(wait() != $pid){
		die "bad wait return? $!";
	}

	my $r = $?;

	if($r >> 8){
		system('sed', 's/^/	/', $log);
	}

	return $r;
}

my $ucc = require_env('UCC');
my $tmpdir = require_env('UCC_TESTDIR');
$verbose = exists $ENV{UCC_VERBOSE};

if(@ARGV != 1){
	die "Usage: $0 file\n";
}
$file = shift;

my $target = "$tmpdir/$$.tmp";
$log = "$tmpdir/$$.log";
END {
	my $r = $?;
	unlink $target if defined $target;
	unlink $log if defined $log;
	$? = $r;
}

$vars{s} = $file;
$vars{t} = $target;
$vars{ucc} = $ucc;

open F, '<', $file or die "open $file: $!\n";
while(<F>){
	chomp;

	if(my($command, $sh) = m{\b([A-Z]+): *(.*)}){
		if($command eq 'RUN'){
			$ran++;

			my $subst_sh = apply_vars($sh);
			print "$0: run: $subst_sh\n" if $verbose;

			my $want_err = ($subst_sh =~ s/^ *! *//);

			my $ec = system_loud_on_failure($subst_sh);

			if($ec & 127){
				# signal death - always a failure
				die "$0: command '$subst_sh' caught signal " . ($ec - 128) . "\n";
			}

			my $ec_normalised = !!($ec >> 8);
			my $unexpected = ($want_err ? "passed" : "failed");

			die "$0: $unexpected: $subst_sh\n" if $want_err != $ec_normalised;

		}elsif($command eq 'CHECK'){
			$want_check = 1;
		}
	}
}
close F;

die "no commands in $file"
unless $ran;

die "CHECK commands found with no %check"
unless $had_check or not $want_check;
