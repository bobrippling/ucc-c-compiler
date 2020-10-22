// RUN: %ucc -P -E %s | grep -F '(1, 2) + (3, 4)'
#define F(a, b) a + b

F((1, 2), (3, 4))
