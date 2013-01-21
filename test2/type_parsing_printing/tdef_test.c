typedef int tdef_int;

typedef tdef_int tdef_int2;

tdef_int2 i = 3;

#if 0
i = {
	type = tdef,
	ref = {
		type = tdef,
		ref = {
			type = type,
			ref = NULL
		}
	}
}
#endif




typedef int *intptr;
intptr p = (void *)0;

#if 0
p = {
	type = tdef,
	ref = {
		type = ptr,
		ref = {
			type = type,
			ref = NULL
		}
	}
}
#endif


static intptr x;

#if 0
#endif
