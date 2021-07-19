void volatile_reads() {
	int obj = 1;
	volatile int vobj = 2;

#if 0
	A scalar volatile object is read when it is accessed in a void context:

	```
	volatile int *src = somevalue;
	*src;
	```

	Such expressions are rvalues, and GCC implements this as a read of the volatile object being pointed to.

	Assignments are also expressions and have an rvalue. However when assigning to a scalar volatile, the volatile object is not reread, regardless of whether the assignment expression's rvalue is used or not. If the assignment's rvalue is used, the value is that assigned to the volatile object. For instance, there is no read of vobj in all the following cases:
#endif

	vobj = something;
	obj = vobj = something;
	obj ? vobj = onething : vobj = anotherthing;
	obj = (something, vobj = anotherthing);
}
