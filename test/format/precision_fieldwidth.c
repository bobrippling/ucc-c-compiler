// RUN: %check --only --prefix=linux %s -target x86_64-linux
// RUN: %check --prefix=darwin %s -target x86_64-darwin

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;

int main()
{
	// TODO:
	// [ ] test all these
	// [ ] truncations part way
	// [ ] bad length modifiers

	// modifiers: "#0- +'"
	// field width: 0-9 | *
	// precision: . ( 0-9 | * )
	// length modifiers: 'h' 'l' 'L' 'j' 't' 'z'
	// format char
	// %m on {,non-}linux

	printf("%#f", 5.2f);
	printf("%0f", 5.2f);
	printf("%-f", 5.2f);
	printf("% f", 5.2f);
	printf("%+f", 5.2f);
	printf("%'f", 5.2f);
	printf("%#0' +f", 21.f);

	printf("%23d", 5);
	printf("%35a", 5.2);
	printf("%35.a", 5.2);
	printf("%*.g", 3, 5.2);
	printf("%'.*g", 3, 5.2);
	printf("%#2.*g", 3, 5.2);
	printf("%+*.1e", 3, 5.2);

	printf("%m\n"); // CHECK-darwin: warning: %m used on non-linux system

	printf("%+*.1q", 3, 5.2); // CHECK-linux: warning: invalid conversion character

	printf("%#"); // CHECK-linux: warning: invalid modifier character

	printf("%23"); // CHECK-linux: warning: invalid field width
	printf("%35."); // CHECK-linux: warning: incomplete format specifier (missing precision)
	printf("%*"); // CHECK-linux: warning: invalid field width
	printf("%'.*"); // CHECK-linux: warning: invalid precision
	printf("%+*.1"); // CHECK-linux: warning: invalid precision

	printf("%.*f\n", (size_t)5, 3.2); // CHECK-linux: warning: precision for %.*f expects int argument, not unsigned long
	printf("%*s", 2l, ""); // CHECK-linux: warning: field width for %*s expects int argument, not long
}
