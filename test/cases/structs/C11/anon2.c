// RUN: %ocheck 3 %s

struct nest {
	union {
		struct {
			union {
				int i;
			};
		};
	};
};

int main()
{
	struct nest n = {
		.i = 2
	};
	n.i++;
	return n.i;
}
