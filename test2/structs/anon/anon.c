struct B{int i;};
struct
{
	struct A;
	struct B;
	struct B{int i;};
} jim;

jim.i = 2; // error

struct XY_or_WX
{
	union
	{
		struct
		{
			char x, y;
		};
		struct
		{
			char w, h;
		};
	};
};

struct XY_or_WX tim;

tim.x = 2;
