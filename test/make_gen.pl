#!/usr/bin/perl
use warnings;

sub rules_parse;
sub rules_assume;
sub usage()
{
	die "Usage: $0 [-v]\n";
}

if(@ARGV){
	if($ARGV[0] eq '-v'){
		$verbose = 1;
	}else{
		usage();
	}
}

%rules = rules_gendeps(rules_assume(rules_parse()));

open MAKE, '> deps' or die;

for(keys %rules){
	if($verbose){
		print STDERR "$rules{$_}->{target}: $_, should ";
		if($rules{$_}->{fail}){
			print STDERR "fail";
		}elsif($rules{$_}->{'exit-code'}){
			print STDERR "exit with $rules{$_}->{'exit-code'}";
		}else{
			print STDERR "exit cleanly";
		}
		print STDERR "\n";
	}

	print MAKE "$rules{$_}->{target}: $_\n";

	print MAKE "\t";

	my $fail_compile = $rules{$_}->{fail};

	if($fail_compile){
		print MAKE "! ";
	}

	print MAKE "../../ucc -o \$@ \$<\n";

	unless($fail_compile){
		my $ec = $rules{$_}->{'exit-code'};

		print MAKE "\t./\$@";
		if($ec){
			print MAKE "; [ \$? -eq $ec ]";
		}

		print MAKE "\n";
	}
}

close MAKE;

# -----------------------------------

sub lines
{
	my($f, $must_exist) = @_;
	open((my $fh), '<', $f) or ($must_exist ? die "open $f: $!\n" : return ());

	my @ret = <$fh>;
	close $fh;
	return map { chomp; $_ } @ret;
}

sub rule_new
{
	my $mode = shift;

	if($mode eq 'fail'){
		return { mode => $mode }
	}elsif($mode eq 'success'){
		return {};
	}elsif($mode =~ /^exit=([0-9]+)$/){
		return { 'exit-code' => $1 };
	}
}

sub rules_parse
{
	my %rules;

	for(lines('TestRules')){
		/(.*) *: *(.*)/ or die "bad rule: $_\n";
		my($ret, $fname) = ($1, $2);

		$rules{$fname} = rule_new($ret);
	}

	return %rules;
}

sub rules_assume
{
	my %rules = @_;
	for(glob '*.c'){
		unless($rules{$_}){
			# assume this file should succeed
			$rules{$_} = rule_new('success');
		}
	}
	return %rules;
}

sub rules_gendeps
{
	my %rules = @_;
	for(keys %rules){
		($rules{$_}->{target} = $_) =~ s/\.c$//;
	}
	return %rules;
}
