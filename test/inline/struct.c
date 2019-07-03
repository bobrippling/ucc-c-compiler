// RUN: %ucc -fsyntax-only %s

inline struct A *f()
{
	return (void *)0;
}
