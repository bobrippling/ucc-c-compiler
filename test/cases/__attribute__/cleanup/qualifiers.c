// RUN: %check -e %s

void f_i(int *);
void f_ki(const int *);

void f_i_pk(int *const);
void f_ki_pk(const int *const);

int main()
{
	{
		const int i1 __attribute((cleanup(f_i))); // CHECK: error: type 'int const *' passed - cleanup needs 'int *'
		int j1 __attribute((cleanup(f_i))); // CHECK: !/error/

		const int i2 __attribute((cleanup(f_ki))); // CHECK: !/error/
		int j2 __attribute((cleanup(f_ki))); // CHECK: !/error/
	}

	// pointer/top-level constness:
	{
		const int i1 __attribute((cleanup(f_i_pk))); // CHECK: error: type 'int const *' passed - cleanup needs 'int *'
		int j1 __attribute((cleanup(f_i_pk))); // CHECK: !/error/

		const int i2 __attribute((cleanup(f_ki_pk))); // CHECK: !/error/
		int j2 __attribute((cleanup(f_ki_pk))); // CHECK: !/error/
	}
}
