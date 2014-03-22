#ifndef STD_H
#define STD_H

enum c_std
{
	/* comparable with < */
	STD_C89,
	STD_C90,
	STD_C99,
	STD_C11,
};

/* returns 0 on success */
int std_from_str(const char *, enum c_std *, int *gnu);

#endif
