#ifndef CC1_H
#define CC1_H

enum section_type
{
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	NUM_SECTIONS
};

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

#endif
