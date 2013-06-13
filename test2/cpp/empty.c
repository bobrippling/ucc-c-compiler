// RUN: %ucc -E %s | %output_check '+ + ;'
#define F(a, b, c) a + b + c

F(,,);
