// RUN: %layout_check %s

extern int i = 5; // extern removed

extern int m;
int m; // m is implicit def

extern int n; // ignored
int n = 5;
int l;
extern int l; // ignored
