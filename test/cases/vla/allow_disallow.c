/* All declarations of variably modified (VM) types have to be at either block
 * scope or function prototype scope.
 *
 * Array objects declared with the _Thread_local, static, or extern
 * storage-class specifier cannot have a variable length array (VLA) type.
 *
 * However, an object declared with the static storage-class specifier can have
 * a VM type (that is, a pointer to a VLA type).
 *
 * Finally, all identifiers declared with a VM type have to be ordinary
 * identifiers and cannot, therefore, be members of structures or unions.
 */

// RUN: %check -e %s

extern int n;
int A[n]; // CHECK: error: static-duration variable length array
extern int (*p2)[n]; // CHECK: error: static-duration variably modified type

fn(int m)
{
	typedef int VLA[m][m]; // CHECK: !/error/
	struct tag {
		int (*y)[n]; // CHECK: error: member has variably modifed type 'int (*)[vla]'
		int z[n]; // CHECK: error: member has variably modifed type 'int[vla]'
	};
	int D[m]; // CHECK: !/error/
	static int E[m]; // CHECK: error: static-duration variable length array
	extern int F[m]; // CHECK: error: extern-linkage variable length array
	int (*s)[m]; // CHECK: !/error/
	extern int (*r)[m]; // CHECK: error: extern-linkage variably modified type
	static int (*q)[m]; // CHECK: !/error/
	q = &D;
}

