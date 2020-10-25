#!/usr/bin/perl
use warnings;

# sort according to label
my %labels;
my $curlbl = '';
while(<>){
	chomp;
	if(/(.*):$/){
		$curlbl = $1;
		if($labels{$curlbl}){
			die "label \"$curlbl\" already used";
		}
		@{$labels{$curlbl}} = ();
	}else{
		push @{$labels{$curlbl}}, $_;
	}
}

for(sort keys %labels){
	print "$_:\n";
	print "$_\n" for @{$labels{$_}};
}
