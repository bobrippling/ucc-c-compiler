// RUN: %layout_check %s

#define INIT(...) (int[]){ __VA_ARGS__ }

int y[] = { 5, 6 };

int (*p)[] = &INIT(3, 4);
