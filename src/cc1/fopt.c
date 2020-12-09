#include <string.h>

#include "fopt.h"

void fopt_default(struct cc1_fopt *opt)
{
	memset(opt, 0, sizeof *opt);

	opt->builtin = 1;
	opt->signed_char = 1;

	opt->print_aka = 1;
	opt->print_typedefs = 1;
	opt->show_line = 1;
	opt->track_initial_fnam = 1;
	opt->colour_diagnostics = 1;

	opt->cast_w_builtin_types = 1;
	opt->common = 1;
	opt->const_fold = 1;
	opt->integral_float_load = 1;
	opt->omit_frame_pointer = 1;
	opt->pic = 1;
	opt->plt = 1;
	opt->rounding_math = 0; /* default to no rounding math, aka float-const-folding/fenv-non-conforming */
	opt->semantic_interposition = 1; /* default to -fsemantic-interposition to match gcc */
	opt->symbol_arith = 1;
	opt->thread_jumps = 1;
	opt->zero_init_in_bss = 1;
}

unsigned char *fopt_on(struct cc1_fopt *fopt, const char *argument, int invert)
{
#define X(arg, memb) else if(!strcmp(argument, arg)){ fopt->memb = !invert; return &fopt->memb; }
#define ALIAS(arg, memb) X(arg, memb)
#define INVERT(arg, memb) else if(!strcmp(argument, arg)){ fopt->memb = invert; return &fopt->memb; }
#define EXCLUSIVE(arg, memb, excl) \
	else if(!strcmp(argument, arg)){ \
		fopt->memb = !invert;          \
		fopt->excl = 0;                \
		return &fopt->memb;            \
	} /* -fpic -fno-pie is equivalent to -fno-pie - any prior pic options are overwritten */
#define ALIAS_EXCLUSIVE(arg, memb, excl) EXCLUSIVE(arg, memb, excl)

	if(0);
#include "fopts.h"
#undef X
#undef ALIAS
#undef INVERT
#undef EXCLUSIVE
#undef ALIAS_EXCLUSIVE

	return NULL;
}
