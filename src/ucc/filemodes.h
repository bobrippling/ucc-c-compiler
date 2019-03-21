#define FILEMODES \
	X(preproc, "c", 'i') \
	X(compile, "cpp-output", 's') \
	X(assemb, "assembler", 'o') \
	ALIAS(assemb, "asm") \
	X(assemb_with_cpp, "assembler-with-cpp", 'o') \
	ALIAS(assemb_with_cpp, "asm-with-cpp") \
