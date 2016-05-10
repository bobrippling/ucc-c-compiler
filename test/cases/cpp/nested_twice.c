// RUN: %ucc -P -E %s | grep '1+1+1 + 1+1+1'
#define G(y) y+1
#define F(x) G(x) + G(x)

F(G(1))
