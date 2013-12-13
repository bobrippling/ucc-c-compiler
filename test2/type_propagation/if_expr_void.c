// RUN: %ucc -fsyntax-only %s

_Static_assert(
		__builtin_types_compatible_p( // can't use _Generic - void is incomplete
			__typeof(*(0 ? (int*)0 : (void*)1)),
			void),
		"bad null pointer logic");
