#ifndef CC1_H
#define CC1_H

enum
{
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	NUM_SECTIONS
};

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

#endif
