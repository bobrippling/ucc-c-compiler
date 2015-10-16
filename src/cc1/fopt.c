#include <string.h>

#include "fopt.h"

void fopt_default(struct cc1_fopt *opt)
{
	memset(opt, 0, sizeof *opt);

	opt->const_fold = 1;
	opt->show_line = 1;
	opt->pic = 1;
	opt->builtin = 1;
	opt->track_initial_fnam = 1;
	opt->integral_float_load = 1;
	opt->symbol_arith = 1;
	opt->signed_char = 1;
	opt->cast_w_builtin_types = 1;
	opt->print_typedefs = 1;
	opt->print_aka = 1;
	opt->common = 1;
	opt->thread_jumps = 1;
}

int fopt_on(struct cc1_fopt *fopt, const char *argument, int invert)
{
#define X(arg, memb) else if(!strcmp(argument, arg)){ fopt->memb = !invert; return 1; }
#define ALIAS(arg, memb) X(arg, memb)
#define INVERT(arg, memb) else if(!strcmp(argument, arg)){ fopt->memb = invert; return 1; }
	if(0);
#include "fopts.h"
#undef X
#undef ALIAS
#undef INVERT

	return 0;
}
