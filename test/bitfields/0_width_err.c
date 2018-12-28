// RUN: %check -e %s
struct B
{
	int a : 6;
	int y : 0; // CHECK: /error: none-anonymous bitfield "y" with 0-width/
	int b : 2;
};
