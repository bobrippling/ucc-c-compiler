#!/usr/bin/perl
use warnings;
use strict;

# instead, grep source file for:
# STDOUT: ...           # occurs somewhere on or after current line
# STDOUT-NEXT: ...      # occurs exactly on the next line
# STDOUT-SAME-LINE: ... # occurs exactly on the same line
# STDOUT-NOT: ...       # doesn't occur between here and the next match hitting

sub usage
{
	die "Usage: $0 file-with-annotations [< to-check]\n"
}

sub file_contents
{
	my $f = shift;
	open my $fh, '<', $f or die "$0: open $f: $!\n";
	my @c = map { chomp; $_ } <$fh>;
	close $fh;
	return @c;
}

sub next_line
{
	my $line = <STDIN>;
	die "$0: reached EOF\n" unless defined $line;
	chomp $line;
	$line =~ s/\bSTDOUT.*//;
	return $line;
}

sub linematch
{
	my($line, $text) = @_;

	if($text =~ m;^/(.*)/$;){
		my $regex = $1;

		return $line =~ /$regex/;
	}else{
		return index($line, $text) >= 0;
	}
}

usage()
if @ARGV != 1;

my $f = shift;
my @annotations;
for(file_contents($f)){
	next unless /\b(STDOUT(-[A-Z]+)*): *(.+)$/;

	push @annotations, { tag => $1, text => $3 };
}

my $line = '';
my @nots;
my $nots_active = 0;

sub record_not
{
	return unless $nots_active;
	my $line = shift;
	push @{$nots[$#nots]->{lines}}, $line;
}

for(my $annotation = 0; $annotation < @annotations; $annotation++){
	my $tag = $annotations[$annotation]->{tag};
	my $text = $annotations[$annotation]->{text};

	if($tag eq 'STDOUT'){
		while(1){
			$line = next_line();
			last if linematch($line, $text);

			# didn't match - record a STDOUT-NOT if needed
			record_not($line);
		}
		$nots_active = 0;
		next;
	}

	if($tag eq 'STDOUT-NEXT' or $tag eq 'STDOUT-SAME-LINE'){
		die "$0: STDOUT-NOT not followed by STDOUT\n" if $nots_active;

		$line = next_line() if($tag =~ /NEXT/);

		if(!linematch($line, $text)){
			die "$0: $tag: \"$text\" not in \"$line\"\n";
		}
		next;
	}

	if($tag eq 'STDOUT-NOT'){
		push @nots, { text => $text, lines => [] };
		$nots_active = 1;
		next;
	}

	die "$0: unrecognised tag '$tag'\n";
}

# verify nots
for my $notref (@nots){
	my $text = $notref->{text};
	my @lines = @{$notref->{lines}};

	for $line (@lines){
		if(linematch($line, $text)){
			die "$0: STDOUT-NOT: \"$text\" found in \"$line\"\n";
		}
	}
}
