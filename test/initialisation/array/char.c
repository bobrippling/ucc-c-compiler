// RUN: %layout_check %s

char abc[] = "abc";
char abc2[3] = { "abc" };

char *p = "hi";

char s[][6] = {
	{ "hi" },
	"there"
};

struct
{
	int i;
	char s[3];
} x = {
	1, "hi"
}, xs[] = {
	1, "o",
	{ .s = "des" },
	2, "t",
};
