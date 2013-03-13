typedef struct User {
	const char *name;
} User;

modify(User *u)
{
	u->name = "Paul";
}

run(User *u)
{
	printf("%s\n", u->name);
	modify(u);
	printf("%s\n", u->name);
}

main()
{
	User *u = (User[]){{ .name = "Leto" }};
	run(u);

	run(&(User){ .name = "Tim" });
}
