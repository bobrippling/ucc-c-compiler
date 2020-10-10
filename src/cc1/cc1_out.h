#ifndef CC1_OUT_H
#define CC1_OUT_H

#include "out/section.h"

extern struct cc1_output
{
	struct section section;
	FILE *file;
} cc1_output;

#define SECTION_OUTPUT_UNINIT { { NULL, SECTION_UNINIT, 0 }, NULL }

/* returns true if added (i.e. not present already) */
int cc1_outsections_add(const struct section *);

#endif
