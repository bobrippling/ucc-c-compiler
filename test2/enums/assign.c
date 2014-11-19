// RUN: %check %s -Weverything

enum A
{
	ABC, DEF
};

enum Z
{
	XYZ
};

takes_a(enum A);

main()
{
	enum Z z = ABC; // CHECK: /warning: implicit conversion from 'enum A' to 'enum Z'/

	takes_a(z); // CHECK: /warning: implicit conversion from 'enum Z' to 'enum A'/
	takes_a(ABC); // CHECK: !/warn/
	takes_a(XYZ); // CHECK: /warning: implicit conversion from 'enum Z' to 'enum A'/
}
