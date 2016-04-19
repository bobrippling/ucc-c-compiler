#!/usr/bin/perl
use warnings;

sub asmstrlen
{
	shift =~ /"([^"]+)"/;
	return length $1;
}

my %types = (
	"byte" => sub { return 1 },
	"word" => sub { return 2 },
	"short" => sub { return 2 },
	"long" => sub { return 4 },
	"quad" => sub { return 8 },
	"ascii" => sub { return asmstrlen shift },
	"asciz" => sub { return 1 + asmstrlen shift },
);

my $off = 0;
while(<>){
	my $orig = $_;

	s/#.*//;
	if(/\.([a-z]+)\s+(.*)/){
		my($ty, $rest) = ($1, $2);

		if($ty eq 'section'){
			$off = 0;
		}else{
			my $sub = $types{$ty};

			if(defined $sub){
				my $sz = $sub->($rest);
				print "/* offset = $off */ ";
				$off += $sz;
			}
		}
	}

	print $orig;
}
