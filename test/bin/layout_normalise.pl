#!/usr/bin/perl
use warnings;

sub emit;
sub emit_string;

sub is_zero
{
	return shift =~ /^(space|zero)$/;
}

my %sizes = (
	'byte'  => 1,
	'word'  => 2,
	'long'  => 4,
	'quad'  => 8,
);
my %sizes_r = map { $sizes{$_} => $_ } keys %sizes;

my @parts; # { size, value } or { lbl }
my $any = 0;

my $check_sections = 0;
if(@ARGV && $ARGV[0] eq '--sections'){
	$check_sections = 1;
	shift;
}

while(<>){
	s/#.*//;
	if(/^[ \t]*\.(byte|word|long|quad|zero|space)[ \t]+(.*)/){
		my $sz = $1;
		my $is_zero = is_zero($1);

		(my $rest = $2) =~ s/ +$//;

		for my $ent (split /, */, $rest){
			my $r;
			$ent =~ s/^_//;
			if($is_zero){
				$r = { size  => $ent,
					     value => 0 };
			}else{
				$r = { size  => $sizes{$sz},
					     value => $ent };
			}

			emit($r);
		}
		$any = 1;
	}elsif(/(.*): *$/){
		(my $lbl = $1) =~ s/^_//;

		my $is_private = ($lbl =~ /^[ \t]*\.?L/);

		if(!$is_private){
			emit({ lbl => $lbl });
			$any = 1;
		}
	}elsif(/^[ \t]*\.asci([iz])[ \t]+"(.*)"$/){
		my $asciz = $1 eq 'z';
		emit_string($asciz, $2);
		$any = 1;
	}elsif(/^\.comm\b/){
		chomp;
		emit({ str => $_ });
	}elsif($check_sections and /^\.section/){
		chomp;
		emit({ str => $_ });
	}
}

die "$0: no asm found in input\n" unless $any;

sub flush;
sub emit2;
my $last;
END {
	flush();
}

sub emit_string
{
	my($asciz, $str) = @_;

	for(my $i = 0; $i < length $str; $i++){
		my $ch = substr($str, $i, 1);
		my $val;

		if($ch eq "\\"){
			$ch = substr($str, ++$i);
			if($ch =~ /["\\]/){
				$val = ord $ch;
			}else{
				# either [0-7]{3} or quote or backslash
				my $rest = substr($str, $i);

				if($rest =~ /^([0-9]{3})/){
					$val = oct($1);
					$i += 2;
				}else{
					die "$0: couldn't match string byte from '$str'\n";
				}
			}
		}else{
			$val = ord($ch);
		}

		emit({ size => 1, value => $val });
	}

	emit({ size => 1, value => 0 }) if $asciz;
}

sub flush
{
	if($last){
		emit2 $last;
		$last = undef;
	}
}

sub emit2
{
	my $m = shift;
	if($m->{lbl}){
		print "$m->{lbl}:$/";
	}elsif($m->{str}){
		print "$m->{str}$/";
	}elsif($m->{value}){
		print ".$sizes_r{$m->{size}} $m->{value}\n";
	}else{
		print ".zero $m->{size}\n";
	}
}

sub emit
{
	my $m = shift;
	if($m->{lbl} or $m->{str}){
		;
	}elsif($m->{value} =~ /^[0-9]+$/ and $m->{value} == 0){
		if($last){
			$last->{size} += $m->{size};
		}else{
			$last = $m;
		}
		return;
	}
	flush();
	emit2($m);
}
