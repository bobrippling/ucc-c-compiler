#!/usr/bin/perl
use warnings;

sub dirname
{
	my $s = shift;
	return $s unless $s =~ m#(.*)/[^/]+$#;
	return $1;
}

sub basename
{
	my $s = shift;
	return $s unless $s =~ m#/([^/]+)$#;
	return $1;
}

sub run
{
	my $r = system @_;
	exit $r if $r;
}

$DEBUG = '';

# heuristic-ish
for(@ARGV){
	if(/^-/){
		if(/^--debug$/){
			$DEBUG = "echo ";
		}else{
			push @args, $_;
		}
	}else{
		push @files, $_;
	}
}

$path = dirname($0);

die "need input\n" unless @files;
if(@files == 1){
	exec "${DEBUG}$path/cc", @ARGV;
	die "exec cc: $!\n";
}

for(@files){
	my $out = "/tmp/" . basename($_) . ".tmp";
	push @link_these, $out;

	run "${DEBUG}$path/cc -c -o $out @args $_";
}

@libs = (
	"crt",
	"stdio",
	"stdlib",
	"string",
	"unistd",
	"syscall",
	"signal",
	"assert",
	"ctype",
	"ucc",
	"sys/fcntl",
	"sys/wait",
	"sys/mman",
);

@libs = map { s#^#$path/../lib/#; s#$#.o#; $_ } @libs;

run "${DEBUG}ld -o a.out @libs @link_these";

unlink @link_these;

print "$0: compiled to a.out\n";
