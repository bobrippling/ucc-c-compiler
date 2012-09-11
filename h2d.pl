#!/usr/bin/perl
use warnings;

while(<>){
	if(/0x[0-9a-f]+/){
		print $` . hex($&) . $';
	}else{
		print;
	}
}
