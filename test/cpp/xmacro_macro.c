// RUN: %ucc -E %s | grep -F 'void abc(void); void def(void); void ghi(void);'

#define XMACS \
	XM(abc)     \
	XM(def)     \
	XM(ghi)

#define XM(f) void f(void);
XMACS
#undef XM
