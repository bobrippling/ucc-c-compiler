// RUN: %layout_check %s

typedef int _AlignT;
enum {
	_Len = sizeof(int),
	_Align = _Alignof(_AlignT),
};

union U1
{
	unsigned char __data[_Len];
	struct __attribute__((__aligned__((_Align)))) S1 { } __align;
} s1;

union U2
{
	unsigned char __data[_Len];
	_Alignas(_Align) struct S2 { } __align;
} s2;

union U3
{
	unsigned char __data[_Len];
	_Alignas(__alignof(_AlignT)) struct S3 { } __align;
} s3;

union U4
{
	unsigned char __data[_Len];
	_Alignas(_AlignT) struct S4 { } __align;
} s4;

#define assert_aligned(T, as) \
	_Static_assert(_Alignof(T) == _Alignof(as), #T " not aligned as " #as); \
	_Static_assert(__alignof__(T) == _Alignof(as), #T " not aligned as " #as); \
	_Static_assert(_Alignof(T) == __alignof__(as), #T " not aligned as " #as)

assert_aligned(s1, int);
assert_aligned(s2, int);
assert_aligned(s3, int);
assert_aligned(s4, int);

assert_aligned(union U1, int); assert_aligned(struct S1, int);
assert_aligned(union U2, int); assert_aligned(struct S2, char); // char - only the decl is int-aligned
assert_aligned(union U3, int); assert_aligned(struct S3, char);
assert_aligned(union U4, int); assert_aligned(struct S4, char);
