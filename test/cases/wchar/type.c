// RUN: %ucc -c %s
#define TYPE_CHECK(ch, ty, s)     \
_Static_assert(                   \
		__builtin_types_compatible_p( \
			__typeof(ch), ty),            \
		"sizeof "s" constant != int")

TYPE_CHECK( 'a', int, "char");
TYPE_CHECK(L'a', int, "wchar");
