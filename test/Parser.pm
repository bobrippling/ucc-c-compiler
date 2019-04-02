package Parser;

# exports:
#   parse_warnings, which returns { file, line, col, msg }
#   chomp_all

sub chomp_all
{
	return map { chomp; $_ } @_;
}

sub parse_warnings
{
	my @warnings;

	for(my $i = 0; $i < @_; ++$i){
		$_ = $_[$i];

		if(/^([^:]+):([0-9]+):(([0-9]+):)? *(.*)/){
			my $w = {
				file => $1,
				line => $2,
				col => $4 || 0,
				msg => $5,
				quote => undef,
				caret_spc => -1,
			};
			push @warnings, $w;

			# check for ..."quote"...
			if($i + 1 < @_ && $_[$i + 1] =~ /^ *(\.\.\.)?"(.*)"/){
				$w->{quote} = $2;
				$i++;
			}
			if($i + 1 < @_ && $_[$i + 1] =~ /^( +)\^/){
				my $spc = $1;
				$w->{caret_spc} = ($spc =~ s/ //g) - 3; # count spaces
				$i++;
			}
		}
	}

	return @warnings;
}

return 1;
