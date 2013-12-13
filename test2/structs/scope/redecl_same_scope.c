// RUN: %ucc -fsyntax-only %s

f()
{
	struct A { int i; };
	struct A; // fine, redundant prototype

	struct A yo;

	yo.i = 3;
}
