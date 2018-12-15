// RUN: %ucc -fsyntax-only %s

struct A
{
	signed   : 1,   : 2; // neither
	signed a : 3, b : 4; // both
	signed d : 5,   : 6; // first
	signed   : 7, c : 8; // last
};
