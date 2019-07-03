#ifndef CC1_OUT_H
#define CC1_OUT_H

#include "out/section.h"

extern struct section_output
{
	struct section sec;
	FILE *file;
} cc1_current_section_output;

#define SECTION_OUTPUT_UNINIT { { NULL, SECTION_UNINIT }, NULL }

#endif
