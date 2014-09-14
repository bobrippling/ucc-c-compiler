// RUN: %check -e %s

int bad(char ch[static *]); // CHECK: error: 'static' can't be used with a star-modified array

void bad2(char buf[*]) // CHECK: error: star modifier can only appear on prototypes
{
	(void)buf;
}

// check nested star
void bad3(char (*buf)[*]) // CHECK: error: star modifier can only appear on prototypes
{
	(void)buf;
}
