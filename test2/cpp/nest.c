// RUN: %ucc -E %s | %output_check '(1, 2) + (3, 4)'
#define F(a, b) a + b

F((1, 2), (3, 4))
