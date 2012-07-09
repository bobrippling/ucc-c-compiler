#!/usr/bin/perl
use warnings;

sub extra_args
{
	my $f = shift;
	open F, '<', $f or die;
	my $head = <F>;
	close F;

	my @ret;
	if($head =~ m#/\* *ucc: *(.*) *\*/$#){
		push @ret, split / /, $1;
		print "extra args for $f:\n";
		print "  $_\n" for @ret;
	}
	return @ret;
}

sub compile
{
	my($fnam, @args) = @_;

	push @args, extra_args($fnam);

	unshift @args, $fnam;

	die "error compililng $fnam\n" if system "../../ucc", @args;
}

sub system_timeout
{
	my $cmd = shift;

	die "$0: fork: $!\n" unless defined(my $pid = fork());

	if($pid){
		my $sleep_time = 0;
		my $sleep_1 = 0.2;
		my $dead = -1;
		my $ret;

		while($dead == -1 && $sleep_time < 5){
			select undef, undef, undef, $sleep_1;
			$sleep_time += $sleep_1;

			$dead = waitpid($pid, WNOHANG);
			$ret = $? >> 8;
		}

		if($dead == -1){
			kill(9, $pid);
			die "timeout: $cmd\n";
		}
		die "waitpid: $!, $dead != $pid\n" unless $dead == $pid;

		return $ret;

	}else{
		exec $cmd;
		die;
	}
}

die if @ARGV;

for my $f (glob '*.c'){
	my $out = "$f.out";

	# TODO: make-esque checks
	compile($f, "-o", $out);

	my $ret = system_timeout("./$out > $f.ref");

	print "\e[1;34mret = $ret\e[m\n";

	die "error $ret running $out\n" if $ret >= 127;
}
