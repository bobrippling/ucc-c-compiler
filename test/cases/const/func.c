// RUN: %check --only %s -Wnonstandard-init

// FIXME: this test (specifically "--only") depends on d780b9e29052d9d7aac641df35cd88ec38125336 in fix/printf-precision-warn

struct api
{
  int *(*init)(void);
};

int *init(void);

static struct api api = {
	.init = init, // no warnings
};
