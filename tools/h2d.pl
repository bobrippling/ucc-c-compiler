#!/usr/bin/perl
use warnings;

while(<>){
	s/0x[0-9a-f]+/hex($&)/eg;
	print;
}
