// RUN: %ucc -fsyntax-only %s

enum A
{
	A
};

/* this ensures alignof() on an enum works - previously it
 * would cause a segfault because it was accessing the enum
 * part of a identifiers .bits union */
_Static_assert(__alignof__(A) == sizeof(A), "");
