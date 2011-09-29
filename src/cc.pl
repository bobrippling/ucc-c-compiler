#!/usr/bin/perl

$exit_code = 0;

sub dirname($)
{
	my $p = shift;
	return $p unless $p =~ /(.*)\/[^\/]+$/;
	return $1;
}

sub run
{
	my $cmd = join '', @_;
	#print "+ $cmd\n";
	if(system $cmd){
		$exit_code = 1;
		exit;
	}
}

my($output, $input);
my($f_s, $f_o, $f);
my @tmpfs;

my $i = 0;
while($i <= $#ARGV){
	my $_ = $ARGV[$i];
	if($_ eq '-c'){
		$obj = 1;
	}elsif($_ eq '-o'){
		$output = $ARGV[++$i] || die "need output\n";
	}else{
		die "too many args\n" if $input;
		$input = $ARGV[$i];
	}
	$i++;
}

$where = dirname(readlink($0) || $0);

$f_s = "/tmp/ucc_$$.s";
$f_o = "/tmp/ucc_$$.o";
$f   = $output || "a.out";

push @tmpfs, $f_s;
run "$where/cc1 $input > $f_s";

if($obj){
	$f_o = $output || "a.o";
}else{
	push @tmpfs, $f_o;
}

run "nasm -f elf64 -o $f_o $f_s";
exit 0 if $obj;
run "ld -o $f $f_o $where/../lib/*.o";

END {
	run "rm @tmpfs";
	close STDOUT;
	exit $exit_code;
}
