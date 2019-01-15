#ifndef CC1_OUT_H
#define CC1_OUT_H

#include "out/asm.h"

extern struct section_output
{
	struct section sec;
	FILE *file;
} cc1_current_section_output;

#endif
