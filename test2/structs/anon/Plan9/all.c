// RUN: %ucc -fplan9-extensions -o %t %s
// RUN: %t

/*
3.3. Unnamed substructures
	The most important and most heavily used of the extensions is the
	declaration of an unnamed substructure or subunion. For example:
*/

typedef struct lock
{
	int locked;
} Lock;

typedef struct node
{
	int type;
	union
	{
		double dval;
		float  fval;
		long   lval;
	};
	Lock;
} Node;

Lock *lock;
Node  node;

/*
The declaration of Node has an unnamed substructure of type Lock and an unnamed
subunion. One use of this feature allows references to elements of the subunit
to be accessed as if they were in the outer structure. Thus node->dval and
node->locked are legitimate references.

When an outer structure is used in a context that is only legal for an unnamed
substructure, the compiler promotes the reference to the unnamed substructure.
This is true for references to structures and to references to pointers to
structures. This happens in assignment statements and in argument passing where
prototypes have been declared. Thus, continuing with the example,
*/

assign()
{
    lock = &node;
}

/*
would assign a pointer to the unnamed Lock in the Node to the variable lock.
Another example,
*/

do_lock(Lock *l)
{
	l->locked = 1;
}

call()
{
	do_lock(&node);
	if(!node.locked)
		abort();
}

/*
will pass a pointer to the Lock substructure.

Finally, in places where context is insufficient to identify the unnamed
structure, the type name (it must be a typedef) of the unnamed structure
can be used as an identifier. In our example, &node->Lock gives the
address of the anonymous Lock structure.
*/

address()
{
	Lock *l = &node.Lock;
	if(!node.locked)
		abort();
}

main()
{
	assign();
	call();
	address();
	return 0;
}
