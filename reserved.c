int main() // no warning
{
	__yo(); // warning
	_Yo(); // warning

	_yo(); // no warning
}

_yo() // warning
{
}

struct _hello; // warning
struct __hello; // warning
struct _Hello; // warning
