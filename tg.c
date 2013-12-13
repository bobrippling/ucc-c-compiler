/*
hacky.c

Written on 9/19/13 by @C0deH4cker for reasons unknown.
Inspired by http://carolina.mff.cuni.cz/~trmac/blog/2005/the-ugliest-c-feature-tgmathh/

DISCLAIMER:
No preprocessors were harmed in the creation of this code.
*/
/*
My whole purpose behind writing this was to create the next_ macro. I commonly write
next_int and next_float functions for classwork problems to make them easier, so I
decided to make it easier on myself (lol) and write one macro to handle them all.
Please try not to suffer too terribly much while reading these macros.

And yes, this compiles under both gcc and clang without any warnings and runs correctly.
*/
#ifdef __UCC__
#  define NULL (void *)0
#  define EXIT_FAILURE 1
#  define Q(x) #x
#  define FILE(nam) nam asm(Q(___##nam##p))
extern void *FILE(stdin), *FILE(stdout), *FILE(stderr);
extern void exit(), printf(), fprintf(), fscanf();

#else
#  include <stdio.h>
#  include <stdlib.h>
#endif


/* GET_FORMAT
Very hacky macro to return a format string for printf or scanf given the type
*/
#define GET_FORMAT(type) ( \
		/* if(type is some form of integer) */ \
		((type)1.1 == 1) ? ( \
			/* if(signedness of type is unsigned) */ \
			((type)-1 > 0) ? ( \
				/* type is unsigned long long */ \
				(sizeof (type) == sizeof (unsigned long long)) ? \
				"%llu" \
				: ( /* else 1 */ \
					/* type is unsigned long */ \
					(sizeof (type) == sizeof (unsigned long)) ? \
					"%lu" \
					: ( /* else 2 */ \
						/* type is unsigned int */ \
						(sizeof (type) == sizeof (unsigned int)) ? \
						"%u" \
						: ( /* else 3 */ \
							/* type is unsigned short */ \
							(sizeof (type) == sizeof (unsigned short)) ? \
							"%hu" \
							: ( /* else 4 */ \
								/* type is unsigned char */ \
								(sizeof (type) == sizeof (unsigned char)) ? \
								"%hhu" \
								: NULL /* else error */ \
								) /* 4 */ \
							) /* 3 */ \
						) /* 2 */ \
					) /* 1 */ \
) : ( /* else */ \
	/* signed */ \
	/* type is long long */ \
	(sizeof (type) == sizeof (long long)) ? \
	"%lld" \
	: ( /* else 1 */ \
		/* type is long */ \
		(sizeof (type) == sizeof (long)) ? \
		"%ld" \
		: ( /* else 2 */ \
			/* type is int */ \
			(sizeof (type) == sizeof (int)) ? \
			"%d" \
			: ( /* else 3 */ \
				/* type is short */ \
				(sizeof (type) == sizeof (short)) ? \
				"%hd" \
				: ( /* else 4 */ \
					/* type is signed char */ \
					(sizeof (type) == sizeof (signed char)) ? \
					"%hhd" \
					: NULL /* else error */ \
					) /* 4 */ \
				) /* 3 */ \
			) /* 2 */ \
		) /* 1 */ \
) \
) : ( /* else */ \
	/* type is some floating point type */ \
	/* type is long double */ \
	(sizeof (type) == sizeof (long double)) ? \
	"%llf" \
	: ( /* else 1 */ \
		/* type is double */ \
		(sizeof (type) == sizeof (double)) ? \
		"%lf" \
		: ( /* else 2 */ \
			/* type is float */ \
			(sizeof (type) == sizeof (float)) ? \
			"%f" \
			: /* else */ \
			/* Unknown type. This will error out */ \
			NULL \
			) /* 2 */ \
		) /* 1 */ \
	) \
)


/* next_
	 Macro which will read the next value of the corresponding type from fp and return it.
	 */
#define next_(type, fp) ({ \
		type _ret; \
		const char* fmt = GET_FORMAT(type); \
		if(!fmt) { \
		fprintf(stderr, "Unknown type: %s\n", #type); \
		exit(EXIT_FAILURE); \
		} \
		fscanf((fp), fmt, &_ret); \
		_ret; \
		})


/* TEST_INPUT
	 Useful to make sure that the hacky macros actually work properly.
	 */
#define TEST_INPUT(type, fmt) do { \
	type _val; \
	printf("Enter a " #type ": "); \
	_val = next_(type, stdin); \
	printf("val: " fmt "\n", _val); \
} while(0)


main()
{
	TEST_INPUT(long long, "%lld");
	TEST_INPUT(float, "%f");
	TEST_INPUT(unsigned int, "%u");
	return 0;
}
