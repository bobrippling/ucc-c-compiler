// RUN: %ocheck 0 %s
int strcmp(const char *s1, const char *s2);
void abort(void) __attribute__((noreturn));

typedef struct
{
	const char *name;
} User;

modify(User *u)
{
	u->name = "Mod";
}

int i;
run(User *u)
{
	if(strcmp(u->name, i++ == 0 ? "Norm" : "Tim"))
		abort();
	modify(u);
	if(strcmp(u->name, "Mod"))
		abort();
}

main()
{
	run((User[]){{ .name = "Norm" }});
	run(&(User){ .name = "Tim" });
	return 0;
}
