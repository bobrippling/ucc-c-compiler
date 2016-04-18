// RUN: %layout_check --sections %s -target x86_64-linux

int tenative;
int initialised = 3;
const int ro_tenative;
const int ro_initialised = 4;


__attribute((section("a")))
int tenative2;

__attribute((section("b")))
int initialised2 = 3;

__attribute((section("c")))
const int ro_tenative2;

__attribute((section("d")))
const int ro_initialised2 = 4;

__attribute((section("e")))
int f()
{
	return 3;
}

int main()
{
}
