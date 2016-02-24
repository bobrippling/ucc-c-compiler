typedef struct colour {
	int padding;
	struct { // index 1
		int a;
		int b;
		struct{ // index 2
			int c,d,e;
			struct{ // index 3
				struct{ // index 0
					int f, g, h, i, j; // index 4
				};
			};
		};
	};
	int after;
} colour;

colour x = { .j = 3 };

int *p = &x.j;

f(colour *p)
{
	return p->j;
}
