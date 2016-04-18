// RUN: %check %s

__attribute((aligned(2))) // CHECK: warning: attribute ignored - no declaration (place attribute after 'enum')
enum A
{
	X
};

enum B
{
	Y
} __attribute((enum_bitmask)); // CHECK: warning: attribute ignored - no declaration (place attribute after 'enum')
