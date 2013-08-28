// RUN: %ucc -fsyntax-only %s

enum
{
	sh_enu = (short)-1
};

_Static_assert(sh_enu != -1, "enums shouldn't compare equal to -1");
