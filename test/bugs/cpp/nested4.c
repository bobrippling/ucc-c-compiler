#define SIZE            256

#define bog             0
#define fog(x)          bog
#define dog(x)          fog(), fog()

#define bar(x)          dog(), fog(), pr("SIZE")

#define foo(x)          bar(), bar()

void pr(char *a)
{
}

int main(int argc, char *argv[])
{
	// should be:
	// 0, 0, 0, pr("SIZE"), 0, 0, 0, pr("SIZE");
	foo();

	return 0;
}

