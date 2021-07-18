// RUN: %layout_check %s

static void f()
{
}

void (*p)() = 0 ? f : 0;
void (*q)() = 0 ? 0 : f;
