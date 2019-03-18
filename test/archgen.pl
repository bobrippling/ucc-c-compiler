#!/usr/bin/perl
use warnings;

sub usage
{
	print STDERR "Usage: $0 [-v] source-file match [match2...] [flags]\n";
	print STDERR " match lists architectures followed by a match:\n";
	print STDERR " 'x86,x86_64:abc' 'x86_64:/xyz/' -fcommon\n";
}

sub check_codegen;

my $verbose = 0;
if($ARGV[0] eq '-v'){
	$verbose = 1;
	shift;
}

my $src = $ARGV[0] or die "no src";
shift;

my @matches;
while(@ARGV && $ARGV[0] !~ /^-/){
	push @matches, $ARGV[0];
	shift;
}

die "no matches given"
unless @matches;

my @flags = @ARGV;

my $ucc = $ENV{UCC} or die "no \$UCC";
chomp(my $arch = `uname -m`);

my @src_output = map { chomp; $_ } `$ucc -S -o- @flags '$src'`;

my $found_arch = 0;
for(@matches){
	/(.*):(.*)/;
	my($arches, $match) = ($1, $2);

	for(split /,/, $arches){
		if($_ eq $arch){
			check_codegen($match, @src_output);
			$found_arch = 1;
		}
	}
}

die "arch '$arch' not in $src"
unless $found_arch;

exit 0;

sub check_codegen
{
	my($match, @output) = @_;
	my $inverse = (substr($match, 0, 1) eq '!');
	$match = substr($match, 1) if $inverse;

	my $foundsub = sub {
		my($m, $l) = @_;
		if($inverse){
			die "found $m: $l";
		}

		return unless $verbose;
		print "found $m: $l\n";
	};

	for my $line (@output){
		if($match =~ m;^/;){
			my $regex = substr($match, 1, length($match) - 2);
			if($line =~ /$regex/){
				$foundsub->($match, $line);
				return;
			}
		}elsif(index($line, $match) != -1){
			$foundsub->($match, $line);
			return;
		}
	}
	if(!$inverse){
		die "no match for '$match'";
	}
}
