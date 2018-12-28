// RUN: %ucc -S -o- %s | grep -F 'f:'

extern int f(void); // forces code-gen of f()

inline int f(void)
{
	return 3;
}
