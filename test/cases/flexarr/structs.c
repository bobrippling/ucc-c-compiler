// RUN: %ocheck 8 %s
struct A
{
	int n;
	// pad of 4
	struct Ent
	{
		char *nam;
		int type;
	} ents[];
};

main()
{
#include "../ocheck-init.c"
	return sizeof(struct A);
}
