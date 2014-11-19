// RUN: %ucc -fsyntax-only %s 2>&1 | grep -F ':3:6: note: from here'

enum an_enum
{
	X
};

enum an_enum
{
	REDEFINED
};
