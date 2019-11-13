// RUN: %check --only %s -std=c2x -Wgnu -Wno-gnu-typeof

int main()
{
	int i = 3;

	_Static_assert(
			1 == _Generic(
				(__typeof(i)[]){ 1, 2, 3 }[1],
				//          ^~ was a bug parsing this
				int: 1));

	struct A { int x; } *p = 0;

	(void)sizeof(p)->x;
	(void)_Alignof(p)->x; // CHECK: _Alignof applied to expression is a GNU extension
	(void)__alignof(p)->x; // CHECK: _Alignof applied to expression is a GNU extension
}
