#!/usr/bin/perl
use warnings;

sub mapchomper
{
	return map { chomp; $_ } @_;
}

sub file_contents
{
	my $f = shift;
	open S, '<', $f or die "open $f: $!\n";
	my @l = <S>;
	close S;
	return mapchomper @l;
}

my $src = shift;

die "Usage: $0 src\n" unless @ARGV == 0 and defined($src);

(my $chk = $src) =~ s/c$/chk.s/;

die "no \$UCC" unless exists $ENV{UCC};
my @src = mapchomper `$ENV{UCC} -S -o- $src 2>/dev/null`;
my @chk = file_contents($chk);

# @chk is a list of strings:
# /type: *match/
#   type is either "absent" or "present"
#   string is: string =~ m_^/.*/$_ ? regex : &index

for my $line (@chk){
	chomp($line);

	next if $line =~ /^(#.*|)$/;

	die "bad asmcheck line '$line'\n" unless $line =~ /^([a-z]+): *(.*)/;

	my $present;
	if($1 eq 'present'){
		$present = 1;
	}elsif($1 eq 'absent'){
		$present = 0;
	}else{
		die "bad asmcheck directive $1\n";
	}

	my($matcher, $is_regex) = ($2, 0);
	if($matcher =~ m#^/(.*)/$#){
		$is_regex = 1;
		$matcher = $1;
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
	if($present != match($matcher, $is_regex, @src)){
		my $desc = $present ? "couldn't find" : "found";
		die "$desc { .s = /$matcher/, .is_regex = $is_regex }\n";
	}
}
