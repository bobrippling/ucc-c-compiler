typedef void *id;

struct A
{
	void (^blk)(id);
};

typeof(((struct A *)0)->blk) yo;

struct A a = {^{}};
