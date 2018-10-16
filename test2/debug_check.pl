#!/usr/bin/perl
use warnings;
use strict;

my $ec;

sub filter
{
	my @lines = @_;
	my @relevant;
	my @files;
	my $in_dbg = 0;

	for(@lines){
		if(/\.file.*"/){
			push @files, $_;
			next;
		}

		if(/section_(begin|end)_dbg/){
			$in_dbg = ($1 eq 'begin');
		}

		if($in_dbg){
			push @relevant, $_;
		}
	}

	return (
		files => \@files,
		lines => \@relevant,
	);
}

sub read_file
{
	my $f = shift();
	open(my $fh, '<', $f) or die "open $f: $!\n";
	my @contents = map { chomp; $_ } <$fh>;
	close($fh) or die "close $f: $!\n";
	return @contents;
}

sub write_file
{
	my($f, @contents) = @_;
	open(my $fh, '>', $f) or die "open $f: $!\n";
	for(@contents){
		print $fh "$_\n";
	}
	close($fh) or die "close $f: $!\n";
}

sub arrays_eq
{
	my($a, $b) = @_;

	if(scalar(@$a) != scalar(@$b)){
		return 0;
	}

	for(my $i = 0; $i < scalar(@$a); $i++){
		if(!(${$a}[$i] eq ${$b}[$i])){
			return 0;
		}
	}

	return 1;
}

sub assert_arrays_eq
{
	my($got, $expected, $type) = @_;

	if(!arrays_eq($got->{$type}, $expected->{$type})){
		print STDERR "mismatching $type entries:\n";

		my $f_got = "./debug_check_diff_got";
		my $f_exp = "./debug_check_diff_expected";

		write_file($f_got, @{$got->{$type}});
		write_file($f_exp, @{$expected->{$type}});

		system("diff", "-u", $f_exp, $f_got);
		unlink($f_got, $f_exp);

		$ec = 1;
	}
}

if(@ARGV != 1){
	die "Usage: $0 path/to/source\n";
}
my $in = shift;
my $expected = "$in.dwarf";
my $ucc = $ENV{UCC} or die "no \$UCC";

# format $in - remove any components up until dir/file.c
$in =~ s;^\./;;;

# FIXME: -target x86_64-linux-gnu
my @output = map { chomp; $_ } `'$ucc' -fno-leading-underscore -fdebug-compilation-dir=/tmp/ -g -S -o- '$in'`;
if($?){
	die "$0: compile failed ($?)";
}

my %got = filter(@output);
my %expected = filter(read_file($expected));
$ec = 0;

assert_arrays_eq(\%got, \%expected, "files");
assert_arrays_eq(\%got, \%expected, "lines");

exit($ec);
