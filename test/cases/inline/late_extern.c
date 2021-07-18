// RUN: ! %ucc -S -o- %s | grep 'f:'

static void f();

inline void f()
{
}

extern void f(); // too late - after definition
