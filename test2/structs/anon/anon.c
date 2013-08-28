// RUN: %ucc -fsyntax-only %s

struct
{
	struct B{int i;};
} jim;

q()
{
	jim.i = 2;
}

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

f()
{
	struct XY_or_WX tim;

	tim.x = 2;
}
