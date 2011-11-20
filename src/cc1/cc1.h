#ifndef CC1_H
#define CC1_H

enum section_type
{
	SECTION_BSS,
	SECTION_DATA,
	SECTION_TEXT,
	NUM_SECTIONS
};

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

#endif
