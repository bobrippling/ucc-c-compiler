#!/usr/bin/perl
use warnings;

sub file_contents
{
	open S, '<', shift or die;
	my @l = <STDIN>;
	close S;
	return @l;
}

my $src = shift;

die "Usage: $0 src\n" unless @ARGV == 0 and defined($src);

(my $chk = $src) =~ s/c$/chk.s/;

my @asm_src = file_contents($src);
my @asm_chk = file_contents($chk);

# @asm_chk is a list of strings:
# /type: *match/
#   type is either "absent" or "present"
#   string is: string =~ m_^/.*/$_ ? regex : &index

for my $line (@asm_chk){
	chomp($line);
	die "bad asmcheck line $line\n" unless /^([a-z]+): *(.*)/;

	my $present;
	if($1 eq 'present'){
		$present = 1;
	}elsif($1 eq 'absent'){
		$present = 0;
	}else{
		die "bad asmcheck directive $1\n";
	}

	my($matcher, $is_regex) = ($2, 0);
	if($matcher =~ /^\//){
		$is_regex = 1;
	}

	sub match
	{
		my($matcher, $is_regex, @lines) = @_;

		for my $l (@lines){
			if($is_regex ? $l =~ /$matcher/ : (index($l, $matcher) != -1)){
				return 1;
			}
		}
		return 0;
	}

	# run the match
	if(!match($matcher, $is_regex, @asm_src)){
		die "couldn't match { .s = /$matcher/, .is_regex = $is_regex }\n";
	}
}
