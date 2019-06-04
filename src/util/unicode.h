#ifndef UTIL_UNICODE_H
#define UTIL_UNICODE_H

enum char_type
{
	UNICODE_NO,
	UNICODE_u8, /* used in ordered comparisons */
	UNICODE_u16,
	UNICODE_U32,
	UNICODE_L
};

enum char_type unicode_classify(const char *s, char **end);

#endif
