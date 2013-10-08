// RUN: %ucc -E %s > /dev/null
// RUN: %check -e %s

enum
{
	Y = '', // CHECK: /error: empty char constant/
};
