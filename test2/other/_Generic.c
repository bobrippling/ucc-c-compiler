// RUN: %ucc -o %t %s
// RUN: %t | grep 'yo'
// RUN: %t | grep '5'

main()
{
#define print(x) \
	printf(_Generic(x, char *: "%s", int: "%d"), x)

#define newline() putchar(10)

	int i = 5;
	char *s = "yo";

	print(i);
	newline();

	print(s);
	newline();
}
