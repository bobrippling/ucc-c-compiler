// RUN: %ocheck 3 %s -fno-semantic-interposition

__attribute((always_inline))
inline int f(void)
{
	return 3;
}

extern int f(void);

int main()
{
	return f();
}
