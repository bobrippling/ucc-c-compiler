#!/usr/bin/perl
use warnings;

use constant
{
	ENOENT => 2
};

my %exes;
my $exes_init = 0;
sub can_exec
{
	if(!$exes_init){
		$exes_init = 1;

		for my $d (split /:/, $ENV{PATH}){
			opendir D, $d or next;
			map { $exes{$_} = 1 } readdir D;
			closedir D;
		}
	}

	my $e = shift;
	return exists $exes{$e};
}

if(@ARGV != 1){
	die "Usage: $0 [-v] path/to/source\n";
}
my $in = shift;
my $out = '/tmp/ucc.test/debug.out';

END
{
	my $r = $?;
	unlink $out;
	$? = $r;
}

my $ucc = $ENV{UCC} or die "no \$UCC";

if(system($ucc, '-g', '-c', '-o', $out, $in)){
	die "$0: compile failed";
}

my @dumpers = (
	{ exe => 'gdb', input => "q\n" },
	{ exe => 'lldb', input => "q\n" },
	{ exe => 'objdump', args => ['-W'] },
	{ exe => 'dwarfdump' },
);

my $bad = 0;
for my $dumper (@dumpers){
	my $cmd = $dumper->{exe};

	if($dumper->{args}){
		$cmd .= ' ' . join(' ', @{$dumper->{args}});
	}
	$cmd .= " $out >/dev/null";

	next unless can_exec($dumper->{exe});

	open SUB, '|-', $cmd or die "exec $dumper->{exe}: $!";
	{
		my $to_write = $dumper->{input};
		if(defined($to_write)){
			print SUB $to_write;
		}
	}
	close SUB;

	my $ret = $?;
	if($ret){
		warn "running '$dumper->{exe}' on $in returned $ret\n";
		$bad = 1;
	}
}

exit $bad;
