#ifndef WARNING_H
#define WARNING_H

enum warning_owner
{
	W_OWNER_CPP = 1 << 0,
	W_OWNER_CC1 = 1 << 1
};

enum warning_owner warning_owner(const char *);

#endif
