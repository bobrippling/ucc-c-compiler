// http://www.open-std.org/jtc1/sc22/wg14/www/docs/dr_413.htm

// RUN: %layout_check %s

typedef struct {
	int k;
	int l;
	int a[2];
} T;

typedef struct {
	int i;
	T t;
} S;

T x = {.l = 43, .k = 42, .a[1] = 19, .a[0] = 18 };
// { 42, 43, 18, 19 }

f()
{
	S l = { 1, .t = x, .t.l = 41, .t.a[1] = 17};
	return l.t.k;
}

main()
{
	return f();
}


/* The question is: what is now the value of l.t.k? Is it 42 (due to the
 * initialization of .t = x) or is it 0 (due to the fact that .t.l starts an
 * incomplete initialization of .t?  The relevant clause from the standard is
 * 6.7.9 clause 19:
 *
 * 19 The initialization shall occur in initializer list order, each
 * initializer provided for a particular subobject overriding any previously
 * listed initializer for the same subobject;151) all subobjects that are
 * not initialized explicitly shall be initialized implicitly the same as
 * objects that have static storage duration.
 */
