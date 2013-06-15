// RUN: %ucc -E %s | grep '+ *+ *;' > /dev/null
#define F(a, b, c) a + b + c

F(,,);
