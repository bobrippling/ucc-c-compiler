#!/usr/bin/perl
use strict;
use warnings;

die "Usage: $0 executable-file\n"
unless @ARGV == 1;

die "$0: need \$objdump\n" unless exists $ENV{objdump};

my $file = shift;

my @dwarf_info = `"$ENV{objdump}" --dwarf=info "$file"`;
my @dwarf_linemap = `"$ENV{objdump}" --dwarf=decodedline "$file"`;

my @linemap = sort {
	$a->{addr} <=> $b->{addr}
} map {
	/ ([0-9]+) +([0-9a-fx]+) *$/;
	{ line => $1, addr => oct($2) }
} grep / [0-9]+ +[0-9a-fx]+ *$/, @dwarf_linemap;

my %curlexblk;
my %scopes;

for(my $i = 0; $i < @dwarf_info; $i++){
	$_ = $dwarf_info[$i];

	if(/DW_AT_(low|high)_pc *: *([0-9a-fx]+)/){
		my $highlow = $1;
		my $addr = $2;

		$curlexblk{$highlow} = oct($addr);
	}
	elsif(/^ *<[0-9a-f]*><([0-9a-f]+)>.*DW_TAG_(variable|formal_parameter)/){
		my $var_id = $1;
		$i++;
		for(; $i < @dwarf_info; $i++){
			$_ = $dwarf_info[$i];

			if(/DW_AT_name *: *([^ ].*)/){
				# got a variable name
				my $base = $1;
				my $name = $base;
				my $i = 1;

				while(exists $scopes{$name}){
					$name = "$base<$i>";
					$i++;
				}

				$scopes{$name} = { %curlexblk };

			}elsif(/DW_TAG/){
				# not found
				$i--;
				last;
			}
		}
	}
}

sub addr2lineno
{
	my $addr = shift();

	for(my $i = 0; $i < $#linemap; $i++){
		my $low = $linemap[$i]->{addr};
		my $high = $linemap[$i + 1]->{addr};

		if($low <= $addr and $addr < $high){
			return $linemap[$i]->{line};
		}
	}

	return $linemap[$#linemap]->{line};
}

for my $name (keys %scopes){
	my $low = addr2lineno($scopes{$name}->{low});
	my $high = addr2lineno($scopes{$name}->{high});

	print "$name $low $high\n";
}
