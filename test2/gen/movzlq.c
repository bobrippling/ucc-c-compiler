// RUN: %ucc -S -o- %s | grep 'movzlq'
struct timespec
{
	long tv_sec, tv_nsec;
};

zlq(unsigned int sec)
{
	int f(struct timespec *);
	struct timespec tsp;
	tsp.tv_sec = sec;
	f(&tsp);
}
