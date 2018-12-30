// RUN: %ucc -S -o- %s | grep 'f:'

static void f()
{
}

void (*p)() = 0 ? f : 0;
