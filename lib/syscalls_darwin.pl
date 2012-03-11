#!/usr/bin/perl

# read from syscalls_darwin_unix.h
while(<>){
	if(/[0-9]+$/){
		my $n = $&;
		$n += 0x2000000;
		printf '%s0x%x%s', $`, $n, $'
	}else{
		print
	}
}
