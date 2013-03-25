int _Alignas(8) i;
struct { int _Alignas(16) j; } a;

int align_i  = _Alignof(i);
int align_st = _Alignof(a.j);
