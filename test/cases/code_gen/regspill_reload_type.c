// RUN: %ocheck 0 %s

int strcmp(const char *, const char *);

char *f(char *p, int c)
{
	// this tests that reloading -1 (or 0) brings its original type (char *) back.
	// if its type is wrong, we'll attempt to ptrstep the value and return an
	// incorrectly offset pointer

	return p + (c ? 0 : -1);
}

char *prev(char ***pa, int n)
{
	// similar idea, only this time the type is something that
	// needs a ptrstep adjustment
	return (*pa)[n - 1];
}

int main()
{
	char buf[32] = "hello";;

	*f(buf + 4, 0) = '\0';

	if(buf[0] != 'h'
	|| buf[1] != 'e'
	|| buf[2] != 'l'
	|| buf[3] != '\0'
	|| buf[4] != 'o')
	{
		return 1;
	}

	char **ents = (char *[]){
		"hi",
		"there",
		0
	};

	if(prev(&ents, 3))
		return 2;
	if(strcmp(prev(&ents, 2), "there"))
		return 3;
	if(strcmp(prev(&ents, 1), "hi"))
		return 4;

	return 0;
}
