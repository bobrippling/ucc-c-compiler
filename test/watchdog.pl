#!/usr/bin/perl

use constant
{
	LIM_TERM => 5,
	LIM_KILL => 7,
	SLEEP_TM => 0.25,
};

sub sleep_ms
{
	select undef, undef, undef, shift
}

sub killpid
{
	my($sig, $pid) = @_;

	print "kill $pid\n" if $verbose;

	my $dead = kill($sig, $pid);

	if(not $dead){
		warn "kill(sig=$sig, pid=$pid): $!\n";
	}
}

if($ARGV[0] eq '-v'){
	$verbose = 1;
	shift;
}

if(@ARGV != 1){
	print STDERR "Usage: $0 [-v] name\n";
	exit 1;
}
my $nam = shift;

my %pids;
for(;;){
	my %alive = map { chomp; $_ => 1 } `pgrep '$nam'`;

	# prune the dead
	delete @pids{
		map {
			print "died $_\n" if $verbose;
			$_
		} grep {
			not exists $alive{$_}
		} keys %pids
	};

	# increment live count
	map {
		$pids{$_}++;
		print "alive $_ = $pids{$_}\n" if $verbose
	} keys %alive;

	# kill long living
	map {
		if($pids{$_} > LIM_KILL){
			killpid 9, $_;
		}elsif($pids{$_} > LIM_TERM){
			killpid 15, $_
		}
	} keys %pids;

	sleep_ms SLEEP_TM;
}
