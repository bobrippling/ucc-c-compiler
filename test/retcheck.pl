#!/usr/bin/perl
use warnings;
use Config '%Config';

my $verbose = 0;
my %sigmap;
my @signames = split " ", $Config{sig_name};
for my $n (split " ", $Config{sig_num}){
	$sigmap{$n} = $signames[$n];
}

sub signame
{
	my $n = shift();
	my $name = $sigmap{$n};
	return $name if $name;
	return "<unknown signal>";
}

sub dirname
{
	my $a = shift;
	return $1 if $a =~ m@^(.*/)[^/]+$@;
	return './';
}

sub system_v
{
	print "$0: run: @_\n" if $verbose;
	return system @_;
}

if($ARGV[0] eq '-v'){
	$verbose = 1;
	shift;
}

my $expected = shift();
my $expected_signal = 0;
my @unlinks;

if($expected =~ /^SIG[A-Z]+$/){
	$expected_signal = 1;
	$expected =~ s/^SIG//;
}elsif($expected =~ /^[0-9]+$/){
	# fine
}else{
	die "\"$expected\" not numeric, nor a signal";
}

unless(-x $ARGV[0]){
	# we've been passed a source file
	my($cmd, @args) = @ARGV;
	@ARGV = ();

	my $ucc = $ENV{UCC};
	die "$0: no \$UCC" unless $ucc;

	my $tmp = "$ENV{UCC_TESTDIR}/$$.out";
	push @unlinks, $tmp;
	if(system_v($ucc, '-o', $tmp, $cmd, @args)){
		die;
	}

	$ARGV[0] = $tmp;
}

my $r = system_v(@ARGV);
my $got_exitcode = $r >> 8;
my $got_signal = $r & 127;
if($got_signal){
	my $SAVE = $got_signal;
	$got_signal = signame($got_signal);
}

if($expected_signal){
	if(!$got_signal){
		die "$0: expected signal $expected, got exit code $got_exitcode, from @ARGV\n";
	}

	if(!($got_signal eq $expected)){
		die "$0: expected signal $expected, got signal $got_signal, from @ARGV\n";
	}
}else{
	if($got_signal){
		die "$0: expected exit code $expected, got signal $got_signal, from @ARGV\n";
	}

	if($expected != $got_exitcode){
		die "$0: expected exit code $expected, got exit code $got_exitcode, from @ARGV\n";
	}
}

END
{
	my $r = $?;
	unlink @unlinks;
	$? = $r;
}
