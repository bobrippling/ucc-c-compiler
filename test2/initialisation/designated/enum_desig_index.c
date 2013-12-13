// RUN: %ucc -c %s

enum { A };

int x[] = {
	[A] = 2
};
