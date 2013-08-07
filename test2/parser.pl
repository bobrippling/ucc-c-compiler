# exports parse_warnings, which returns { file, line, col, msg }

sub parse_warnings
{
	my @warnings;

	for(@_){
		if(/^([^:]+):([0-9]+):(([0-9]+):)? *(.*)/){
			push @warnings, {
				file => $1,
				line => $2,
				col => $4 || 0,
				msg => $5
			};
		}
	}

	return @warnings;
}

return 1;
