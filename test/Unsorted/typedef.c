int typedef typedef_int;
typedef int *intptr_t;
void typedef typedef_void;

typedef_void x(intptr_t);

typedef_int main()
{
	typedef_int i;
	typedef_int *p;
	intptr_t q;
	intptr_t *pq;

	pq = &q;

	p = q = (typedef_int *)&i;
	(typedef_void)(i = (typedef_int)2);

	function();

	return **pq; /* 2 */
}

typedef int typedef_int2;
typedef typedef_int2 *intptr_t2;

typedef_int2 function()
{
	typedef_int2 p;
	intptr_t2 q;
	*(q = &p) = 2;
	return p;
}
