#define CMP(nam, op)   \
nam(float a, float b)  \
{                      \
	return a op b;       \
}

CMP(ge, >=)
CMP(gt, > )
CMP(le, <=)
CMP(lt, < )
CMP(eq, ==)
CMP(ne, !=)

five(float a, float b)
{
	return
		ge(a, b) ||
		gt(a, b) ||
		le(a, b) ||
		lt(a, b) ||
		eq(a, b);
}

main()
{
 float nan = __builtin_nanf("");

 if(five(nan, nan)) return 1;
 if(five(5, nan)) return 2;
 if(five(nan, 5)) return 3;
 if(!five(3, 5)) return 4;

 if(!ne(nan, nan)) return 5;
 if(!ne(3, nan)) return 6;
 if(!ne(nan, 3)) return 7;
 if(ne(3, 3)) return 8;

 return 0;
}
